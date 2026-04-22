from __future__ import annotations

import asyncio
import importlib
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Optional, Tuple

from .state import ServerState
from .telemetry import TELEMETRY


def _optional_import(module_name: str):
    try:
        return importlib.import_module(module_name)
    except Exception:
        return None


cv2 = _optional_import("cv2")
Image = _optional_import("PIL.Image")
ImageOps = _optional_import("PIL.ImageOps")
ImageSequence = _optional_import("PIL.ImageSequence")


OLED_WIDTH = 128
OLED_HEIGHT = 64
FRAME_BYTES = OLED_WIDTH * OLED_HEIGHT // 8

MIN_STREAM_FPS = 1.0
MAX_STREAM_FPS = 12.0
DEFAULT_STREAM_FPS = 8.0
DEFAULT_MAX_FRAMES = 480
MIN_ZOOM = 0.25
MAX_ZOOM = 4.0
DEFAULT_ZOOM = 1.0
DEFAULT_ROTATION_DEG = 0.0
DEFAULT_OFFSET_X = 0
DEFAULT_OFFSET_Y = 0

IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}
VIDEO_EXTENSIONS = {".mp4", ".avi", ".mov", ".mkv", ".webm", ".m4v"}

if Image is not None:
    RESAMPLING_LANCZOS = Image.Resampling.LANCZOS if hasattr(Image, "Resampling") else Image.LANCZOS
    RESAMPLING_BICUBIC = Image.Resampling.BICUBIC if hasattr(Image, "Resampling") else Image.BICUBIC
    DITHER_FS = Image.Dither.FLOYDSTEINBERG if hasattr(Image, "Dither") else Image.FLOYDSTEINBERG
else:
    RESAMPLING_LANCZOS = None
    RESAMPLING_BICUBIC = None
    DITHER_FS = None


@dataclass(frozen=True)
class PreparedMedia:
    source_path: str
    source_type: str
    frames_hex: Tuple[str, ...]
    frame_interval: float
    loop: bool


class MediaScreenStore:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._prepared: Optional[PreparedMedia] = None
        self._status_code = "NO_MEDIA"
        self._status_text = "No media selected"
        self._error_text = ""
        self._version = 0
        self._updated_at = time.time()

    def mark_processing(self, source_path: str) -> None:
        with self._lock:
            file_name = Path(source_path).name or "media"
            self._prepared = None
            self._status_code = "PROCESSING"
            self._status_text = f"Processing {file_name}"
            self._error_text = ""
            self._version += 1
            self._updated_at = time.time()

    def set_ready(self, prepared: PreparedMedia) -> None:
        with self._lock:
            self._prepared = prepared
            self._status_code = "READY"
            self._status_text = f"Loaded {len(prepared.frames_hex)} frame(s)"
            self._error_text = ""
            self._version += 1
            self._updated_at = time.time()

    def set_error(self, error_text: str) -> None:
        with self._lock:
            self._prepared = None
            self._status_code = "ERROR"
            self._status_text = "Media processing failed"
            self._error_text = error_text
            self._version += 1
            self._updated_at = time.time()

    def clear(self) -> None:
        with self._lock:
            self._prepared = None
            self._status_code = "NO_MEDIA"
            self._status_text = "No media selected"
            self._error_text = ""
            self._version += 1
            self._updated_at = time.time()

    def snapshot(self) -> Tuple[str, str, str, int, Optional[PreparedMedia]]:
        with self._lock:
            return (
                self._status_code,
                self._status_text,
                self._error_text,
                self._version,
                self._prepared,
            )

    def public_status(self) -> Dict[str, Any]:
        status_code, status_text, error_text, _, prepared = self.snapshot()
        return {
            "status": status_code.lower(),
            "status_text": status_text,
            "error": error_text,
            "source_path": prepared.source_path if prepared else "",
            "source_type": prepared.source_type if prepared else "",
            "frames": len(prepared.frames_hex) if prepared else 0,
            "fps": round(1.0 / prepared.frame_interval, 2) if prepared else 0.0,
            "loop": prepared.loop if prepared else True,
        }


