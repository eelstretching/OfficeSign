"""Binary message encoders matching firmware/SignMessage.h exactly.

Every message is a single little-endian, fixed-size BLE write: a leading type
byte followed by that message's payload. See SignMessage.h's doc comment for
the full byte layout of each message.
"""

import struct
from datetime import datetime, timezone

from .constants import PARTICIPANT_NAME_MAX_LEN, MeetingStyle, MessageType, WeatherCondition


def encode_set_time(when: datetime | None = None) -> bytes:
    """The sign's own local wall-clock time, reinterpreted as seconds-since-
    epoch (i.e. we take the local time and treat it as if it were UTC) --
    deliberately *not* a true UTC Unix timestamp. This lets the firmware
    display exactly what it's told with zero timezone/DST logic on-device.
    """
    when = when or datetime.now()
    local_epoch_seconds = int(when.replace(tzinfo=timezone.utc).timestamp())
    return struct.pack("<Bq", MessageType.SET_TIME, local_epoch_seconds)


def encode_meeting_mode(
    duration_minutes: int,
    participant_name: str,
    tz_offset_minutes: int,
    style: MeetingStyle,
) -> bytes:
    name_bytes = participant_name.encode("utf-8")[:PARTICIPANT_NAME_MAX_LEN]
    name_bytes = name_bytes.ljust(PARTICIPANT_NAME_MAX_LEN, b"\x00")
    return struct.pack(
        "<BHhB14s",
        MessageType.MEETING_MODE,
        duration_minutes,
        tz_offset_minutes,
        int(style),
        name_bytes,
    )


def encode_weather(
    condition: WeatherCondition, current_temp_f: int, high_temp_f: int, low_temp_f: int
) -> bytes:
    return struct.pack(
        "<BBhhh", MessageType.WEATHER, int(condition), current_temp_f, high_temp_f, low_temp_f
    )


def encode_clock_mode(show_weather: bool = True) -> bytes:
    return struct.pack("<BB", MessageType.CLOCK_MODE, 1 if show_weather else 0)


def encode_vibe_mode() -> bytes:
    return struct.pack("<B", MessageType.VIBE_MODE)


def encode_go_dark() -> bytes:
    return struct.pack("<B", MessageType.GO_DARK)
