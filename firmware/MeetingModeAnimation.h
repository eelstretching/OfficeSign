#ifndef MEETINGMODEANIMATION_H
#define MEETINGMODEANIMATION_H

#pragma once

#include "Animation.h"
#include "SignMessage.h"
#include "Font.h"

/// @brief The sign's "Meeting Mode" display: who the meeting is with, how
/// long it runs, and their computed local time (the sign's own displayed
/// time plus the meeting's relative timezone offset). Serious meetings get a
/// steady color treatment; Fun meetings get a cycling accent color on the
/// participant's name. step() always returns true -- ModeController is
/// responsible for switching back to Clock Mode once the meeting's duration
/// elapses.
class MeetingModeAnimation : public Animation {
   public:
    MeetingModeAnimation(Canvas* canvas, Font* font);

    void init() override;
    bool step() override;

    void configure(uint16_t newDurationMinutes, const char* newParticipantName,
                    int16_t newTzOffsetMinutes, MeetingStyle newStyle);

   private:
    Font* font;
    char participantName[kParticipantNameMaxLen + 1] = {};
    uint16_t durationMinutes = 0;
    int16_t tzOffsetMinutes = 0;
    MeetingStyle style = MeetingStyle::Serious;

    uint64_t lastRenderUs = 0;
    uint32_t frame = 0;

    char durationBuf[24];
    char theirTimeBuf[24];

    void render();
    RGB accentColor() const;
};

#endif
