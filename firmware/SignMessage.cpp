#include "SignMessage.h"

#include "WireFormat.h"

bool SignMessage::decode(const uint8_t* data, uint16_t len, SignMessage& out) {
    if (len < 1) {
        return false;
    }

    ByteReader reader(data, len);
    MessageType type = (MessageType)reader.readU8();

    SignMessage msg;
    msg.type = type;

    switch (type) {
        case MessageType::SetTime:
            msg.localEpochSeconds = reader.readI64LE();
            break;

        case MessageType::MeetingMode: {
            msg.meetingDurationMinutes = reader.readU16LE();
            msg.meetingTzOffsetMinutes = reader.readI16LE();
            msg.meetingStyle = (MeetingStyle)reader.readU8();
            char name[kParticipantNameMaxLen];
            reader.readBytes(name, kParticipantNameMaxLen);
            for (uint16_t i = 0; i < kParticipantNameMaxLen; i++) {
                msg.meetingParticipantName[i] = name[i];
            }
            msg.meetingParticipantName[kParticipantNameMaxLen] = '\0';
            break;
        }

        case MessageType::Weather:
            msg.weatherCondition = (WeatherCondition)reader.readU8();
            msg.weatherCurrentTempF = reader.readI16LE();
            msg.weatherHighTempF = reader.readI16LE();
            msg.weatherLowTempF = reader.readI16LE();
            break;

        case MessageType::ClockMode:
            msg.clockShowWeather = reader.readU8() != 0;
            break;

        case MessageType::VibeMode:
        case MessageType::GoDark:
            // Type byte only, nothing further to decode.
            break;

        default:
            return false;
    }

    if (reader.hasFailed()) {
        return false;
    }

    out = msg;
    return true;
}
