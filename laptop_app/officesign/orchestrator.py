"""Startup sequence: fetch weather, connect to the sign, then seed it with
the current time/weather/clock mode. Runs once at web server startup.

Failures here are logged, not fatal -- the web UI stays usable so the user
can retry manually (e.g. if the sign wasn't powered on yet when the laptop
app started).
"""

import asyncio
import logging

from .ble.client import SignClient
from .protocol import messages
from .weather.nws import fetch_burlington_weather

logger = logging.getLogger(__name__)


class AppState:
    """Shared, mutable state the web routes read/update."""

    def __init__(self) -> None:
        self.sign = SignClient()
        self.last_weather: dict | None = None
        # Set once web/app.py's lifespan starts the background reconnect
        # loop (ble/reconnector.py), so it can be cancelled cleanly on
        # shutdown.
        self.reconnect_task: asyncio.Task | None = None


async def run_startup_sequence(state: AppState) -> None:
    try:
        state.last_weather = fetch_burlington_weather()
        logger.info("Fetched weather: %s", state.last_weather)
    except Exception:
        logger.exception("Failed to fetch weather at startup")

    try:
        await state.sign.connect()
    except Exception:
        logger.exception("Failed to connect to OfficeSign at startup")
        return

    try:
        await state.sign.send(messages.encode_set_time())
        if state.last_weather is not None:
            await state.sign.send(
                messages.encode_weather(
                    state.last_weather["condition"],
                    state.last_weather["current_f"],
                    state.last_weather["high_f"],
                    state.last_weather["low_f"],
                )
            )
        await state.sign.send(messages.encode_clock_mode(show_weather=state.last_weather is not None))
        logger.info("Startup sequence complete: sign set to Clock Mode")
    except Exception:
        logger.exception("Failed to seed OfficeSign with time/weather/clock mode at startup")