MEDIA_SCREEN = MediaScreenStore()


def _clamp_fps(fps: float) -> float:
    return max(MIN_STREAM_FPS, min(MAX_STREAM_FPS, fps))


def _frame_interval_from_fps(fps: float) -> float:
    return 1.0 / _clamp_fps(fps)


def _clamp_zoom(zoom: float) -> float:
    return max(MIN_ZOOM, min(MAX_ZOOM, zoom))


def _assert_dependencies_for_image() -> None:
    if Image is None or ImageOps is None:
        raise RuntimeError("Pillow is required for media screen support")


def _validate_media_path(path_text: str) -> Path:
    source = Path(path_text).expanduser()
    if not source.exists():
        raise ValueError(f"Media file not found: {source}")
    if not source.is_file():
        raise ValueError(f"Path is not a file: {source}")
    return source.resolve()


def _convert_frame_to_oled_bytes(
    image: Any,
    dither: bool,
    threshold: int,
    rotation_deg: float,
    zoom: float,
    offset_x: int,
    offset_y: int,
) -> bytes:
    if RESAMPLING_LANCZOS is None or RESAMPLING_BICUBIC is None or DITHER_FS is None:
        raise RuntimeError("Pillow is required for media screen support")

    # Apply transforms in source space first, then do a final OLED resize.
    source_gray = image.convert("L")
    working = source_gray

    if abs(rotation_deg) > 0.01:
        working = working.rotate(
            rotation_deg,
            resample=RESAMPLING_BICUBIC,
            expand=False,
            fillcolor=0,
        )

    zoom_value = _clamp_zoom(zoom)
    if abs(zoom_value - 1.0) > 0.0001:
        scaled_width = max(1, int(round(working.width * zoom_value)))
        scaled_height = max(1, int(round(working.height * zoom_value)))
        working = working.resize((scaled_width, scaled_height), RESAMPLING_LANCZOS)

    # Keep stage fixed to source dimensions so zoom/offset alter the framed area
    # instead of being normalized away before final OLED resize.
    stage_width = source_gray.width
    stage_height = source_gray.height

    canvas = Image.new("L", (stage_width, stage_height), 0)
    paste_x = ((stage_width - working.width) // 2) + int(offset_x)
    paste_y = ((stage_height - working.height) // 2) + int(offset_y)
    canvas.paste(working, (paste_x, paste_y))

    # Final resize to OLED happens after all transforms.
    canvas = ImageOps.fit(canvas, (OLED_WIDTH, OLED_HEIGHT), method=RESAMPLING_LANCZOS)

    gray = ImageOps.autocontrast(canvas, cutoff=2)

    if dither:
        mono = gray.convert("1", dither=DITHER_FS)
    else:
        mono = gray.point(lambda pixel: 255 if pixel >= threshold else 0, mode="1")

    packed = mono.tobytes()
    if len(packed) != FRAME_BYTES:
        raise ValueError("Unexpected packed frame size")

    return packed


def _load_image_or_gif_frames(
    source: Path,
    fps: float,
    dither: bool,
    threshold: int,
    max_frames: int,
    rotation_deg: float,
    zoom: float,
    offset_x: int,
    offset_y: int,
) -> Tuple[Tuple[str, ...], float, str]:
    _assert_dependencies_for_image()

    with Image.open(source) as image:
        is_animated = bool(getattr(image, "is_animated", False) and getattr(image, "n_frames", 1) > 1)

        if not is_animated:
            frame_hex = _convert_frame_to_oled_bytes(
                image,
                dither=dither,
                threshold=threshold,
                rotation_deg=rotation_deg,
                zoom=zoom,
                offset_x=offset_x,
                offset_y=offset_y,
            ).hex()
            return (frame_hex,), 1.0, "image"

        if ImageSequence is None:
            raise RuntimeError("Animated image decoding requires Pillow")

        frame_hexes = []
        durations_ms = []

        for idx, frame in enumerate(ImageSequence.Iterator(image)):
            if idx >= max_frames:
                break
            frame_hexes.append(
                _convert_frame_to_oled_bytes(
                    frame,
                    dither=dither,
                    threshold=threshold,
                    rotation_deg=rotation_deg,
                    zoom=zoom,
                    offset_x=offset_x,
                    offset_y=offset_y,
                ).hex()
            )
            duration = frame.info.get("duration", 0)
            if isinstance(duration, (int, float)) and duration > 0:
                durations_ms.append(float(duration))

        if not frame_hexes:
            raise ValueError("No decodable frames found in image")

        if durations_ms:
            avg_seconds = (sum(durations_ms) / len(durations_ms)) / 1000.0
            if avg_seconds > 0:
                interval = _frame_interval_from_fps(1.0 / avg_seconds)
            else:
                interval = _frame_interval_from_fps(fps)
        else:
            interval = _frame_interval_from_fps(fps)

        return tuple(frame_hexes), interval, "animated-image"


def _load_video_frames(
    source: Path,
    fps: float,
    dither: bool,
    threshold: int,
    max_frames: int,
    rotation_deg: float,
    zoom: float,
    offset_x: int,
    offset_y: int,
) -> Tuple[Tuple[str, ...], float, str]:
    _assert_dependencies_for_image()
    if cv2 is None:
        raise RuntimeError("opencv-python-headless is required for video screen support")

    capture = cv2.VideoCapture(str(source))
    if not capture.isOpened():
        raise ValueError("Could not open video file")

    requested_fps = _clamp_fps(fps)
    source_fps = capture.get(cv2.CAP_PROP_FPS)
    if not source_fps or source_fps <= 0 or source_fps > 240:
        source_fps = requested_fps

    frame_stride = max(1, int(round(source_fps / requested_fps)))

    frame_hexes = []
    source_index = 0

    try:
        while len(frame_hexes) < max_frames:
            ok, frame_bgr = capture.read()
            if not ok:
                break

            should_sample = (source_index % frame_stride) == 0
            source_index += 1
            if not should_sample:
                continue

            frame_rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)
            pil_frame = Image.fromarray(frame_rgb)
            frame_hexes.append(
                _convert_frame_to_oled_bytes(
                    pil_frame,
                    dither=dither,
                    threshold=threshold,
                    rotation_deg=rotation_deg,
                    zoom=zoom,
                    offset_x=offset_x,
                    offset_y=offset_y,
                ).hex()
            )
    finally:
        capture.release()

    if not frame_hexes:
        raise ValueError("No decodable frames found in video")

    return tuple(frame_hexes), _frame_interval_from_fps(requested_fps), "video"


def prepare_media_clip(
    path_text: str,
    fps: float = DEFAULT_STREAM_FPS,
    dither: bool = True,
    threshold: int = 128,
    loop: bool = True,
    max_frames: int = DEFAULT_MAX_FRAMES,
    rotation_deg: float = DEFAULT_ROTATION_DEG,
    zoom: float = DEFAULT_ZOOM,
    offset_x: int = DEFAULT_OFFSET_X,
    offset_y: int = DEFAULT_OFFSET_Y,
) -> PreparedMedia:
    if threshold < 1 or threshold > 254:
        raise ValueError("Threshold must be in the range 1..254")
    if max_frames < 1:
        raise ValueError("max_frames must be >= 1")
    if zoom < MIN_ZOOM or zoom > MAX_ZOOM:
        raise ValueError(f"zoom must be in the range {MIN_ZOOM:.2f}..{MAX_ZOOM:.2f}")

    source = _validate_media_path(path_text)
    requested_fps = _clamp_fps(fps)
    normalized_zoom = _clamp_zoom(zoom)
    normalized_offset_x = int(offset_x)
    normalized_offset_y = int(offset_y)

    suffix = source.suffix.lower()
    if suffix in IMAGE_EXTENSIONS:
        frames_hex, frame_interval, source_type = _load_image_or_gif_frames(
            source,
            fps=requested_fps,
            dither=dither,
            threshold=threshold,
            max_frames=max_frames,
            rotation_deg=rotation_deg,
            zoom=normalized_zoom,
            offset_x=normalized_offset_x,
            offset_y=normalized_offset_y,
        )
    elif suffix in VIDEO_EXTENSIONS:
        frames_hex, frame_interval, source_type = _load_video_frames(
            source,
            fps=requested_fps,
            dither=dither,
            threshold=threshold,
            max_frames=max_frames,
            rotation_deg=rotation_deg,
            zoom=normalized_zoom,
            offset_x=normalized_offset_x,
            offset_y=normalized_offset_y,
        )
    else:
        # Try image decode first, then video decode, to support uncommon file extensions.
        try:
            frames_hex, frame_interval, source_type = _load_image_or_gif_frames(
                source,
                fps=requested_fps,
                dither=dither,
                threshold=threshold,
                max_frames=max_frames,
                rotation_deg=rotation_deg,
                zoom=normalized_zoom,
                offset_x=normalized_offset_x,
                offset_y=normalized_offset_y,
            )
        except Exception:
            frames_hex, frame_interval, source_type = _load_video_frames(
                source,
                fps=requested_fps,
                dither=dither,
                threshold=threshold,
                max_frames=max_frames,
                rotation_deg=rotation_deg,
                zoom=normalized_zoom,
                offset_x=normalized_offset_x,
                offset_y=normalized_offset_y,
            )

    return PreparedMedia(
        source_path=str(source),
        source_type=source_type,
        frames_hex=frames_hex,
        frame_interval=frame_interval,
        loop=loop,
    )


def get_media_status() -> Dict[str, Any]:
    return MEDIA_SCREEN.public_status()


async def media_screen_watcher(state: ServerState) -> None:
    active_version = -1
    frame_index = 0
    next_frame_deadline = time.monotonic()
    last_status_sent = ""

    while True:
        ws = state.esp32_ws
        should_stream = ws is not None and state.esp32_current_page == 8

        status_code, _, _, version, prepared = MEDIA_SCREEN.snapshot()

        if not should_stream:
            active_version = -1
            frame_index = 0
            last_status_sent = ""
            await asyncio.sleep(0.35)
            continue

        if status_code != "READY" or prepared is None or not prepared.frames_hex:
            outgoing_status = status_code if status_code in {"NO_MEDIA", "PROCESSING", "ERROR"} else "NO_MEDIA"

            if outgoing_status != last_status_sent and ws is not None:
                try:
                    await ws.send_text(f"MEDIA_STATUS:{outgoing_status}")
                    TELEMETRY.inc("ws_messages_tx")
                    TELEMETRY.inc("media_status_sent")
                    TELEMETRY.record_media_status(outgoing_status)
                except Exception:
                    TELEMETRY.inc("ws_send_failures")

                last_status_sent = outgoing_status

            await asyncio.sleep(0.25)
            continue

        if version != active_version:
            active_version = version
            frame_index = 0
            next_frame_deadline = time.monotonic()
            last_status_sent = "READY"
            if ws is not None:
                try:
                    await ws.send_text("MEDIA_STATUS:READY")
                    TELEMETRY.inc("ws_messages_tx")
                    TELEMETRY.inc("media_status_sent")
                    TELEMETRY.record_media_status("READY")
                except Exception:
                    TELEMETRY.inc("ws_send_failures")

        now = time.monotonic()
        if now < next_frame_deadline:
            await asyncio.sleep(min(0.02, next_frame_deadline - now))
            continue

        if ws is None:
            await asyncio.sleep(0.2)
            continue

        frame_hex = prepared.frames_hex[frame_index]
        try:
            await ws.send_text(f"MEDIA_FRAME:{frame_hex}")
            TELEMETRY.inc("ws_messages_tx")
            TELEMETRY.inc("media_frames_sent")
        except Exception:
            TELEMETRY.inc("ws_send_failures")
            TELEMETRY.inc("media_frame_send_failures")
            await asyncio.sleep(0.1)
            continue

        frame_index += 1
        if frame_index >= len(prepared.frames_hex):
            frame_index = 0 if prepared.loop else len(prepared.frames_hex) - 1

        next_frame_deadline = now + prepared.frame_interval
