from __future__ import annotations

import asyncio
import time
from typing import Any, Dict, List, Tuple
from urllib.parse import quote_plus

import requests

from .state import ServerState
from .telemetry import TELEMETRY

try:
    from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager
except Exception:
    GlobalSystemMediaTransportControlsSessionManager = None


async def get_current_media_info() -> Dict[str, Any] | None:
    if GlobalSystemMediaTransportControlsSessionManager is None:
        return None

    try:
        sessions = await GlobalSystemMediaTransportControlsSessionManager.request_async()
        current_session = sessions.get_current_session()
        if current_session:
            media_props = await current_session.try_get_media_properties_async()
            timeline = current_session.get_timeline_properties()
            playback_info = current_session.get_playback_info()
            return {
                "title": media_props.title,
                "artist": media_props.artist,
                "reported_position": timeline.position.total_seconds(),
                "is_playing": playback_info.playback_status == 4,
            }
    except Exception:
        return None

    return None


def fetch_lyrics(title: str, artist: str) -> Tuple[List[Dict[str, Any]], str]:
    TELEMETRY.inc("lyric_fetch_attempts")
    TELEMETRY.set("last_lyrics_lookup", f"{artist or ''} | {title or ''}")

    if not title:
        TELEMETRY.inc("lyric_fetch_not_found")
        return [], "LYRICS_NOT_FOUND"

    track_name = quote_plus(title)
    artist_name = quote_plus(artist or "")
    url = f"https://lrclib.net/api/get?track_name={track_name}&artist_name={artist_name}"

    try:
        response = requests.get(url, timeout=6)
    except requests.RequestException:
        TELEMETRY.inc("lyric_fetch_error")
        return [], "LYRICS_FETCH_ERROR"
    except Exception:
        TELEMETRY.inc("lyric_fetch_error")
        return [], "LYRICS_FETCH_ERROR"

    if response.status_code == 404:
        TELEMETRY.inc("lyric_fetch_not_found")
        return [], "LYRICS_NOT_FOUND"
    if response.status_code != 200:
        TELEMETRY.inc("lyric_fetch_error")
        return [], "LYRICS_FETCH_ERROR"

    try:
        data = response.json()
    except Exception:
        TELEMETRY.inc("lyric_fetch_error")
        return [], "LYRICS_FETCH_ERROR"

    synced = data.get("syncedLyrics")
    if not synced:
        TELEMETRY.inc("lyric_fetch_not_found")
        return [], "LYRICS_NOT_FOUND"

    lines = synced.split("\n")
    parsed: List[Dict[str, Any]] = []
    for line in lines:
        if line.startswith("["):
            try:
                minutes, seconds = line[1:9].split(":")
                parsed.append({"time": int(minutes) * 60 + float(seconds), "text": line[10:].strip()})
            except ValueError:
                continue

    if not parsed:
        TELEMETRY.inc("lyric_fetch_not_found")
        return [], "LYRICS_NOT_FOUND"

    TELEMETRY.inc("lyric_fetch_success")
    return parsed, ""


async def media_watcher(state: ServerState) -> None:
    last_reported_position = -1.0
    last_system_time = time.time()
    current_title = ""
    current_lyrics: List[Dict[str, Any]] = []
    last_printed_index = -1
    lyrics_mode_active = False
    last_status_sent = ""

    async def send_lyric_status(status_code: str) -> None:
        nonlocal last_status_sent, last_printed_index
        if status_code == last_status_sent:
            return

        if state.esp32_ws:
            try:
                await state.esp32_ws.send_text(f"LYRIC_STATUS:{status_code}")
                TELEMETRY.inc("ws_messages_tx")
                TELEMETRY.inc("lyric_status_sent")
                TELEMETRY.record_lyric_status(status_code)
            except Exception:
                TELEMETRY.inc("ws_send_failures")

        last_status_sent = status_code
        last_printed_index = -1

    async def send_lyric_text(text: str) -> None:
        nonlocal last_status_sent
        if state.esp32_ws:
            try:
                await state.esp32_ws.send_text(f"LYRIC:{text}")
                TELEMETRY.inc("ws_messages_tx")
                TELEMETRY.inc("lyric_lines_sent")
                TELEMETRY.set("last_lyric_line", text)
            except Exception:
                TELEMETRY.inc("ws_send_failures")
        last_status_sent = ""

    while True:
        should_stream_lyrics = state.esp32_ws is not None and state.esp32_current_page == 6

        if not should_stream_lyrics:
            if lyrics_mode_active:
                last_reported_position = -1
                last_system_time = time.time()
                current_title = ""
                current_lyrics = []
                last_printed_index = -1
                last_status_sent = ""
            lyrics_mode_active = False
            await asyncio.sleep(0.6)
            continue

        if not lyrics_mode_active:
            last_printed_index = -1
            lyrics_mode_active = True

        media = await get_current_media_info()
        if media is None:
            TELEMETRY.inc("lyrics_no_media_events")
            await send_lyric_status("NO_MEDIA_SESSION")
            await asyncio.sleep(0.35)
            continue

        if not media["is_playing"]:
            TELEMETRY.inc("lyrics_no_song_events")
            await send_lyric_status("NO_SONG")
            await asyncio.sleep(0.35)
            continue

        now = time.time()
        if media["title"] != current_title:
            current_title = media["title"]
            last_reported_position = media["reported_position"]
            last_system_time = now
            current_lyrics, lyric_error = fetch_lyrics(media["title"], media["artist"])
            last_printed_index = -1

            if lyric_error:
                await send_lyric_status(lyric_error)
                await asyncio.sleep(0.25)
                continue
        elif media["reported_position"] != last_reported_position:
            last_reported_position = media["reported_position"]
            last_system_time = now

        if not current_lyrics:
            await send_lyric_status("LYRICS_NOT_FOUND")
            await asyncio.sleep(0.25)
            continue

        smooth_position = last_reported_position + (now - last_system_time)
        active_index = -1

        for i, line in enumerate(current_lyrics):
            if smooth_position >= line["time"]:
                active_index = i
            else:
                break

        if active_index != -1 and active_index != last_printed_index:
            lyric_text = current_lyrics[active_index]["text"].strip() or "[music]"
            await send_lyric_text(lyric_text)
            last_printed_index = active_index
        elif active_index == -1 and last_printed_index != -2:
            TELEMETRY.inc("lyrics_waiting_line_events")
            await send_lyric_status("WAITING_LINE")
            last_printed_index = -2

        await asyncio.sleep(0.12)
