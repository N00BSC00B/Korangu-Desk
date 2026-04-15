from __future__ import annotations

from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from .config import DISCOVERY_PORT, SERVER_PORT
from .hardware import send_page_with_retry
from .state import ServerState
from .telemetry import TELEMETRY


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
                        state.esp32_current_page = max(0, min(7, page))
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
        page_id = max(0, min(7, page_id))

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
