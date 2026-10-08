"""FastAPI control app for the OfficeSign.

FastAPI + uvicorn (not Flask) because bleak's BLE client is asyncio-only:
running everything -- HTTP handling and the BLE client -- on one event loop
means a route handler can just `await sign.send(...)` directly, instead of
marshalling calls across a thread boundary into a separate BLE event loop.
"""

import asyncio
import contextlib
import logging
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from pydantic import BaseModel
from starlette.requests import Request

from ..ble.reconnector import keep_connected
from ..calendar_source.eventkit_source import next_meeting
from ..orchestrator import AppState, run_startup_sequence
from ..protocol import messages
from ..protocol.constants import MeetingStyle, WeatherCondition

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

BASE_DIR = Path(__file__).resolve().parent
templates = Jinja2Templates(directory=str(BASE_DIR / "templates"))

state = AppState()


@asynccontextmanager
async def lifespan(_app: FastAPI):
    await run_startup_sequence(state)
    # Keeps the sign connected for the life of the process -- reconnects
    # automatically after laptop sleep, walking out of BLE range, or the
    # sign power-cycling, without anyone needing to notice and hit
    # /api/connect by hand. No-ops (just waits) if run_startup_sequence
    # already connected successfully.
    state.reconnect_task = asyncio.create_task(keep_connected(state))
    yield
    state.reconnect_task.cancel()
    with contextlib.suppress(asyncio.CancelledError):
        await state.reconnect_task
    await state.sign.disconnect()


app = FastAPI(title="OfficeSign", lifespan=lifespan)
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")


class MeetingModeRequest(BaseModel):
    duration_minutes: int
    participant_name: str
    tz_offset_minutes: int = 0
    style: MeetingStyle = MeetingStyle.SERIOUS


class WeatherRequest(BaseModel):
    condition: WeatherCondition
    current_f: int
    high_f: int
    low_f: int


class ClockModeRequest(BaseModel):
    show_weather: bool = True


async def _send_or_502(payload: bytes) -> dict:
    try:
        await state.sign.send(payload)
    except Exception as exc:
        raise HTTPException(status_code=502, detail=str(exc)) from exc
    return {"ok": True}


@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse(request, "index.html")


@app.get("/api/status")
async def api_status():
    return {"connected": state.sign.is_connected, "last_weather": state.last_weather}


@app.post("/api/connect")
async def api_connect():
    try:
        await state.sign.connect()
    except Exception as exc:
        raise HTTPException(status_code=502, detail=str(exc)) from exc
    return {"ok": True}


@app.post("/api/messages/set-time")
async def api_set_time():
    return await _send_or_502(messages.encode_set_time())


@app.post("/api/messages/meeting-mode")
async def api_meeting_mode(body: MeetingModeRequest):
    return await _send_or_502(
        messages.encode_meeting_mode(
            body.duration_minutes, body.participant_name, body.tz_offset_minutes, body.style
        )
    )


@app.post("/api/messages/weather")
async def api_weather(body: WeatherRequest):
    payload = messages.encode_weather(body.condition, body.current_f, body.high_f, body.low_f)
    result = await _send_or_502(payload)
    state.last_weather = body.model_dump()
    return result


@app.post("/api/messages/clock-mode")
async def api_clock_mode(body: ClockModeRequest):
    return await _send_or_502(messages.encode_clock_mode(body.show_weather))


@app.post("/api/messages/vibe-mode")
async def api_vibe_mode():
    return await _send_or_502(messages.encode_vibe_mode())


@app.post("/api/messages/go-dark")
async def api_go_dark():
    return await _send_or_502(messages.encode_go_dark())


@app.get("/api/calendar/next-meeting")
async def api_next_meeting():
    return next_meeting()
