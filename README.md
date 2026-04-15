# 🐒 Korangu Desk

[![CI](https://github.com/N00BSC00B/Korangu-Desk/actions/workflows/ci.yml/badge.svg)](https://github.com/N00BSC00B/Korangu-Desk/actions/workflows/ci.yml)

Korangu Desk is a local-first desktop companion powered by an ESP32 and a 0.96" OLED display. Far more than just a digital pet, Korangu acts as a real-time hardware monitor, an offline-synced lyrics engine, a daily dashboard, and an interactive desk toy.

It features a full-stack architecture including a FastAPI backend, a PyQt6 desktop control panel, and highly optimized C++ firmware with a custom 2D physics engine.

## ✨ Core Features

### 🤖 The Companion (Ultra Pro Physics Engine)

- **Procedural Animation:** Smooth, math-driven eye movements (saccades), blinking, and dynamic eyelid angles (eyebrows) for expressive emotions.
- **Autonomous Brain:** Korangu has a metabolism. His energy drains over time, and he requires sleep or "coffee" to recharge. He cycles through idle animations (Happy, Suspicious, Relaxed, etc.) autonomously.
- **Capacitive Touch:** Interact using a simple touch pin.
    - _Single Tap:_ Pet him (boosts energy & triggers combo reactions) or Jump (in mini-game).
    - _Double Tap:_ Fast toggle between Face, Time and Weather Display.
    - _Long Press:_ Opens Menu.

### 🎵 The Lyrics Engine

- **Real-Time Offline Sync:** Uses Windows Media APIs to track your currently playing song with millisecond accuracy—bypassing Spotify's rate limits.
- **Beautiful Rendering:** Automatically fetches `.lrc` files via LRCLIB and pushes them to the ESP32 via WebSockets.
- **Dynamic Typography:** A 3-tier cascading font engine ensures lyrics never get awkwardly truncated, seamlessly falling back to smaller text for wordy verses.
- **Dancing Sprite:** Features a custom 4-frame animated monkey sprite that bops, smiles, and swishes its tail to the beat of your music.

### 💻 The Hardware Cyberdeck

- **Live PC Telemetry:** Transforms the OLED into a cyberpunk-style system monitor.
- **Vitals Tracking:** Displays real-time CPU usage, RAM allocation, and live Network Upload/Download speeds (KB/s) utilizing Python's `psutil`.

### 🌤️ The Daily Dashboard

- **Weather Station:** Integrates with OpenWeatherMap API for live temperature, humidity, "feels like" metrics, and dynamic 1-bit weather icons.
- **3-Day Forecast:** Clean, retro UI showing upcoming weather trends.
- **Dual Clocks:** NTP-synced local time and a dedicated World Clock screen (e.g., India & Sydney).

### 🦖 "Korangu Run" Mini-Game

- **Built-in Arcade:** A fully playable, Chrome-Dino style side-scroller running natively on the ESP32.
- **Physics & Collisions:** Uses AABB collision math and gravity physics. Tap the capacitive wire to jump over obstacles and rack up high scores!

## 🧩 Architecture Components

- **Backend Server (`server.py`):** FastAPI app with UDP discovery, background media/hardware watchers, and WebSocket communication.
- **Desktop Control App (`korangu_control_pyqt.py`):** A sleek PyQt6 UI for triggering moods, changing pages, feeding energy, and viewing telemetry.
- **Device Firmware (`KoranguDesk.ino`):** ESP32 Arduino sketch handling the OLED drawing routines, physics math, Wi-Fi portal, and real-time WebSocket parsing.

## 🚀 Quick Start (Windows PowerShell)

### 1. Hardware Requirements

- ESP32 Microcontroller (e.g., C3 Super Mini or WROOM)
- 0.96" I2C OLED Display (SH1106 or SSD1306)
- Jumper wires & a single exposed wire for the Capacitive Touch Pin.

### 2. Set up the Python Backend

Create and activate a virtual environment:

```powershell
py -m venv .venv
& .\.venv\Scripts\Activate.ps1
```

Install dependencies:

```powershell
python -m pip install --upgrade pip
pip install -r requirements.txt
```

Run the backend server:

```powershell
python server.py
```

_(Default Host: 0.0.0.0 | Port: 8765)_

### 3. Run the Desktop Control App

Open a new terminal, activate your environment, and launch the UI:

```powershell
python korangu_control_pyqt.py
```

### 4. Flash the ESP32

- Open `KoranguDesk.ino` in the Arduino IDE.
- Install required libraries: `WebSockets` by Markus Sattler, `Adafruit GFX`, `Adafruit SH110X`, and `Arduino_JSON`.
- Flash to your board. On first boot, hold the touch pin for 3 seconds to launch the **Config Portal** (Connect to the `KoranguDesk` Wi-Fi network and go to `192.168.4.1` to enter your home Wi-Fi and OpenWeather API details).

## ⚙️ Configuration

The backend port can be changed with an environment variable:

- `KORANGU_SERVER_PORT`: defaults to 8765

Example:

```powershell
$env:KORANGU_SERVER_PORT = "9000"
python server.py
```

## 📡 API Endpoints

| Method | Endpoint              | Description                                  |
| ------ | --------------------- | -------------------------------------------- |
| GET    | `/`                   | Basic status and server info                 |
| GET    | `/set_mood/{mood_id}` | Send mood change command                     |
| GET    | `/feed`               | Add energy to the device                     |
| GET    | `/set_page/{page_id}` | Switch active display page with ACK tracking |
| GET    | `/esp/config_mode`    | Ask ESP32 to enter setup mode                |
| GET    | `/debug/telemetry`    | Inspect server telemetry counters/state      |
| WS     | `/ws`                 | ESP32 real-time command & data channel       |

## 📁 Repository Layout

```text
Korangu-Desk/
  korangu_server/          # FastAPI app, routes, watchers, telemetry
  KoranguDesk.ino          # ESP32 firmware (Arduino C++)
  korangu_control_pyqt.py  # Desktop control UI
  server.py                # Backend entrypoint
  requirements.txt         # Python dependencies
```

## 🛠️ Development Workflow

1. Create a feature branch.
2. Implement and test locally.
3. Run local validation:

```powershell
python -m compileall server.py korangu_server korangu_control_pyqt.py
```

4. Open a pull request using the provided PR template.

## 🛡️ Production Checklist

- Keep dependencies current using Dependabot.
- Protect main branch (required review + passing CI).
- Use tags/releases for firmware and server milestones.
- Keep `SECURITY.md` process current.

## 🤝 Contributing and Support

- Contribution process: see [CONTRIBUTING](CONTRIBUTING.md)
- Security reporting: see [SECURITY](SECURITY.m)
- Code of conduct: see [CODE OF CONDUCT](CODE_OF_CONDUCT.md)

## 📄 License

This project is licensed under the MIT License. See **[LICENSE](LICENSE)**.
