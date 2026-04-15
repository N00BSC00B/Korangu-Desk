from __future__ import annotations

import asyncio
from dataclasses import dataclass, field
from typing import Optional

from fastapi import WebSocket


@dataclass
class ServerState:
    esp32_ws: Optional[WebSocket] = None
    esp32_current_page: int = -1
    page_change_lock: asyncio.Lock = field(default_factory=asyncio.Lock)


STATE = ServerState()
