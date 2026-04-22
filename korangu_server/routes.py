from __future__ import annotations

import asyncio

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from pydantic import BaseModel, Field

from .config import DISCOVERY_PORT, SERVER_PORT
from .hardware import send_page_with_retry
from .media_screen import (
    DEFAULT_MAX_FRAMES,
    DEFAULT_OFFSET_X,
    DEFAULT_OFFSET_Y,
    DEFAULT_ROTATION_DEG,
    DEFAULT_STREAM_FPS,
    DEFAULT_ZOOM,
    MEDIA_SCREEN,
    get_media_status,
    prepare_media_clip,
)
from .state import ServerState
from .telemetry import TELEMETRY


class MediaSetRequest(BaseModel):
    path: str = Field(..., min_length=1)
    fps: float = Field(default=DEFAULT_STREAM_FPS, ge=1.0, le=12.0)
    dither: bool = True
    threshold: int = Field(default=128, ge=1, le=254)
    loop: bool = True
    max_frames: int = Field(default=DEFAULT_MAX_FRAMES, ge=1, le=1200)
    rotation: float = Field(default=DEFAULT_ROTATION_DEG, ge=-180.0, le=180.0)
    zoom: float = Field(default=DEFAULT_ZOOM, ge=0.25, le=4.0)
    offset_x: int = Field(default=DEFAULT_OFFSET_X, ge=-256, le=256)
    offset_y: int = Field(default=DEFAULT_OFFSET_Y, ge=-128, le=128)


