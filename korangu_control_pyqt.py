import sys
from typing import Dict, List, Tuple

import requests

try:
    from PyQt6.QtCore import Qt, QTimer
    from PyQt6.QtGui import QFont
    from PyQt6.QtWidgets import (
        QApplication,
        QCheckBox,
        QGridLayout,
        QGroupBox,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QPushButton,
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
    border: 1px solid #dcdde1;
    border-radius: 4px;
    padding: 6px;
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
                f"  last_page_set_detail: {state.get('last_page_set_detail')}",
                "",
                "top_counters:",
            ]

            show_keys = [
                "ws_connects", "ws_disconnects", "ws_messages_tx",
                "ws_messages_rx", "api_set_page_requests", "page_set_ack_success",
                "page_set_ack_failed", "lyric_fetch_attempts", "lyric_fetch_success",
                "lyric_fetch_not_found", "lyric_fetch_error",
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
