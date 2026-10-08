#ifndef SIGNMESSAGE_H
#define SIGNMESSAGE_H

#pragma once

#include <stdint.h>

/// @brief The message types the laptop app can send to the sign. Values are
/// the wire-format type byte and must stay in sync with
/// laptop_app/officesign/protocol/constants.py.
enum class MessageType : uint8_t {
    SetTime = 0x01,
    MeetingMode = 0x02,
    Weather = 0x03,
    ClockMode = 0x04,
    VibeMode = 0x05,
    GoDark = 0x06,
};

enum class WeatherCondition : uint8_t {
    Sunny = 0,
    Cloudy = 1,
    PartlyCloudy = 2,
    Rain = 3,
    Snow = 4,
};

enum class MeetingStyle : uint8_t {
    Serious = 0,
    Fun = 1,
};

/// @brief Maximum length, in bytes, of a meeting participant's name as carried
/// on the wire (see MEETING_MODE layout below).
static constexpr uint16_t kParticipantNameMaxLen = 14;

/// @brief A decoded message from the laptop app. This is a single
/// trivially-copyable, tagged class rather than a polymorphic hierarchy: the
/// inter-core Mailbox copies "the current message" by value under a mutex,
/// and with only six tiny fixed-shape messages there's no benefit to
/// heap-allocated, virtual-dispatch message objects.
///
/// Wire format: one BLE write to the control characteristic, little-endian,
/// dispatched on a leading type byte. Every message fits in <= 20 bytes so it
/// never needs BLE MTU negotiation.
///
///   SET_TIME     (0x01, 9 bytes):  type:u8, localEpochSeconds:i64
///   MEETING_MODE (0x02, 20 bytes): type:u8, durationMinutes:u16,
///                                  tzOffsetMinutes:i16, style:u8,
///                                  participantName:14 raw bytes (zero-padded)
///   WEATHER      (0x03, 8 bytes):  type:u8, condition:u8, currentTempF:i16,
///                                  highTempF:i16, lowTempF:i16
///   CLOCK_MODE   (0x04, 2 bytes):  type:u8, showWeather:u8
///   VIBE_MODE    (0x05, 1 byte):   type:u8
///   GO_DARK      (0x06, 1 byte):   type:u8
class SignMessage {
   public:
    MessageType type = MessageType::GoDark;

    // SET_TIME. The sign's own local wall-clock time, reinterpreted as
    // seconds-since-epoch (the laptop sends datetime.now() as if it were
    // UTC) -- deliberately *not* a true UTC Unix timestamp. This means the
    // firmware never has to know its own timezone or DST rules: it just
    // displays whatever it's told, and MeetingMode's tzOffsetMinutes is a
    // simple relative delta added on top of that displayed time.
    int64_t localEpochSeconds = 0;

    // MEETING_MODE
    uint16_t meetingDurationMinutes = 0;
    int16_t meetingTzOffsetMinutes = 0;
    MeetingStyle meetingStyle = MeetingStyle::Serious;
    char meetingParticipantName[kParticipantNameMaxLen + 1] = {};  // NUL-terminated after decode

    // WEATHER
    WeatherCondition weatherCondition = WeatherCondition::Sunny;
    int16_t weatherCurrentTempF = 0;
    int16_t weatherHighTempF = 0;
    int16_t weatherLowTempF = 0;

    // CLOCK_MODE
    bool clockShowWeather = true;

    /// @brief Decodes a raw ATT-write buffer into out. Returns false on
    /// truncated or unrecognized input; out is left untouched on failure.
    static bool decode(const uint8_t* data, uint16_t len, SignMessage& out);
};

#endif
