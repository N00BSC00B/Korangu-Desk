import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

import requests

try:
    from korangu_server.media_screen import prepare_media_clip
except Exception:
    prepare_media_clip = None

try:
    from PyQt6.QtCore import Qt, QTimer
    from PyQt6.QtGui import QFont, QImage, QPixmap
    from PyQt6.QtWidgets import (
        QApplication,
        QCheckBox,
        QDoubleSpinBox,
        QFileDialog,
        QGridLayout,
        QGroupBox,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QPushButton,
        QSlider,
        QSpinBox,
        QSplitter,
        QTextEdit,
        QVBoxLayout,
        QWidget,
    )
except Exception:  # pragma: no cover
    print("PyQt6 is required. Install with: pip install PyQt6 requests")
    raise


MOODS: List[Tuple[str, int]] = [
    ("😐 Normal", 0),
    ("😊 Happy", 1),
    ("😌 Relaxed", 10),
    ("❤️ Love", 7),
    ("⚡ Excited", 6),
    ("😲 Surprised", 2),
    ("🤨 Suspicious", 8),
    ("😡 Grumpy", 4),
    ("🥺 Sad", 5),
    ("💤 Sleepy", 3),
]

PAGES: List[Tuple[str, int]] = [
    ("🐵 Face", 0),
    ("⏱️ Clock", 1),
    ("⛅ Weather", 2),
    ("🌍 World Clock", 3),
    ("📅 Forecast", 4),
    ("🎵 Lyrics View", 5),
    ("💻 Cyberdeck", 6),
    ("🦖 Mini-Game", 7),
    ("🎬 Media Screen", 8),
]

# Native Desktop Theme (Subtle application of the web colors)
STYLESHEET = """
QMainWindow {
    background-color: #f5f6f7;
}
QGroupBox {
    font-weight: bold;
    color: #444;
    border: 1px solid #dcdde1;
    border-radius: 6px;
    margin-top: 12px;
    background-color: #ffffff;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 5px;
    color: #ff69b4; /* Theme accent for group headers */
}
QLabel, QCheckBox {
    color: #263238;
}
QPushButton {
    padding: 8px 12px;
    background-color: #fdfdfd;
    color: #333;
    border: 1px solid #dcdde1;
    border-radius: 4px;
}
QPushButton:hover {
    background-color: #f0f0f0;
    border-color: #add8e6;
}
QPushButton:pressed {
    background-color: #e0e0e0;
}
QPushButton[class="btn-feed"] {
    background-color: #ff69b4;
    color: white;
    border: 1px solid #ff1493;
    font-weight: bold;
}
QPushButton[class="btn-feed"]:hover {
    background-color: #ff1493;
}
QPushButton[class="btn-config"] {
    background-color: #7dbb58;
    color: white;
    border: 1px solid #5d9f3b;
    font-weight: bold;
}
QPushButton[class="btn-config"]:hover {
    background-color: #5d9f3b;
}
QLineEdit, QTextEdit {
    background-color: #ffffff;
    color: #1f2933;
    border: 1px solid #dcdde1;
    border-radius: 4px;
    padding: 6px;
}
QSpinBox, QDoubleSpinBox {
    background-color: #ffffff;
    color: #1f2933;
    border: 1px solid #dcdde1;
    border-radius: 4px;
    padding: 4px 6px;
}
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {
    width: 16px;
}
QSlider::groove:horizontal {
    border: 1px solid #c9ced6;
    height: 6px;
    border-radius: 3px;
    background: #e2e7ee;
}
QSlider::handle:horizontal {
    background: #3f8efc;
    border: 1px solid #2f6ec6;
    width: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
QTextEdit {
    font-family: Consolas, "Courier New", monospace;
    font-size: 12px;
    color: #333;
}
QStatusBar {
    background-color: #e8e8e8;
    color: #333;
    border-top: 1px solid #dcdde1;
}
"""


class KoranguControlWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Korangu Control Center")
        self.resize(900, 600)  # Wider desktop aspect ratio

        self.telemetry_timer = QTimer(self)
        self.telemetry_timer.timeout.connect(self.refresh_telemetry)

        self.preview_timer = QTimer(self)
        self.preview_timer.timeout.connect(self.advance_media_preview)
        self.preview_pixmaps: List[QPixmap] = []
        self.preview_index = 0
        self.preview_interval_ms = 125
        self.preview_loop = True
        self.preview_fps = 0.0

        self.preview_recalc_timer = QTimer(self)
        self.preview_recalc_timer.setSingleShot(True)
        self.preview_recalc_timer.timeout.connect(self.refresh_media_preview)

        # Central Widget & Main Splitter
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
        main_layout.setContentsMargins(15, 15, 15, 15)

        # Use a QSplitter for native side-by-side resizing
        splitter = QSplitter(Qt.Orientation.Horizontal)
        main_layout.addWidget(splitter)

        # --- LEFT PANEL: Controls ---
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(0, 0, 10, 0)

        # Title Area
        title_label = QLabel("🐒 Korangu Control")
        title_label.setStyleSheet("font-size: 20px; font-weight: bold; color: #ff69b4;")
        left_layout.addWidget(title_label)

        # Moods GroupBox
        mood_box = QGroupBox("Emotional State")
        mood_grid = QGridLayout(mood_box)
        mood_grid.setSpacing(8)
        for idx, (label, mood_id) in enumerate(MOODS):
            btn = QPushButton(label)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)
            btn.clicked.connect(lambda _=False, mid=mood_id, name=label: self.send_mood(mid, name))
            mood_grid.addWidget(btn, idx // 2, idx % 2)

        feed_btn = QPushButton("☕ Feed Coffee (+50 Energy)")
        feed_btn.setProperty("class", "btn-feed")
        feed_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        feed_btn.clicked.connect(self.feed)
        mood_grid.addWidget(feed_btn, (len(MOODS) + 1) // 2, 0, 1, 2)
        left_layout.addWidget(mood_box)

        # Pages GroupBox
        page_box = QGroupBox("Display Screens")
        page_grid = QGridLayout(page_box)
        page_grid.setSpacing(8)
        for idx, (label, page_id) in enumerate(PAGES):
            btn = QPushButton(label)
            btn.setCursor(Qt.CursorShape.PointingHandCursor)
            btn.clicked.connect(lambda _=False, pid=page_id, name=label: self.send_page(pid, name))
            page_grid.addWidget(btn, idx // 2, idx % 2)

        config_btn = QPushButton("⚙️ Enter ESP Setup Mode")
        config_btn.setProperty("class", "btn-config")
        config_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        config_btn.clicked.connect(self.enter_config_mode)
        page_grid.addWidget(config_btn, (len(PAGES) + 1) // 2, 0, 1, 2)
        left_layout.addWidget(page_box)

        left_layout.addStretch()  # Push controls to top

        # --- RIGHT PANEL: Server & Telemetry ---
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(10, 0, 0, 0)

        # Connection GroupBox
        conn_box = QGroupBox("Server Connection")
        conn_layout = QVBoxLayout(conn_box)

        server_row = QHBoxLayout()
        server_row.addWidget(QLabel("URL:"))
        self.server_url = QLineEdit("http://127.0.0.1:8765")
        server_row.addWidget(self.server_url)

        ping_btn = QPushButton("Ping")
        ping_btn.clicked.connect(self.ping_server)
        server_row.addWidget(ping_btn)
        conn_layout.addLayout(server_row)

        self.auto_refresh_check = QCheckBox("Auto telemetry refresh (3s)")
        self.auto_refresh_check.toggled.connect(self.toggle_auto_refresh)
        conn_layout.addWidget(self.auto_refresh_check)

        right_layout.addWidget(conn_box)

        # Media Studio GroupBox
        media_box = QGroupBox("Media Studio")
        media_layout = QVBoxLayout(media_box)

        media_path_row = QHBoxLayout()
        self.media_path = QLineEdit()
        self.media_path.setPlaceholderText("Select image/video for OLED conversion")
        media_path_row.addWidget(self.media_path)

        browse_media_btn = QPushButton("Browse")
        browse_media_btn.clicked.connect(self.browse_media_file)
        media_path_row.addWidget(browse_media_btn)
        media_layout.addLayout(media_path_row)

        controls_grid = QGridLayout()
        controls_grid.addWidget(QLabel("FPS"), 0, 0)
        self.media_fps = QDoubleSpinBox()
        self.media_fps.setDecimals(1)
        self.media_fps.setRange(1.0, 12.0)
        self.media_fps.setSingleStep(0.5)
        self.media_fps.setValue(8.0)
        controls_grid.addWidget(self.media_fps, 0, 1)

        controls_grid.addWidget(QLabel("Threshold"), 0, 2)
        self.media_threshold = QSpinBox()
        self.media_threshold.setRange(1, 254)
        self.media_threshold.setValue(128)
        controls_grid.addWidget(self.media_threshold, 0, 3)

        controls_grid.addWidget(QLabel("Max Frames"), 1, 0)
        self.media_max_frames = QSpinBox()
        self.media_max_frames.setRange(1, 1200)
        self.media_max_frames.setValue(360)
        controls_grid.addWidget(self.media_max_frames, 1, 1)

        controls_grid.addWidget(QLabel("Rotation (deg)"), 1, 2)
        self.media_rotation = QDoubleSpinBox()
        self.media_rotation.setDecimals(1)
        self.media_rotation.setRange(-180.0, 180.0)
        self.media_rotation.setSingleStep(5.0)
        self.media_rotation.setValue(0.0)
        controls_grid.addWidget(self.media_rotation, 1, 3)

        controls_grid.addWidget(QLabel("Zoom"), 2, 0)
        self.media_zoom = QDoubleSpinBox()
        self.media_zoom.setDecimals(2)
        self.media_zoom.setRange(0.25, 4.0)
        self.media_zoom.setSingleStep(0.05)
        self.media_zoom.setValue(1.00)
        controls_grid.addWidget(self.media_zoom, 2, 1)

        controls_grid.addWidget(QLabel("Offset X"), 2, 2)
        self.media_offset_x = QSpinBox()
        self.media_offset_x.setRange(-256, 256)
        self.media_offset_x.setValue(0)
        controls_grid.addWidget(self.media_offset_x, 2, 3)

        controls_grid.addWidget(QLabel("Offset Y"), 3, 0)
        self.media_offset_y = QSpinBox()
        self.media_offset_y.setRange(-128, 128)
        self.media_offset_y.setValue(0)
        controls_grid.addWidget(self.media_offset_y, 3, 1)

        self.media_dither = QCheckBox("Dither")
        self.media_dither.setChecked(True)
        controls_grid.addWidget(self.media_dither, 3, 2)

        self.media_loop = QCheckBox("Loop")
        self.media_loop.setChecked(True)
        controls_grid.addWidget(self.media_loop, 3, 3)

        media_layout.addLayout(controls_grid)

        media_buttons_row = QHBoxLayout()
        preview_btn = QPushButton("Preview")
        preview_btn.clicked.connect(self.refresh_media_preview)
        media_buttons_row.addWidget(preview_btn)

        send_media_btn = QPushButton("Send To Media Screen")
        send_media_btn.clicked.connect(self.send_media_with_controls)
        media_buttons_row.addWidget(send_media_btn)

        clear_media_btn = QPushButton("Clear")
        clear_media_btn.clicked.connect(self.clear_media_for_screen)
        media_buttons_row.addWidget(clear_media_btn)

        reset_transform_btn = QPushButton("Reset View")
        reset_transform_btn.clicked.connect(self.reset_media_transform)
        media_buttons_row.addWidget(reset_transform_btn)
        media_layout.addLayout(media_buttons_row)

        self.preview_animate = QCheckBox("Animate Preview")
        self.preview_animate.setChecked(True)
        self.preview_animate.toggled.connect(self.toggle_preview_playback)
        media_layout.addWidget(self.preview_animate)

        self.auto_recalc_preview = QCheckBox("Auto Recalculate On Control Change")
        self.auto_recalc_preview.setChecked(True)
        media_layout.addWidget(self.auto_recalc_preview)

        self.preview_frame = QLabel("OLED preview will appear here")
        self.preview_frame.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.preview_frame.setMinimumSize(396, 210)
        self.preview_frame.setStyleSheet(
            "QLabel { background-color: #101216; color: #8edcff; border: 1px solid #3a4a5a; border-radius: 6px; }"
        )
        media_layout.addWidget(self.preview_frame)

        self.preview_scrubber = QSlider(Qt.Orientation.Horizontal)
        self.preview_scrubber.setMinimum(0)
        self.preview_scrubber.setMaximum(0)
        self.preview_scrubber.setEnabled(False)
        self.preview_scrubber.valueChanged.connect(self.on_preview_scrub)
        media_layout.addWidget(self.preview_scrubber)

        self.preview_meta = QLabel("No preview generated yet")
        media_layout.addWidget(self.preview_meta)

        self.media_path.textChanged.connect(self.schedule_preview_recalc)
        self.media_fps.valueChanged.connect(self.schedule_preview_recalc)
        self.media_threshold.valueChanged.connect(self.schedule_preview_recalc)
        self.media_max_frames.valueChanged.connect(self.schedule_preview_recalc)
        self.media_rotation.valueChanged.connect(self.schedule_preview_recalc)
        self.media_zoom.valueChanged.connect(self.schedule_preview_recalc)
        self.media_offset_x.valueChanged.connect(self.schedule_preview_recalc)
        self.media_offset_y.valueChanged.connect(self.schedule_preview_recalc)
        self.media_dither.toggled.connect(self.schedule_preview_recalc)
        self.media_loop.toggled.connect(self.schedule_preview_recalc)

        right_layout.addWidget(media_box)

        # Telemetry GroupBox
        telemetry_box = QGroupBox("Live Telemetry")
        telemetry_layout = QVBoxLayout(telemetry_box)

        refresh_btn = QPushButton("🔄 Refresh Telemetry Now")
        refresh_btn.clicked.connect(self.refresh_telemetry)
        telemetry_layout.addWidget(refresh_btn)

        self.telemetry_view = QTextEdit()
        self.telemetry_view.setReadOnly(True)
        telemetry_layout.addWidget(self.telemetry_view)

        right_layout.addWidget(telemetry_box)

        # Add panels to splitter
        splitter.addWidget(left_panel)
        splitter.addWidget(right_panel)
        splitter.setSizes([450, 450])  # Initial 50/50 split

        # Native Status Bar
        self.statusBar().showMessage("Ready.")
        self.setStyleSheet(STYLESHEET)

    # === API Methods ===
    def base_url(self) -> str:
        return self.server_url.text().strip().rstrip("/")

    def set_status(self, text: str) -> None:
        # Use native status bar instead of a label
        self.statusBar().showMessage(text)

    def api_get(self, path: str) -> Dict:
        url = f"{self.base_url()}{path}"
        response = requests.get(url, timeout=5)
        try:
            data = response.json()
        except Exception:
            data = {"raw": response.text}

        if response.status_code >= 400:
            raise RuntimeError(f"HTTP {response.status_code}: {data}")

        return data

    def api_post(self, path: str, payload: Dict) -> Dict:
        url = f"{self.base_url()}{path}"
        response = requests.post(url, json=payload, timeout=90)
        try:
            data = response.json()
        except Exception:
            data = {"raw": response.text}

        if response.status_code >= 400:
            raise RuntimeError(f"HTTP {response.status_code}: {data}")

        return data

    def media_request_payload(self) -> Dict[str, Any]:
        media_path = self.media_path.text().strip()
        if not media_path:
            raise ValueError("Choose a media file first")

        return {
            "path": media_path,
            "fps": float(self.media_fps.value()),
            "dither": bool(self.media_dither.isChecked()),
            "threshold": int(self.media_threshold.value()),
            "loop": bool(self.media_loop.isChecked()),
            "max_frames": int(self.media_max_frames.value()),
            "rotation": float(self.media_rotation.value()),
            "zoom": float(self.media_zoom.value()),
            "offset_x": int(self.media_offset_x.value()),
            "offset_y": int(self.media_offset_y.value()),
        }

    def reset_media_transform(self) -> None:
        self.media_rotation.setValue(0.0)
        self.media_zoom.setValue(1.0)
        self.media_offset_x.setValue(0)
        self.media_offset_y.setValue(0)
        self.set_status("Media view transform reset")

    def schedule_preview_recalc(self, *_: Any) -> None:
        if prepare_media_clip is None:
            return
        if not self.auto_recalc_preview.isChecked():
            return
        if not self.media_path.text().strip():
            return

        # Debounce slider/spinbox drags to avoid expensive reprocessing on every tick.
        self.preview_recalc_timer.start(220)

    def browse_media_file(self) -> None:
        selected_path, _ = QFileDialog.getOpenFileName(
            self,
            "Select image/video for OLED media screen",
            str(Path.home()),
            "Media Files (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.mp4 *.avi *.mov *.mkv *.webm *.m4v);;All Files (*)",
        )

        if not selected_path:
            self.set_status("Media selection cancelled")
            return

        self.media_path.setText(selected_path)
        self.set_status("Media selected. Click Preview or Send.")
        self.schedule_preview_recalc()

    def frame_hex_to_pixmap(self, frame_hex: str, scale_factor: int = 3) -> QPixmap:
        width, height = 128, 64
        expected_bytes = width * height // 8
        frame_bytes = bytes.fromhex(frame_hex)
        if len(frame_bytes) != expected_bytes:
            raise ValueError("Unexpected frame size from media processor")

        rgba = bytearray(width * height * 4)
        for pixel_index in range(width * height):
            byte_index = pixel_index >> 3
            bit_index = 7 - (pixel_index & 7)
            is_on = ((frame_bytes[byte_index] >> bit_index) & 0x01) == 1
            value = 255 if is_on else 0

            rgba_index = pixel_index * 4
            rgba[rgba_index] = value
            rgba[rgba_index + 1] = value
            rgba[rgba_index + 2] = value
            rgba[rgba_index + 3] = 255

        image = QImage(bytes(rgba), width, height, QImage.Format.Format_RGBA8888).copy()
        pixmap = QPixmap.fromImage(image)
        return pixmap.scaled(
            width * scale_factor,
            height * scale_factor,
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.FastTransformation,
        )

    def set_preview_frame(self, frame_index: int, sync_slider: bool = True) -> None:
        if not self.preview_pixmaps:
            return

        frame_index = max(0, min(len(self.preview_pixmaps) - 1, frame_index))
        self.preview_index = frame_index
        self.preview_frame.setPixmap(self.preview_pixmaps[frame_index])
        self.preview_meta.setText(
            f"Frame {frame_index + 1}/{len(self.preview_pixmaps)} | {self.preview_fps:.1f} fps | "
            f"loop={'on' if self.preview_loop else 'off'} | rot={self.media_rotation.value():+.1f} | "
            f"zoom={self.media_zoom.value():.2f} | x={self.media_offset_x.value():+d} y={self.media_offset_y.value():+d}"
        )

        if sync_slider and self.preview_scrubber.value() != frame_index:
            self.preview_scrubber.blockSignals(True)
            self.preview_scrubber.setValue(frame_index)
            self.preview_scrubber.blockSignals(False)

    def refresh_media_preview(self) -> None:
        if prepare_media_clip is None:
            self.set_status("Preview unavailable: media dependencies are missing")
            return

        try:
            payload = self.media_request_payload()
            preview_max_frames = min(int(payload["max_frames"]), 180)
            self.set_status("Generating OLED preview...")
            QApplication.processEvents()

            prepared = prepare_media_clip(
                path_text=str(payload["path"]),
                fps=float(payload["fps"]),
                dither=bool(payload["dither"]),
                threshold=int(payload["threshold"]),
                loop=bool(payload["loop"]),
                max_frames=preview_max_frames,
                rotation_deg=float(payload["rotation"]),
                zoom=float(payload["zoom"]),
                offset_x=int(payload["offset_x"]),
                offset_y=int(payload["offset_y"]),
            )

            self.preview_pixmaps = [self.frame_hex_to_pixmap(frame_hex) for frame_hex in prepared.frames_hex]
            self.preview_loop = prepared.loop
            self.preview_fps = 1.0 / prepared.frame_interval if prepared.frame_interval > 0 else 0.0
            self.preview_interval_ms = max(60, int(round(prepared.frame_interval * 1000.0)))

            self.preview_scrubber.setEnabled(len(self.preview_pixmaps) > 1)
            self.preview_scrubber.setMinimum(0)
            self.preview_scrubber.setMaximum(max(0, len(self.preview_pixmaps) - 1))
            self.set_preview_frame(0)
            self.toggle_preview_playback(self.preview_animate.isChecked())

            self.set_status(
                f"Preview ready: {len(self.preview_pixmaps)} frame(s), {self.preview_fps:.1f} fps"
            )
        except Exception as exc:
            self.preview_timer.stop()
            self.preview_pixmaps = []
            self.preview_scrubber.setEnabled(False)
            self.preview_frame.setText("Preview failed")
            self.preview_meta.setText(str(exc))
            self.set_status(f"Preview failed: {exc}")

    def advance_media_preview(self) -> None:
        if len(self.preview_pixmaps) <= 1:
            self.preview_timer.stop()
            return

        next_index = self.preview_index + 1
        if next_index >= len(self.preview_pixmaps):
            if not self.preview_loop:
                self.preview_timer.stop()
                return
            next_index = 0

        self.set_preview_frame(next_index)

    def on_preview_scrub(self, slider_value: int) -> None:
        if not self.preview_pixmaps:
            return
        self.set_preview_frame(slider_value, sync_slider=False)

    def toggle_preview_playback(self, checked: bool) -> None:
        if checked and len(self.preview_pixmaps) > 1:
            self.preview_timer.start(self.preview_interval_ms)
        else:
            self.preview_timer.stop()

    def ping_server(self) -> None:
        try:
            data = self.api_get("/")
            self.set_status(f"Connected: {data.get('status', 'ok')}")
        except Exception as exc:
            self.set_status(f"Ping failed: {exc}")

    def send_mood(self, mood_id: int, label: str) -> None:
        try:
            self.api_get(f"/set_mood/{mood_id}")
            self.set_status(f"Mood sent: {label}")
        except Exception as exc:
            self.set_status(f"Mood send failed: {exc}")

    def feed(self) -> None:
        try:
            self.api_get("/feed")
            self.set_status("Feed command sent")
        except Exception as exc:
            self.set_status(f"Feed failed: {exc}")

    def send_page(self, page_id: int, label: str) -> None:
        try:
            data = self.api_get(f"/set_page/{page_id}")
            ack = data.get("ack", False)
            detail = data.get("detail", "")
            if ack:
                self.set_status(f"Page switched: {label} ({detail})")
            else:
                self.set_status(f"Page switch failed: {detail}")
        except Exception as exc:
            self.set_status(f"Page switch error: {exc}")

    def enter_config_mode(self) -> None:
        try:
            data = self.api_get("/esp/config_mode")
            if data.get("status") == "sent":
                self.set_status("ESP setup mode requested. Join WiFi 'KoranguDesk'")
            else:
                self.set_status(f"Config mode failed: {data}")
        except Exception as exc:
            self.set_status(f"Config mode error: {exc}")

    def pick_media_for_screen(self) -> None:
        self.browse_media_file()

    def send_media_with_controls(self) -> None:
        try:
            payload = self.media_request_payload()
            self.set_status("Processing media for OLED...")
            data = self.api_post("/media/set", payload)

            if data.get("status") == "ready":
                frames = data.get("frames", 0)
                fps = data.get("fps", 0)
                self.set_status(f"Media ready ({frames} frames @ {fps} fps). Switch to Media Screen.")
            else:
                self.set_status(f"Media processing failed: {data.get('detail', 'unknown error')}")
        except Exception as exc:
            self.set_status(f"Media set failed: {exc}")

    def clear_media_for_screen(self) -> None:
        try:
            data = self.api_post("/media/clear", {})
            if data.get("status") == "cleared":
                self.set_status("Media screen clip cleared")
            else:
                self.set_status(f"Media clear failed: {data}")
        except Exception as exc:
            self.set_status(f"Media clear failed: {exc}")

    def toggle_auto_refresh(self, checked: bool) -> None:
        if checked:
            self.telemetry_timer.start(3000)
            self.set_status("Auto-refresh enabled")
        else:
            self.telemetry_timer.stop()
            self.set_status("Auto-refresh disabled")

    def refresh_telemetry(self) -> None:
        try:
            data = self.api_get("/debug/telemetry")
            counters = data.get("counters", {})
            state = data.get("state", {})

            lines = [
                f"uptime_seconds: {data.get('uptime_seconds')}",
                f"esp_connected: {data.get('esp_connected')}",
                f"esp_current_page: {data.get('esp_current_page')}",
                "",
                "last_state:",
                f"  last_esp_page: {state.get('last_esp_page')}",
                f"  last_lyric_status: {state.get('last_lyric_status')}",
                f"  last_lyric_line: {state.get('last_lyric_line')}",
                f"  last_media_status: {state.get('last_media_status')}",
                f"  last_media_source: {state.get('last_media_source')}",
                f"  last_media_error: {state.get('last_media_error')}",
                f"  last_page_set_detail: {state.get('last_page_set_detail')}",
                "",
                "top_counters:",
            ]

            show_keys = [
                "ws_connects", "ws_disconnects", "ws_messages_tx",
                "ws_messages_rx", "api_set_page_requests", "page_set_ack_success",
                "page_set_ack_failed", "lyric_fetch_attempts", "lyric_fetch_success",
                "lyric_fetch_not_found", "lyric_fetch_error",
                "api_media_set_requests", "media_set_success", "media_set_failed",
                "media_frames_sent", "media_status_sent",
            ]
            for key in show_keys:
                lines.append(f"  {key}: {counters.get(key, 0)}")

            self.telemetry_view.setPlainText("\n".join(lines))
            # We don't overwrite the status bar here to avoid flickering "Ready" out constantly
        except Exception as exc:
            self.set_status(f"Telemetry refresh failed: {exc}")


def main() -> int:
    app = QApplication(sys.argv)

    # Set global application font
    font = QFont("Segoe UI", 10)
    font.setStyleHint(QFont.StyleHint.SansSerif)
    app.setFont(font)

    win = KoranguControlWindow()
    win.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
