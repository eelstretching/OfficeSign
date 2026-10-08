"""Entry point: python -m officesign.main

Starts the local control web server. FastAPI's lifespan hook (see web/app.py)
runs the startup sequence -- fetch weather, connect to the sign, seed it with
time/weather/clock mode -- before the server starts accepting requests.
"""

import uvicorn


def main() -> None:
    uvicorn.run("officesign.web.app:app", host="127.0.0.1", port=8420, log_level="info")


if __name__ == "__main__":
    main()
