from __future__ import annotations

import threading
import time
from typing import Any, Dict


class TelemetryStore:
    def __init__(self) -> None:
        self._started_monotonic = time.monotonic()
        self._lock = threading.Lock()
        self._counters: Dict[str, int] = {
            "udp_beacon_broadcast_cycles": 0,
            "udp_discovery_probe_received": 0,
            "udp_discovery_probe_replied": 0,
            "ws_connects": 0,
            "ws_disconnects": 0,
            "ws_messages_rx": 0,
            "ws_messages_tx": 0,
            "ws_send_failures": 0,
            "api_dashboard_hits": 0,
            "api_set_mood_requests": 0,
            "api_feed_requests": 0,
            "api_set_page_requests": 0,
            "api_config_mode_requests": 0,
            "page_set_ack_success": 0,
            "page_set_ack_failed": 0,
            "lyric_fetch_attempts": 0,
            "lyric_fetch_success": 0,
            "lyric_fetch_not_found": 0,
            "lyric_fetch_error": 0,
            "lyric_status_sent": 0,
            "lyric_lines_sent": 0,
            "lyrics_waiting_line_events": 0,
            "lyrics_no_media_events": 0,
            "lyrics_no_song_events": 0,
            "hardware_stats_packets_sent": 0,
            "api_media_set_requests": 0,
            "api_media_status_requests": 0,
            "api_media_clear_requests": 0,
            "media_set_success": 0,
            "media_set_failed": 0,
            "media_processing_attempts": 0,
            "media_frames_sent": 0,
            "media_status_sent": 0,
            "media_frame_send_failures": 0,
        }
        self._state: Dict[str, Any] = {
            "esp_connected": False,
            "last_esp_page": -1,
            "last_lyric_status": "",
            "last_lyric_line": "",
            "last_lyrics_lookup": "",
            "last_page_set_detail": "",
            "lyric_status_counts": {},
            "last_media_status": "",
            "last_media_source": "",
            "last_media_error": "",
            "media_status_counts": {},
        }

    def inc(self, counter_name: str, amount: int = 1) -> None:
        with self._lock:
            self._counters[counter_name] = self._counters.get(counter_name, 0) + amount

    def set(self, state_key: str, value: Any) -> None:
        with self._lock:
            self._state[state_key] = value

    def record_lyric_status(self, status_code: str) -> None:
        with self._lock:
            status_counts = self._state.get("lyric_status_counts", {})
            status_counts[status_code] = status_counts.get(status_code, 0) + 1
            self._state["lyric_status_counts"] = status_counts
            self._state["last_lyric_status"] = status_code

    def record_media_status(self, status_code: str) -> None:
        with self._lock:
            status_counts = self._state.get("media_status_counts", {})
            status_counts[status_code] = status_counts.get(status_code, 0) + 1
            self._state["media_status_counts"] = status_counts
            self._state["last_media_status"] = status_code

    def snapshot(self, server_port: int, discovery_port: int, esp_connected: bool, esp_current_page: int) -> Dict[str, Any]:
        with self._lock:
            counters_copy = dict(self._counters)
            state_copy = dict(self._state)
            state_copy["lyric_status_counts"] = dict(self._state.get("lyric_status_counts", {}))
            state_copy["media_status_counts"] = dict(self._state.get("media_status_counts", {}))

        return {
            "uptime_seconds": round(time.monotonic() - self._started_monotonic, 2),
            "server_port": server_port,
            "discovery_port": discovery_port,
            "esp_connected": esp_connected,
            "esp_current_page": esp_current_page,
            "counters": counters_copy,
            "state": state_copy,
        }


TELEMETRY = TelemetryStore()
