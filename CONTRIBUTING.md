# Contributing

Thanks for contributing to Korangu Desk.

## Prerequisites

- Python 3.11+
- Windows PowerShell (recommended for local setup)
- ESP32 toolchain (Arduino IDE) if you are changing firmware

## Local Setup

```powershell
py -m venv .venv
& .\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
```

## Development Steps

1. Create a branch from main.
2. Make focused changes.
3. Run checks before pushing:

```powershell
python -m compileall server.py korangu_server korangu_control_pyqt.py
```

4. Update docs if behavior changed.
5. Open a pull request.

## Commit Style

Use clear, action-focused commit messages. Conventional commit style is recommended.

Examples:

- feat: add websocket retry backoff metric
- fix: handle page ack timeout when device reconnects
- docs: document local setup and release process

## Pull Request Expectations

- Explain what changed and why.
- Include manual test notes.
- Keep PRs reasonably scoped.
- Ensure CI passes.

## Firmware Changes

For changes in KoranguDesk.ino:

- Note board type and core version used.
- Document added libraries in the PR description.
- Include a short validation summary (boot, connect, page switch, telemetry).
