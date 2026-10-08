#ifndef MODECONTROLLER_H
#define MODECONTROLLER_H

#pragma once

#include "Animator.h"
#include "MapAnimator.h"
#include "ClockWeatherAnimation.h"
#include "DarkAnimation.h"
#include "Mailbox.h"
#include "MeetingModeAnimation.h"
#include "RandomAnimator.h"
#include "SignMessage.h"

enum class SignMode : uint8_t { Clock = 0, Meeting = 1, Vibe = 2, Dark = 3 };

/// @brief Owns one Animator per sign mode (Clock/Meeting/Dark) plus a
/// reference to the Vibe mode's RandomAnimator (built by main() from
/// existing animation building blocks), and switches which one gets step()'d
/// each frame in response to messages pulled from the Mailbox. Reuses the
/// existing Animator/Animation framework unchanged.
class ModeController {
   public:
    ModeController(Canvas* canvas, Font* font, RandomAnimator* vibeAnimator, Mailbox* mailbox);

    /// @brief Dispatches a decoded message: sets the RTC, caches weather,
    /// configures a mode's animation, and/or switches modes.
    void handleMessage(const SignMessage& msg);

    /// @brief Steps whichever mode is currently active. Call once per
    /// main-loop iteration.
    void step();

    void setMode(SignMode newMode);

   private:
    SignMode mode = SignMode::Dark;

    ClockWeatherAnimation clockAnimation;
    MeetingModeAnimation meetingAnimation;
    DarkAnimation darkAnimation;

    MapAnimator animator;

    RandomAnimator* vibeAnimator;

    Mailbox* mailbox;

    uint64_t meetingEndTimeUs = 0;

    // How long Vibe Mode runs unattended before falling back to the clock.
    // Comfortably outlasts the laptop app's own reconnect backoff (capped at
    // 30s) so a brief BLE blip doesn't cause a visible flicker back and
    // forth -- this only kicks in for a genuinely extended absence.
    static constexpr uint64_t kVibeFallbackAfterDisconnectUs = 60ull * 1000000ull;
};

#endif
