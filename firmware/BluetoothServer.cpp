#include "BluetoothServer.h"

#include <string.h>

#include "BtReadySignal.h"
#include "OfficeSign.h"  // generated from OfficeSign.gatt by pico_btstack_make_gatt_header
#include "SignMessage.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "pico/stdlib.h"

BluetoothServer* BluetoothServer::instance = nullptr;

// Advertising payload: flags + our 128-bit service UUID, so the laptop app
// can find the sign by scanning for that service rather than matching on
// name/MAC address.
//
// 20B1C720-1391-4410-9F7F-7ECE217F621C, transmitted least-significant octet
// first as required for AD structures (i.e. reversed from how the UUID is
// normally written).
//
// The complete local name is deliberately NOT in this packet: legacy BLE
// advertising data is capped at 31 octets total (see gap_advertisements_set_data's
// own doc comment), and flags(3) + name(12) + 128-bit service UUID(18) = 33
// bytes -- 2 over. Since the service UUID is what the laptop app actually
// filters on, that's the one that has to fit here; the name goes in the
// scan response packet instead (a second, separate 31-byte packet BLE
// already provides for exactly this, which bleak's active scan fetches
// automatically).
static const uint8_t kAdvData[] = {
    // Flags: LE General Discoverable, BR/EDR not supported.
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Complete list of 128-bit service class UUIDs.
    0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
    0x1C, 0x62, 0x7F, 0x21, 0xCE, 0x7E, 0x7F, 0x9F, 0x10, 0x44, 0x91, 0x13, 0x20, 0xC7, 0xB1, 0x20,
};

static const uint8_t kScanRespData[] = {
    // Complete local name.
    0x0b, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'O', 'f', 'f', 'i', 'c', 'e', 'S', 'i', 'g', 'n',
};

// Catch a regression at compile time rather than relearn the 31-byte legacy
// advertising limit the hard way again.
static_assert(sizeof(kAdvData) <= 31, "legacy BLE advertising data is capped at 31 octets");
static_assert(sizeof(kScanRespData) <= 31, "legacy BLE scan response data is capped at 31 octets");

BluetoothServer::BluetoothServer(Mailbox* mailbox) : mailbox(mailbox) {}

void BluetoothServer::run() {
    instance = this;

    // We're the "other" core relative to main()'s core 0, so we register as
    // a lockout victim too -- whichever core ends up calling
    // flash_safe_execute() (BTStack's flash-backed link key/device DB
    // storage runs on this core, but nothing stops core 0 from needing this
    // too someday) needs the other one to already be initialized here.
    flash_safe_execute_core_init();

    printf("BluetoothServer starting...\n");

    if (cyw43_arch_init()) {
        assert(1);
        // // Nothing else we can do without the radio; sit here rather than
        // // silently running with no Bluetooth at all.
        // while (true) {
        //     tight_loop_contents();
        // }
    }

    printf("BluetoothServer initialized.\n");

    // Core 0 is waiting to claim PIO/DMA resources for the LED renderer;
    // cyw43_arch_init() above has already claimed the PIO1 state machine it
    // needs, so it's now safe to let core 0 proceed.
    BtReadySignal::signalReady();

    setupBluetoothStack();

    hci_power_control(HCI_POWER_ON);

    btstack_run_loop_execute();  // never returns
}

void BluetoothServer::setupBluetoothStack() {
    l2cap_init();

    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(0);  // Just Works, no bonding (see plan notes)

    att_server_init(profile_data, &attReadTrampoline, &attWriteTrampoline);

    uint16_t advIntMin = 0x0030;
    uint16_t advIntMax = 0x0030;
    uint8_t advType = 0;
    bd_addr_t nullAddr;
    memset(nullAddr, 0, 6);
    gap_advertisements_set_params(advIntMin, advIntMax, advType, 0, nullAddr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(kAdvData), (uint8_t*)kAdvData);
    gap_scan_response_set_data(sizeof(kScanRespData), (uint8_t*)kScanRespData);
    gap_advertisements_enable(1);

    hciEventReg.callback = &packetHandlerTrampoline;
    hci_add_event_handler(&hciEventReg);

    smEventReg.callback = &packetHandlerTrampoline;
    sm_add_event_handler(&smEventReg);

    att_server_register_packet_handler(&packetHandlerTrampoline);
}

void BluetoothServer::onPacket(uint8_t packetType, uint16_t channel, uint8_t* packet, uint16_t size) {
    (void)channel;
    (void)size;

    if (packetType != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                connHandle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                mailbox->setConnected(true);
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            connHandle = HCI_CON_HANDLE_INVALID;
            mailbox->setConnected(false);
            // att_server stops advertising once connected; re-arm it so the
            // laptop app can reconnect later without a firmware reset.
            gap_advertisements_enable(1);
            break;

        default:
            break;
    }
}

int BluetoothServer::onAttWrite(hci_con_handle_t connHandle, uint16_t attHandle,
                                 uint16_t transactionMode, uint16_t offset, uint8_t* buffer,
                                 uint16_t bufferSize) {
    (void)transactionMode;
    (void)offset;

    if (attHandle == ATT_CHARACTERISTIC_D353EE31_9EAB_4FA5_BFF2_258667F6D295_01_VALUE_HANDLE) {
        // Parameter shadows the member of the same name -- qualify explicitly.
        this->connHandle = connHandle;
        SignMessage msg;
        if (SignMessage::decode(buffer, bufferSize, msg)) {
            mailbox->publish(msg);
        }
        // Malformed/truncated writes are silently ignored; write-with-response
        // still ATT-acks the bytes arriving even though we didn't act on them.
    }
    return 0;
}

uint16_t BluetoothServer::onAttRead(hci_con_handle_t connHandle, uint16_t attHandle, uint16_t offset,
                                     uint8_t* buffer, uint16_t bufferSize) {
    (void)connHandle;
    (void)attHandle;
    (void)offset;
    (void)buffer;
    (void)bufferSize;
    // The control characteristic is write-only for v1; static GAP/GATT
    // reads are served by att_server itself and never reach here.
    return 0;
}

void BluetoothServer::packetHandlerTrampoline(uint8_t packetType, uint16_t channel, uint8_t* packet,
                                               uint16_t size) {
    instance->onPacket(packetType, channel, packet, size);
}

int BluetoothServer::attWriteTrampoline(hci_con_handle_t connHandle, uint16_t attHandle,
                                         uint16_t transactionMode, uint16_t offset, uint8_t* buffer,
                                         uint16_t bufferSize) {
    return instance->onAttWrite(connHandle, attHandle, transactionMode, offset, buffer, bufferSize);
}

uint16_t BluetoothServer::attReadTrampoline(hci_con_handle_t connHandle, uint16_t attHandle,
                                             uint16_t offset, uint8_t* buffer, uint16_t bufferSize) {
    return instance->onAttRead(connHandle, attHandle, offset, buffer, bufferSize);
}
