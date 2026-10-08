#include "ModeController.h"

#include "pico/aon_timer.h"
#include "pico/time.h"

ModeController::ModeController(Canvas* canvas, Font* font,
                               RandomAnimator* vibeAnimator, Mailbox* mailbox)
    : clockAnimation(canvas, font),
      meetingAnimation(canvas, font),
      darkAnimation(canvas),
      animator(canvas),
      vibeAnimator(vibeAnimator),
      mailbox(mailbox) {
    animator.add((int)SignMode::Clock, &clockAnimation);
    animator.add((int)SignMode::Meeting, &meetingAnimation);
    animator.add((int)SignMode::Dark, &darkAnimation);
    setMode(SignMode::Dark);
}

void ModeController::handleMessage(const SignMessage& msg) {
    printf("Received message of type %d\n", msg.type);
    switch (msg.type) {
        case MessageType::SetTime: {
            struct timespec ts;
            ts.tv_sec = (time_t)msg.localEpochSeconds;
            ts.tv_nsec = 0;
            aon_timer_set_time(&ts);
            break;
        }

        case MessageType::Weather: {
            WeatherInfo info;
            info.condition = msg.weatherCondition;
            info.currentTempF = msg.weatherCurrentTempF;
            info.highTempF = msg.weatherHighTempF;
            info.lowTempF = msg.weatherLowTempF;
            info.valid = true;
            clockAnimation.setWeather(info);
            break;
        }

        case MessageType::ClockMode:
            clockAnimation.setShowWeather(msg.clockShowWeather);
            setMode(SignMode::Clock);
            break;

        case MessageType::MeetingMode:
            meetingAnimation.configure(
                msg.meetingDurationMinutes, msg.meetingParticipantName,
                msg.meetingTzOffsetMinutes, msg.meetingStyle);
            meetingEndTimeUs =
                time_us_64() +
                (uint64_t)msg.meetingDurationMinutes * 60ull * 1000000ull;
            setMode(SignMode::Meeting);
            break;

        case MessageType::VibeMode:
            setMode(SignMode::Vibe);
            break;

        case MessageType::GoDark:
            setMode(SignMode::Dark);
            break;
    }
}

void ModeController::step() {
    if (mode == SignMode::Meeting && time_us_64() >= meetingEndTimeUs) {
        // The meeting's stated duration has elapsed with no follow-up
        // message; fall back to the clock rather than sitting on stale
        // meeting info indefinitely.
        setMode(SignMode::Clock);
    }

    if (mode == SignMode::Vibe && !mailbox->isConnected() &&
        mailbox->microsSinceDisconnected() >= kVibeFallbackAfterDisconnectUs) {
        // Nobody's been around to change it for a while; fall back to the
        // clock rather than looping Vibe Mode's animations indefinitely with
        // no laptop attached. Deliberately doesn't touch Dark Mode -- "Go
        // Dark" is an explicit instruction, and a BLE hiccup (or the laptop
        // just being asleep) shouldn't quietly turn the lights back on.
        printf(
            "Vibe Mode has been running unattended for %llu seconds; falling "
            "back to Clock Mode\n",
            mailbox->microsSinceDisconnected() / 1000000ull);
        setMode(SignMode::Clock);
    }

    switch (mode) {
        case SignMode::Vibe:
            vibeAnimator->step();
            break;
        default:
            animator.step();
            break;
    }
}

void ModeController::setMode(SignMode newMode) {
    if (newMode == mode) {
        return;
    }
    mode = newMode;
    animator.set((int)mode);
    if (mode == SignMode::Vibe) {
        vibeAnimator->init();
    }
}
