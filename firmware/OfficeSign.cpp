#include <stdlib.h>

#include "6x10_font.h"
#include "ArrayColorMap.h"
#include "BDFFont.h"
#include "BluetoothServer.h"
#include "BtReadySignal.h"
#include "Direction.h"
#include "FireworkWipe.h"
#include "Mailbox.h"
#include "ModeController.h"
#include "PacWipe.h"
#include "Panel.h"
#include "PanelCanvas.h"
#include "RainbowWipe.h"
#include "RandomAnimator.h"
#include "ScrollWipe.h"
#include "pico/aon_timer.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include <Marquees.h>
#include <Icicles.h>

// TODO: placeholder hardware configuration, copied from PanelTest's pattern.
// Update these once the physical OfficeSign's panel wiring/grid layout is
// finalized. The sign is built from Panels (2D zig-zag-wired sub-strips),
// one per pin, arranged in a PANELS_X x PANELS_Y grid.
#define PANEL_WIDTH 32
#define PANEL_HEIGHT 8
#define PANELS_X 4
#define PANELS_Y 4
#define NUM_PANELS (PANELS_X * PANELS_Y)
#define START_PIN 2

static Mailbox gMailbox;

// cyw43 + all of BTStack (HCI/L2CAP/SM/ATT) plus our own packet handlers run
// on core 1's stack, and need more than the 4KB the default
// multicore_launch_core1() stack is limited to (it lives in SCRATCH_X, a
// fixed-size SRAM bank -- PICO_CORE1_STACK_SIZE can't push it past that).
// Give core 1 its own stack in regular SRAM instead, where it isn't
// bank-limited.
alignas(8) static uint32_t core1Stack[2048];  // 8 KB

static void core1Entry() {
    BluetoothServer server(&gMailbox);
    server.run();  // never returns
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    // Seed the always-on timer with whatever gettimeofday() returns (likely
    // the epoch on a cold boot) so reads are well-defined before the laptop
    // app sends its first SET_TIME message.
    aon_timer_start_with_timeofday();

    // Core 1 brings up BTStack, which persists its link keys/device DB to
    // flash (see NVM_NUM_LINK_KEYS etc. in btstack_config.h) via
    // flash_safe_execute(). That call needs the *other* core (us) to already
    // be registered as a lockout victim, or it asserts deep in pico_flash
    // the first time BTStack touches flash -- register before core 1 ever
    // gets a chance to.
    flash_safe_execute_core_init();

    // Core 1 will bring up cyw43/BTStack, which needs to claim a specific
    // PIO1 state machine. We must not let the LED renderer's "claim any free
    // state machine" logic race that, so core 0 waits here until core 1
    // signals it's done.
    BtReadySignal::claim();
    multicore_launch_core1_with_stack(core1Entry, core1Stack, sizeof(core1Stack));
    BtReadySignal::waitForReady();

    // Build the panel grid for the sign. Each Panel is one pin's worth of
    // zig-zag-wired LEDs forming a PANEL_WIDTH x PANEL_HEIGHT block; the
    // PanelCanvas arranges them into one PANELS_X x PANELS_Y addressable
    // surface. See PicoLEDs/examples/PanelTest/PanelTest.cpp for the reference usage.
    Panel* panels[NUM_PANELS];
    for (int i = 0; i < NUM_PANELS; i++) {
        panels[i] = new Panel(START_PIN + i, PANEL_WIDTH, PANEL_HEIGHT);
    }

    PanelCanvas canvas(PANEL_WIDTH, PANEL_HEIGHT, PANELS_X, PANELS_Y);
    for (int y = 0; y < PANELS_Y; y++) {
        for (int x = 0; x < PANELS_X; x++) {
            canvas.addPanel(panels[y * PANELS_X + x], x, y);
        }
    }
    canvas.setup();
    canvas.setBrightness(16);
    canvas.clear();
    canvas.show();

    BDFFont font(font_6x10);

    font.render(&canvas, "Secret Collect!", 0, 3, RGB::Green);
    canvas.show();
    sleep_ms(2000);

    // Vibe Mode: a reasonable starting playlist built from existing
    // animation building blocks, following the same pattern CampSign uses
    // for its transition wipes. Easy to extend/edit once it's running.
    ArrayColorMap vibeColors({RGB::Red, RGB::Orange, RGB::Yellow, RGB::Green,
                              RGB::Blue, RGB::Indigo, RGB::Violet, RGB::Gold});
    uint8_t rgbwgColors[] = {0, 1, 2, 3, 4};

    RandomAnimator vibeAnimator(&canvas, 30);

    Marquees fancyMarq(&canvas, &vibeColors, 5, rgbwgColors, 20, RIGHT,
                       canvas.getHeight());
    fancyMarq.setName("FMarq");
    fancyMarq.setFPS(40);
    vibeAnimator.addTimed(&fancyMarq, 20000);

    uint8_t rgColors[] = {3, 4};
    Marquees rgMarq(&canvas, &vibeColors, 2, rgColors, 25, RIGHT,
                    canvas.getHeight());
    rgMarq.setName("RGMarq");
    vibeAnimator.addTimed(&rgMarq, 30000);

    ArrayColorMap icicleMap(8);
    Icicles icicles(&canvas, &icicleMap, 6, 6, RGB(0, 255, 0));
    icicles.setName("Icicles");
    vibeAnimator.addTimed(&icicles, 20000);

    // ModeController boots into SignMode::Dark itself (see ModeController.h)
    // and clears the canvas as part of construction, so the sign starts dark
    // until the laptop app sends a mode-switching message.
    ModeController controller(&canvas, &font, &vibeAnimator, &gMailbox);

    while (true) {
        SignMessage msg;
        if (gMailbox.take(msg)) {
            controller.handleMessage(msg);
        }
        controller.step();
    }
}
