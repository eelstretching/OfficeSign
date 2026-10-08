#ifndef BTREADYSIGNAL_H
#define BTREADYSIGNAL_H

#pragma once

/// @brief One-shot startup handshake between the two cores, used to avoid a
/// PIO state-machine claim race: the cyw43 driver needs a specific state
/// machine on PIO1 (CYW43_SPI_PIO), while Renderer::setup() dynamically
/// claims "any free" PIO/SM for the LED strips. If core 0 raced ahead and
/// called canvas.setup() before core 1 finished cyw43_arch_init(), the LED
/// renderer could grab the state machine cyw43 needs and Bluetooth init
/// would fail.
///
/// This uses a dedicated RP2350 hardware doorbell, kept separate from the
/// Mailbox's FIFO doorbell (which is reserved for ongoing message delivery).
class BtReadySignal {
   public:
    /// @brief Core 0, before multicore_launch_core1().
    static void claim();

    /// @brief Core 1, right after cyw43_arch_init() succeeds (before
    /// att_server_init()/advertising setup, which is where BTStack's own PIO
    /// use would happen if it needed any -- signal as early as cyw43's PIO
    /// claim is done).
    static void signalReady();

    /// @brief Core 0, before canvas.setup(). Busy-waits until core 1 signals,
    /// then clears the doorbell.
    static void waitForReady();

   private:
    static int doorbellNum;
};

#endif
