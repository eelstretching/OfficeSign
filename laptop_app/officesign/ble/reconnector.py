"""Keeps the sign connected across drops -- laptop sleep (lid closed while
walking to a meeting), walking briefly out of BLE range, the sign itself
losing power -- without anyone having to notice and hit reconnect by hand.

Runs as a background asyncio task for the lifetime of the web server. Waits
on SignClient.disconnected_event rather than polling, so it reacts the
moment bleak notices a drop (or immediately at startup, since the event
starts set).
"""

import asyncio
import logging
from typing import TYPE_CHECKING

from ..protocol import messages

if TYPE_CHECKING:
    from ..orchestrator import AppState

logger = logging.getLogger(__name__)

MIN_RETRY_DELAY_S = 3.0
MAX_RETRY_DELAY_S = 30.0


async def keep_connected(state: "AppState") -> None:
    delay = MIN_RETRY_DELAY_S
    while True:
        await state.sign.disconnected_event.wait()

        try:
            await state.sign.connect()
        except Exception as exc:
            logger.warning("Reconnect attempt failed (%s); retrying in %.0fs", exc, delay)
            await asyncio.sleep(delay)
            delay = min(delay * 2, MAX_RETRY_DELAY_S)
            continue

        delay = MIN_RETRY_DELAY_S

        # Refresh clock + cached weather on every (re)connect -- cheap, and
        # covers both "the laptop was asleep for hours so the sign's clock
        # has drifted" and "the sign itself power-cycled and lost its clock
        # entirely". Deliberately *not* resending Clock Mode here: there's
        # no status characteristic (v1 protocol is write-only, see the plan
        # notes), so the laptop has no way to know whether the sign is still
        # showing Vibe/Meeting/Dark mode or came back up in its power-on
        # default -- forcing Clock Mode on every reconnect would be right in
        # the "sign rebooted" case but wrong (and disruptive) in the far more
        # common "laptop's the one that came and went" case. Easy to flip if
        # that tradeoff turns out to feel wrong in practice.
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
        except Exception:
            logger.exception("Reconnected, but failed to refresh the sign's clock/weather")
