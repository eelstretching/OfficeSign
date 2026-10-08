# OfficeSign

A wall-mounted LED panel sign driven by a Raspberry Pi Pico 2 W, controlled from a MacBook over
Bluetooth LE.

- `firmware/` is the Pico code. Core 0 runs the LED animations, core 1 runs a BTStack BLE server.
- `laptop_app/` is the Python app on the laptop. It connects to the sign, sends it the time, weather
  and meeting info, and serves a small control page on http://127.0.0.1:8420.
- `PicoLEDs/` is the LED library, pulled in as a git submodule that tracks PicoLEDs `main`.
- `docs/sign-project.md` is the original description of the project.

This moved here from the `office-sign` branch of PicoLEDs (`examples/OfficeSign` and `laptop_app`).

## Getting the code

The submodule has to be fetched too, so clone with `--recursive`:

```sh
git clone --recursive https://github.com/eelstretching/OfficeSign.git
```

If you already cloned without it, run this inside the checkout:

```sh
git submodule update --init
```

## Building the firmware

Needs pico-sdk 2.3.0 (with its tinyusb, cyw43-driver, lwip, btstack and mbedtls submodules) and the
ARM toolchain. The board is fixed to `pico2_w`.

```sh
PICO_SDK_PATH=/path/to/pico-sdk cmake -S . -B build -G Ninja
ninja -C build
```

The output is `build/firmware/OfficeSign.uf2`. The Pico VS Code extension works too: open this folder
and build as usual.

## Running the laptop app

```sh
cd laptop_app
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
.venv/bin/python -m officesign.main
```

Needs Python 3.10 or newer.

## Picking up newer PicoLEDs changes

The submodule is pinned to a specific PicoLEDs commit. To move it to the latest PicoLEDs `main`:

```sh
git submodule update --remote PicoLEDs
git add PicoLEDs
git commit -m "Update PicoLEDs"
git push
```

`git status` shows `modified: PicoLEDs (new commits)` when the pinned commit differs from what is
checked out. Running `git submodule update` (without `--remote`) puts it back to the pinned one.
