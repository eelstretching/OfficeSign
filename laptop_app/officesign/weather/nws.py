"""Weather for Burlington, MA via the free, no-API-key National Weather
Service API.

server/server.py's existing code hardcodes the forecast gridpoint
(BOX/54,48); that value has since gone stale -- NWS re-grids office coverage
over time, and it now 404s. We instead resolve the current gridpoint at
request time via the /points/{lat},{lon} endpoint, so this can't silently
go stale the same way.

The /forecast endpoint only gives period forecasts (today/tonight/etc), no
live observation -- so "current temperature" there would just duplicate the
day's forecast high, which is wrong outside midday. We instead pull a true
current reading from the nearest observation station (KBED, Hanscom Field)
and keep /forecast only for the high/low.
"""

import logging

import requests

from ..protocol.constants import WeatherCondition

# Burlington, MA.
LATITUDE = 42.5048
LONGITUDE = -71.1956

POINTS_URL = f"https://api.weather.gov/points/{LATITUDE},{LONGITUDE}"
CURRENT_OBSERVATION_URL = "https://api.weather.gov/stations/KBED/observations/latest"

# A NWS API client is expected to send a User-Agent identifying the app.
HEADERS = {"User-Agent": "OfficeSign (github.com/eelstretching/OfficeSign)"}

logger = logging.getLogger(__name__)


def _map_condition(short_forecast: str) -> WeatherCondition:
    s = short_forecast.lower()
    if "snow" in s:
        return WeatherCondition.SNOW
    if "rain" in s or "shower" in s or "storm" in s or "drizzle" in s:
        return WeatherCondition.RAIN
    if "partly" in s or "mostly cloudy" in s:
        return WeatherCondition.PARTLY_CLOUDY
    if "cloudy" in s or "overcast" in s:
        return WeatherCondition.CLOUDY
    return WeatherCondition.SUNNY  # "Sunny", "Clear", "Mostly Sunny", fallback


def _celsius_to_fahrenheit(celsius: float) -> int:
    return round(celsius * 9 / 5 + 32)


def fetch_burlington_weather() -> dict:
    """Returns {"condition": WeatherCondition, "current_f": int, "high_f":
    int, "low_f": int}.
    """
    points_resp = requests.get(POINTS_URL, headers=HEADERS, timeout=10)
    points_resp.raise_for_status()
    forecast_url = points_resp.json()["properties"]["forecast"]

    forecast_resp = requests.get(forecast_url, headers=HEADERS, timeout=10)
    forecast_resp.raise_for_status()
    periods = forecast_resp.json()["properties"]["periods"]

    # NWS's periods list always starts with whichever period is current --
    # that's "Tonight" (isDaytime=False) if it's already dark out, not
    # necessarily a daytime period. Pick the next upcoming day/night period
    # by their isDaytime flag rather than assuming fixed indices, or a
    # request made at night ends up with high_f/low_f swapped.
    high_period = next(p for p in periods if p["isDaytime"])
    low_period = next(p for p in periods if not p["isDaytime"])

    condition = _map_condition(periods[0]["shortForecast"])
    high_f = high_period["temperature"]
    low_f = low_period["temperature"]

    current_f = high_f  # fallback if the observation station is unreachable
    try:
        obs_resp = requests.get(CURRENT_OBSERVATION_URL, headers=HEADERS, timeout=10)
        obs_resp.raise_for_status()
        temp_c = obs_resp.json()["properties"]["temperature"]["value"]
        if temp_c is not None:
            current_f = _celsius_to_fahrenheit(temp_c)
    except (requests.RequestException, KeyError, TypeError) as exc:
        logger.warning("Couldn't fetch current observation from %s: %s", CURRENT_OBSERVATION_URL, exc)

    return {
        "condition": condition,
        "current_f": current_f,
        "high_f": high_f,
        "low_f": low_f,
    }
