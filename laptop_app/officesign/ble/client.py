"""BLE client for the OfficeSign, built on bleak (asyncio, CoreBluetooth on
macOS). Finds the sign by its advertised service UUID rather than by name or
MAC address, and writes messages to its single control characteristic.

This class only manages a single connection attempt/session; it does not
retry on its own. Staying connected across drops (laptop sleep, walking out
of BLE range, the sign power-cycling) is the reconnector's job -- see
ble/reconnector.py -- so that callers who just want to fire a message off
(the web routes) don't need to know anything about retry policy.
"""

import asyncio
import logging

from bleak import BleakClient, BleakScanner
from bleak.backends.device import BLEDevice

from ..protocol.constants import CONTROL_CHARACTERISTIC_UUID, SERVICE_UUID

logger = logging.getLogger(__name__)


class SignClient:
    def __init__(self) -> None:
        self._client: BleakClient | None = None
        # Starts "disconnected" (set) since we begin with no connection.
        # Cleared on a successful connect, set again the moment bleak
        # notices the link drop -- the reconnector waits on this instead of
        # polling is_connected, so it reacts immediately rather than only
        # discovering staleness whenever the next message happens to be sent.
        self.disconnected_event = asyncio.Event()
        self.disconnected_event.set()

    @property
    def is_connected(self) -> bool:
        return bool(self._client and self._client.is_connected)

    def _on_disconnect(self, _client: BleakClient) -> None:
        logger.warning("OfficeSign disconnected")
        self.disconnected_event.set()

    async def connect(self, timeout: float = 15.0) -> None:
        print("Scanning for OfficeSign. Looking for UUID:", SERVICE_UUID)
        device: BLEDevice | None = await BleakScanner.find_device_by_filter(
            lambda d, adv: SERVICE_UUID in (adv.service_uuids or []),
            timeout=timeout,
        )
        if device is None:
            raise RuntimeError("OfficeSign not found while scanning for BLE devices")

        logger.info("Found OfficeSign at %s, connecting...", device.address)
        self._client = BleakClient(device, disconnected_callback=self._on_disconnect)
        await self._client.connect()
        self.disconnected_event.clear()
        logger.info("Connected to OfficeSign")

    async def disconnect(self) -> None:
        if self._client:
            await self._client.disconnect()
            self._client = None
        self.disconnected_event.set()

    async def send(self, payload: bytes) -> None:
        if not self.is_connected:
            raise RuntimeError("Not connected to OfficeSign")
        assert self._client is not None
        # response=True: write-with-response, so the ATT layer itself acks
        # delivery (this is our only delivery confirmation in v1 -- there's
        # no separate status/notify characteristic).
        await self._client.write_gatt_char(CONTROL_CHARACTERISTIC_UUID, payload, response=True)
