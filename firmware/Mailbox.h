#ifndef MAILBOX_H
#define MAILBOX_H

#pragma once

#include <atomic>

#include "SignMessage.h"
#include "pico/mutex.h"

/// @brief Inter-core "latest message wins" mailbox. Core 1 (BluetoothServer)
/// is the producer, core 0 (the animation loop) is the consumer.
///
/// The shared SignMessage is protected by a mutex; a dedicated hardware
/// doorbell is used purely to tell core 0 a new message is waiting -- the
/// message payload itself never travels over the doorbell/FIFO.
///
/// This deliberately does NOT use the SIO inter-core FIFO for that signal
/// (an earlier version did). BTStack persists its link keys/device DB to
/// flash, which requires both cores to call flash_safe_execute_core_init()
/// -> multicore_lockout_victim_init() -- and that installs a FIFO-interrupt
/// handler on *each* core that unconditionally drains and discards anything
/// pushed to the FIFO that isn't its own internal lockout handshake value.
/// With that handler active, any value this class pushed to the FIFO was
/// getting silently eaten before take()'s polling loop ever saw it: publish()
/// still worked (the mutex-protected fields were fine), but take() could
/// never observe that a doorbell had rung, so messages piled up unread
/// forever. A dedicated doorbell (same mechanism BtReadySignal already uses
/// for the startup handshake) isn't touched by the lockout machinery at all.
///
/// Concurrency contract: this is latest-wins, not a queue. If two messages
/// are published before core 0 next calls take(), only the second is ever
/// seen -- the first is silently dropped. That's fine for this protocol:
/// every message type is either a one-shot mode switch or a data snapshot,
/// none need strict ordering/delivery guarantees against each other.
///
/// Also carries the sign's current BLE connection state -- separate from the
/// message-passing above, since it's continuously-updated status rather than
/// a discrete event. Plain atomics rather than the mutex: a single flag and
/// timestamp, read by ModeController as a soft/approximate check (see
/// ModeController::step()), not something that needs strict consistency.
class Mailbox {
   public:
    Mailbox();

    /// @brief Producer side (core 1). Publishes msg as the latest message,
    /// overwriting any not-yet-consumed one, then rings the doorbell.
    void publish(const SignMessage& msg);

    /// @brief Consumer side (core 0). Call once per frame. Non-blocking:
    /// checks (and clears) the doorbell and, if there's an unread message,
    /// copies it into out and returns true.
    bool take(SignMessage& out);

    /// @brief Producer side (core 1). Records whether the sign currently has
    /// a laptop connected over BLE.
    void setConnected(bool isConnected);

    /// @brief Consumer side (core 0). Whether a laptop is currently connected.
    bool isConnected() const { return connected; }

    /// @brief Consumer side (core 0). How long the sign has been
    /// disconnected, in microseconds. Meaningless (but harmless to call)
    /// while isConnected() is true.
    uint64_t microsSinceDisconnected() const;

   private:
    mutex_t mutex;
    SignMessage latest;
    bool hasUnread = false;

    std::atomic<bool> connected{false};
    std::atomic<uint64_t> disconnectedAtUs{0};

    // Claimed in the constructor -- gMailbox is a global, so this runs
    // during static init on core 0, well before core 1 is ever launched.
    int doorbellNum = -1;
};

#endif
