# Korangu Desk

[![CI](https://github.com/N00BSC00B/Korangu-Desk/actions/workflows/ci.yml/badge.svg)](https://github.com/N00BSC00B/Korangu-Desk/actions/workflows/ci.yml)

Korangu Desk is a local-first control stack for an ESP32-based desktop companion. It includes a FastAPI backend, a PyQt6 control panel, and ESP32 firmware.

## Components

- Backend server: FastAPI app with UDP discovery and WebSocket communication.
- Desktop control app: PyQt6 UI for mood, page, feed, and telemetry actions.
- Device firmware: ESP32 Arduino sketch for display modes and real-time interactions.

## Features

- LAN discovery beacon and probe support.
- WebSocket bridge between desktop backend and ESP32.
- REST endpoints for mood, page, feed, and setup-mode actions.
- Real-time telemetry snapshot endpoint.
- Optional lyrics and hardware stats streaming.

## Quick Start (Windows PowerShell)

### 1. Create and activate a virtual environment

```powershell
py -m venv .venv
& .\.venv\Scripts\Activate.ps1
```

### 2. Install dependencies

```powershell
python -m pip install --upgrade pip
pip install -r requirements.txt
```

### 3. Run the backend server

```powershell
python server.py
```

Default host and port:

- Host: 0.0.0.0
- Port: 8765

### 4. Run the desktop control app

```powershell
python korangu_control_pyqt.py
```

## Configuration

The backend port can be changed with an environment variable:

- KORANGU_SERVER_PORT: defaults to 8765

Example:

```powershell
$env:KORANGU_SERVER_PORT = "9000"
python server.py
```

## API Endpoints

| Method | Endpoint | Description |
| --- | --- | --- |
| GET | / | Basic status and server info |
| GET | /set_mood/{mood_id} | Send mood change command |
| GET | /feed | Add energy to the device |
| GET | /set_page/{page_id} | Switch active display page with ACK tracking |
| GET | /esp/config_mode | Ask ESP32 to enter setup mode |
| GET | /debug/telemetry | Inspect server telemetry counters/state |
| WS | /ws | ESP32 real-time command channel |

## Repository Layout

```text
Korangu-Desk/
  korangu_server/          # FastAPI app, routes, watchers, telemetry
  KoranguDesk.ino          # ESP32 firmware (Arduino)
  korangu_control_pyqt.py  # Desktop control UI
  server.py                # Backend entrypoint
  requirements.txt         # Python dependencies
```

## Development Workflow

1. Create a feature branch.
2. Implement and test locally.
3. Run local validation:

```powershell
python -m compileall server.py korangu_server korangu_control_pyqt.py
```

4. Open a pull request using the provided PR template.

## Production Checklist

- Keep dependencies current using Dependabot.
- Protect main branch (required review + passing CI).
- Use tags/releases for firmware and server milestones.
- Keep SECURITY.md process current.

## Contributing and Support

- Contribution process: see CONTRIBUTING.md
- Security reporting: see SECURITY.md
- Code of conduct: see CODE_OF_CONDUCT.md

## License

This project is licensed under the MIT License. See LICENSE.
