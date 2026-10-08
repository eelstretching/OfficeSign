# CLAUDE.md

The office sign: `firmware/` (Pico 2 W, C++), `laptop_app/` (Python BLE central + web UI), and the
`PicoLEDs/` submodule (the LED library, tracking PicoLEDs `main`). Library changes belong in
PicoLEDs, not here; edit them there and bump the submodule.

## Build (firmware)

```sh
git submodule update --init
PICO_SDK_PATH=/path/to/pico-sdk cmake -S . -B build -G Ninja   # pico-sdk 2.3.0
ninja -C build            # -> build/firmware/OfficeSign.uf2
```

`PICO_BOARD` is fixed to `pico2_w` in the top-level `CMakeLists.txt`; configuring for a board without
a CYW43 chip fails on purpose. The top level builds only `PicoLEDs/src` (the library), not the
PicoLEDs examples. There's no bench hardware in an agent environment and no firmware test suite:
"verified" means it compiles cleanly for `pico2_w`.

## Build (laptop_app)

```sh
cd laptop_app
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
.venv/bin/python -m officesign.main   # serves http://127.0.0.1:8420
```

Requires Python 3.10+ (uses `X | None` union type hints); the system `python3` on macOS may be older
-- check for a newer Homebrew Python (`/opt/homebrew/bin/python3.13`) if venv creation fails oddly.
No automated tests; sanity-check with `python -m py_compile` across `officesign/` and by importing
`officesign.web.app` directly, since a missing route/model error surfaces at import time.

## Architecture

LED animation runs on core 0 (the usual PicoLEDs `Canvas`/`Animator` loop over a `PanelCanvas`); a
BTStack BLE GATT server runs entirely on core 1 (`BluetoothServer::run()`, launched via
`multicore_launch_core1`). The two cores talk through a mutex-protected, latest-message-wins `Mailbox`,
with the multicore FIFO used only as a "new message" doorbell (the payload itself is never pushed
through the FIFO). Messages are a fixed-size, hand-rolled little-endian binary protocol
(`SignMessage`/`WireFormat`) chosen specifically to avoid struct-padding mismatches between the
firmware's C++ and the laptop app's Python `struct` module -- mirror that byte-for-byte if you touch
the protocol, in both `firmware/SignMessage.h` and `laptop_app/officesign/protocol/`.

Two hardware gotchas:

- **PIO contention at startup**: the cyw43 (WiFi/BT) driver hard-claims a specific PIO1 state machine,
  while `Renderer::setup()` dynamically claims "any free" state machine. If core 0 calls
  `canvas.setup()` before core 1 finishes `cyw43_arch_init()`, the renderer can steal the SM cyw43
  needs. `BtReadySignal` (a dedicated hardware doorbell, separate from the `Mailbox`'s message doorbell)
  makes core 0 wait for core 1 to finish claiming its PIO resources first -- keep that ordering if you
  restructure `OfficeSign.cpp`.
- **RP2350 has no `hardware_rtc`**. Time on the Pico 2 W goes through `pico/aon_timer.h`
  (`aon_timer_set_time`/`aon_timer_get_time_calendar`, using `struct timespec`/`struct tm`, not
  `datetime_t`).

`laptop_app/officesign/` is the BLE central side: bleak (asyncio) for BLE, FastAPI/uvicorn for the
control web UI (chosen over Flask so route handlers can `await` BLE calls on the same event loop bleak
requires), NWS's free `api.weather.gov` API for weather (resolve the forecast gridpoint dynamically via
`/points/{lat},{lon}` rather than hardcoding one), and macOS EventKit (`pyobjc`) for calendar auto-fill.