def register_routes(app: FastAPI, state: ServerState) -> None:
    @app.websocket("/ws")
    async def websocket_endpoint(websocket: WebSocket):
        await websocket.accept()
        state.esp32_ws = websocket
        state.esp32_current_page = -1
        TELEMETRY.inc("ws_connects")
        TELEMETRY.set("esp_connected", True)
        TELEMETRY.set("last_esp_page", -1)
        print("ESP32 connected")
        try:
            await websocket.send_text("GET_PAGE_STATE")
            TELEMETRY.inc("ws_messages_tx")
        except Exception:
            TELEMETRY.inc("ws_send_failures")

        try:
            while True:
                data = await websocket.receive_text()
                TELEMETRY.inc("ws_messages_rx")

                if data.startswith("PAGE_STATE,"):
                    try:
                        page = int(data.split(",", 1)[1])
                        state.esp32_current_page = max(0, min(9, page))
                        TELEMETRY.set("last_esp_page", state.esp32_current_page)
                    except ValueError:
                        pass
                elif data.startswith("CONFIG_MODE,ACK"):
                    print("ESP acknowledged config mode request")
                elif data.startswith("I just got petted!"):
                    print(data)
                else:
                    print(f"Received from ESP32: {data}")
        except WebSocketDisconnect:
            state.esp32_ws = None
            state.esp32_current_page = -1
            TELEMETRY.inc("ws_disconnects")
            TELEMETRY.set("esp_connected", False)
            TELEMETRY.set("last_esp_page", -1)

    @app.get("/")
    def root():
        TELEMETRY.inc("api_dashboard_hits")
        return {
            "status": "ok",
            "message": "Use the PyQt6 desktop app: python korangu_control_pyqt.py",
            "server_port": SERVER_PORT,
            "discovery_port": DISCOVERY_PORT,
        }

    @app.get("/set_mood/{mood_id}")
    async def set_mood(mood_id: int):
        TELEMETRY.inc("api_set_mood_requests")
        if state.esp32_ws:
            await state.esp32_ws.send_text(f"{mood_id},0")
            TELEMETRY.inc("ws_messages_tx")
        return {"status": "sent"}

    @app.get("/feed")
    async def feed():
        TELEMETRY.inc("api_feed_requests")
        if state.esp32_ws:
            await state.esp32_ws.send_text("-1,50")
            TELEMETRY.inc("ws_messages_tx")
        return {"status": "sent"}

    @app.get("/set_page/{page_id}")
    async def set_page(page_id: int):
        TELEMETRY.inc("api_set_page_requests")
        page_id = max(0, min(9, page_id))

        async with state.page_change_lock:
            ok, detail = await send_page_with_retry(state, page_id)

        if ok:
            TELEMETRY.inc("page_set_ack_success")
        else:
            TELEMETRY.inc("page_set_ack_failed")
        TELEMETRY.set("last_page_set_detail", f"page={page_id}, result={detail}")

        return {
            "status": "sent" if ok else "failed",
            "page": page_id,
            "ack": ok,
            "detail": detail,
        }

    @app.post("/media/set")
    async def media_set(request: MediaSetRequest):
        TELEMETRY.inc("api_media_set_requests")
        TELEMETRY.inc("media_processing_attempts")
        TELEMETRY.set("last_media_source", request.path)
        TELEMETRY.set("last_media_error", "")

        MEDIA_SCREEN.mark_processing(request.path)

        try:
            prepared = await asyncio.to_thread(
                prepare_media_clip,
                request.path,
                request.fps,
                request.dither,
                request.threshold,
                request.loop,
                request.max_frames,
                request.rotation,
                request.zoom,
                request.offset_x,
                request.offset_y,
            )
        except Exception as exc:
            detail = str(exc) or "Media processing failed"
            MEDIA_SCREEN.set_error(detail)
            TELEMETRY.inc("media_set_failed")
            TELEMETRY.record_media_status("ERROR")
            TELEMETRY.set("last_media_error", detail)
            return {
                "status": "failed",
                "detail": detail,
            }

        MEDIA_SCREEN.set_ready(prepared)
        TELEMETRY.inc("media_set_success")
        TELEMETRY.record_media_status("READY")
        TELEMETRY.set("last_media_source", prepared.source_path)

        return {
            "status": "ready",
            "detail": "Media prepared",
            "source_path": prepared.source_path,
            "source_type": prepared.source_type,
            "frames": len(prepared.frames_hex),
            "fps": round(1.0 / prepared.frame_interval, 2),
            "loop": prepared.loop,
        }

    @app.get("/media/status")
    def media_status():
        TELEMETRY.inc("api_media_status_requests")
        status = get_media_status()
        TELEMETRY.set("last_media_status", str(status.get("status", "")))
        TELEMETRY.set("last_media_error", str(status.get("error", "")))
        return status

    @app.post("/media/clear")
    def media_clear():
        TELEMETRY.inc("api_media_clear_requests")
        MEDIA_SCREEN.clear()
        TELEMETRY.record_media_status("NO_MEDIA")
        TELEMETRY.set("last_media_status", "NO_MEDIA")
        TELEMETRY.set("last_media_error", "")
        TELEMETRY.set("last_media_source", "")
        return {
            "status": "cleared",
        }

    @app.get("/esp/config_mode")
    async def esp_config_mode():
        TELEMETRY.inc("api_config_mode_requests")
        if state.esp32_ws is None:
            return {
                "status": "failed",
                "reason": "esp-disconnected",
            }

        try:
            await state.esp32_ws.send_text("CONFIG_MODE")
            TELEMETRY.inc("ws_messages_tx")
            return {
                "status": "sent",
                "instructions": [
                    "Connect your phone/PC to WiFi SSID: KoranguDesk",
                    "Password: 12345678",
                    "Open: http://192.168.4.1",
                ],
            }
        except Exception:
            TELEMETRY.inc("ws_send_failures")
            return {
                "status": "failed",
                "reason": "send-failed",
            }

    @app.get("/debug/telemetry")
    def debug_telemetry():
        return TELEMETRY.snapshot(
            server_port=SERVER_PORT,
            discovery_port=DISCOVERY_PORT,
            esp_connected=state.esp32_ws is not None,
            esp_current_page=state.esp32_current_page,
        )
