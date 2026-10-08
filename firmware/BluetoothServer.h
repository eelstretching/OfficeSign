#ifndef BLUETOOTHSERVER_H
#define BLUETOOTHSERVER_H

#pragma once

#include "Mailbox.h"
#include "btstack.h"

/// @brief Runs the sign's BLE GATT server. Owns core 1 for the lifetime of
/// the program: run() never returns.
///
/// Wraps the setup pattern from the Pico SDK's gatt_counter.c reference
/// example (l2cap/sm/att_server init, advertising, HCI/SM/ATT event
/// handling) as a class. BTStack's C callback registrations take plain
/// function pointers with no user-data parameter, so a static
/// trampoline-forwards-to-singleton is used for that one seam; everything
/// else is a member function.
class BluetoothServer {
   public:
    explicit BluetoothServer(Mailbox* mailbox);

    /// @brief Core 1 entry point. Brings up cyw43/BTStack, signals
    /// BtReadySignal once cyw43 has claimed its PIO resources, then blocks
    /// forever in btstack_run_loop_execute().
    void run();

   private:
    Mailbox* mailbox;
    hci_con_handle_t connHandle = HCI_CON_HANDLE_INVALID;

    btstack_packet_callback_registration_t hciEventReg;
    btstack_packet_callback_registration_t smEventReg;

    void setupBluetoothStack();
    void onPacket(uint8_t packetType, uint16_t channel, uint8_t* packet, uint16_t size);
    int onAttWrite(hci_con_handle_t connHandle, uint16_t attHandle, uint16_t transactionMode,
                    uint16_t offset, uint8_t* buffer, uint16_t bufferSize);
    uint16_t onAttRead(hci_con_handle_t connHandle, uint16_t attHandle, uint16_t offset,
                        uint8_t* buffer, uint16_t bufferSize);

    static BluetoothServer* instance;
    static void packetHandlerTrampoline(uint8_t packetType, uint16_t channel, uint8_t* packet,
                                         uint16_t size);
    static int attWriteTrampoline(hci_con_handle_t connHandle, uint16_t attHandle,
                                   uint16_t transactionMode, uint16_t offset, uint8_t* buffer,
                                   uint16_t bufferSize);
    static uint16_t attReadTrampoline(hci_con_handle_t connHandle, uint16_t attHandle,
                                       uint16_t offset, uint8_t* buffer, uint16_t bufferSize);
};

#endif
