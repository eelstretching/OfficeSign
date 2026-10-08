"""Reads the next upcoming meeting from the Mac's local Calendar app via
EventKit (pyobjc), so Meeting Mode's fields can be auto-filled. EventKit
surfaces whatever calendars are already synced into Apple Calendar
(Google/Exchange/iCloud alike) with no separate OAuth setup -- it just needs
a one-time "this app would like to access your Calendar" permission grant
(System Settings -> Privacy & Security -> Calendars).
"""

import logging
import threading
from datetime import datetime, timedelta

from EventKit import EKEntityTypeEvent, EKEventStore

logger = logging.getLogger(__name__)

_store = EKEventStore.alloc().init()


def _request_access(timeout: float = 30.0) -> bool:
    done = threading.Event()
    granted = {"ok": False}

    def _completion(ok, error):
        granted["ok"] = bool(ok)
        done.set()

    _store.requestAccessToEntityType_completion_(EKEntityTypeEvent, _completion)
    if not done.wait(timeout=timeout):
        logger.warning("Timed out waiting for Calendar access grant")
        return False
    return granted["ok"]


def next_meeting(window_hours: int = 12) -> dict:
    """Returns {"found": False} or {"found": True, "title": str,
    "participant_name": str, "duration_minutes": int}.
    """
    if not _request_access():
        return {"found": False, "reason": "calendar_access_denied"}

    now = datetime.now()
    end = now + timedelta(hours=window_hours)
    predicate = _store.predicateForEventsWithStartDate_endDate_calendars_(now, end, None)
    events = _store.eventsMatchingPredicate_(predicate)
    if not events:
        return {"found": False}

    event = min(events, key=lambda e: e.startDate())
    duration_minutes = max(1, round((event.endDate() - event.startDate()) / 60))

    participant_name = event.title() or "Meeting"
    for attendee in event.attendees() or []:
        if not attendee.isCurrentUser():
            participant_name = attendee.name() or participant_name
            break

    return {
        "found": True,
        "title": event.title(),
        "participant_name": participant_name,
        "duration_minutes": int(duration_minutes),
    }
