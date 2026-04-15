from __future__ import annotations

import asyncio
import threading
from contextlib import asynccontextmanager
from typing import AsyncIterator

from fastapi import FastAPI

from .discovery import broadcast_presence
from .hardware import hardware_watcher
from .media import media_watcher
from .routes import register_routes
from .state import STATE


@asynccontextmanager
async def lifespan(_: FastAPI) -> AsyncIterator[None]:
    threading.Thread(target=broadcast_presence, daemon=True).start()
    media_task = asyncio.create_task(media_watcher(STATE))
    hardware_task = asyncio.create_task(hardware_watcher(STATE))

    try:
        yield
    finally:
        media_task.cancel()
        hardware_task.cancel()
        await asyncio.gather(media_task, hardware_task, return_exceptions=True)


app = FastAPI(lifespan=lifespan)
register_routes(app, STATE)
