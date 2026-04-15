from __future__ import annotations

import asyncio
import time

from .state import ServerState
from .telemetry import TELEMETRY

try:
    import psutil
except ImportError:
    psutil = None


async def hardware_watcher(state: ServerState) -> None:
    if psutil is None:
        print("psutil not installed. Hardware watcher disabled.")
        return

    net_io = psutil.net_io_counters()
    last_sent = net_io.bytes_sent
    last_recv = net_io.bytes_recv
    last_tick = time.monotonic()
    psutil.cpu_percent(interval=None)

    while True:
        await asyncio.sleep(1.0)

        now = time.monotonic()
        dt = now - last_tick
        if dt <= 0:
            dt = 1.0
        last_tick = now

        cpu = int(psutil.cpu_percent(interval=None))
        ram = int(psutil.virtual_memory().percent)

        new_io = psutil.net_io_counters()
        up_kbps = int(max(0.0, (new_io.bytes_sent - last_sent) / 1024.0 / dt))
        down_kbps = int(max(0.0, (new_io.bytes_recv - last_recv) / 1024.0 / dt))
        last_sent = new_io.bytes_sent
        last_recv = new_io.bytes_recv

        cpu = max(0, min(100, cpu))
        ram = max(0, min(100, ram))

        if state.esp32_ws is None or state.esp32_current_page != 6:
            continue

        stats_msg = f"STATS:{cpu},{ram},{up_kbps},{down_kbps}"
        try:
            await state.esp32_ws.send_text(stats_msg)
            TELEMETRY.inc("ws_messages_tx")
            TELEMETRY.inc("hardware_stats_packets_sent")
        except Exception:
            TELEMETRY.inc("ws_send_failures")


async def send_page_with_retry(state: ServerState, page_id: int, retries: int = 4, ack_timeout: float = 0.35):
    if state.esp32_ws is None:
        return False, "esp-disconnected"

    if state.esp32_current_page == page_id:
        return True, "already-on-page"

    for attempt in range(1, retries + 1):
        ws = state.esp32_ws
        if ws is None:
            return False, "esp-disconnected"

        try:
            await ws.send_text(f"PAGE,{page_id}")
            TELEMETRY.inc("ws_messages_tx")
        except Exception:
            TELEMETRY.inc("ws_send_failures")
            return False, "send-failed"

        deadline = time.monotonic() + ack_timeout
        while time.monotonic() < deadline:
            if state.esp32_current_page == page_id:
                return True, f"ack-attempt-{attempt}"
            await asyncio.sleep(0.03)

    if state.esp32_current_page == page_id:
        return True, "ack-after-timeout"
    return False, "ack-timeout"
