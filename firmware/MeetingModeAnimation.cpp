#include "MeetingModeAnimation.h"

#include <string.h>
#include <time.h>

#include "pico/aon_timer.h"
#include "pico/printf.h"
#include "pico/time.h"

#define S_IN_US 1000000ull

MeetingModeAnimation::MeetingModeAnimation(Canvas* canvas, Font* font)
    : Animation(canvas, nullptr), font(font) {
    fps = 10;  // smooth enough for the Fun-mode color cycle
}

void MeetingModeAnimation::configure(uint16_t newDurationMinutes, const char* newParticipantName,
                                      int16_t newTzOffsetMinutes, MeetingStyle newStyle) {
    durationMinutes = newDurationMinutes;
    tzOffsetMinutes = newTzOffsetMinutes;
    style = newStyle;
    strncpy(participantName, newParticipantName, kParticipantNameMaxLen);
    participantName[kParticipantNameMaxLen] = '\0';
}

void MeetingModeAnimation::init() {
    frame = 0;
    lastRenderUs = time_us_64();
    render();
}

bool MeetingModeAnimation::step() {
    frame++;
    uint64_t now = time_us_64();
    // Serious style only needs to redraw once/sec (the countdown-relevant
    // parts don't change faster than that); Fun style redraws every frame so
    // the accent color visibly cycles.
    bool dueForSecondTick = (now - lastRenderUs) >= S_IN_US;
    if (style == MeetingStyle::Fun || dueForSecondTick) {
        lastRenderUs = now;
        render();
    }
    return true;
}

RGB MeetingModeAnimation::accentColor() const {
    if (style == MeetingStyle::Fun) {
        RGB c;
        c.setHue((uint8_t)(frame * 3));
        return c;
    }
    return RGB::SteelBlue;
}

void MeetingModeAnimation::render() {
    struct timespec ts;
    aon_timer_get_time(&ts);
    time_t theirSeconds = ts.tv_sec + (time_t)tzOffsetMinutes * 60;
    struct tm theirTm = {};
    gmtime_r(&theirSeconds, &theirTm);

    snprintf(durationBuf, sizeof(durationBuf), "%d MIN", durationMinutes);
    snprintf(theirTimeBuf, sizeof(theirTimeBuf), "THEIRS %02d:%02d", theirTm.tm_hour,
             theirTm.tm_min);

    canvas->clear();
    font->render(canvas, participantName, 2, 16, accentColor());
    font->render(canvas, durationBuf, 2, 8, RGB::White);
    font->render(canvas, theirTimeBuf, 2, 0, RGB::White);
}
