"""Wire-format constants shared with the firmware.

Must stay in sync with firmware/SignMessage.h in this repo -- these are the same enums, same integer values.
"""

from enum import IntEnum


class MessageType(IntEnum):
    SET_TIME = 0x01
    MEETING_MODE = 0x02
    WEATHER = 0x03
    CLOCK_MODE = 0x04
    VIBE_MODE = 0x05
    GO_DARK = 0x06


class WeatherCondition(IntEnum):
    SUNNY = 0
    CLOUDY = 1
    PARTLY_CLOUDY = 2
    RAIN = 3
    SNOW = 4


class MeetingStyle(IntEnum):
    SERIOUS = 0
    FUN = 1


# Max length, in bytes, of a meeting participant's name as carried on the
# wire (see SignMessage.h's MEETING_MODE layout).
PARTICIPANT_NAME_MAX_LEN = 14

# The sign's BLE GATT service/characteristic UUIDs (see OfficeSign.gatt).
#
# Lowercase deliberately: bleak's CoreBluetooth backend always normalizes
# advertised service UUIDs to lowercase (see cb_uuid_to_str), so an uppercase
# constant here would silently never match during scanning.
SERVICE_UUID = "20b1c720-1391-4410-9f7f-7ece217f621c"
CONTROL_CHARACTERISTIC_UUID = "d353ee31-9eab-4fa5-bff2-258667f6d295"
