"""Build, upload, and monitor Core2 firmware via PlatformIO."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

DEFAULT_PORT = "COM4"
FIRMWARE_DIR = Path(__file__).resolve().parents[1] / "firmware"


def pio(*args: str) -> int:
    argv = [sys.executable, "-m", "platformio", *args]
    print(" ".join(argv), flush=True)
    completed = subprocess.run(argv, check=False, cwd=str(FIRMWARE_DIR))
    return int(completed.returncode)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        choices=("build", "upload", "monitor"),
        help="PlatformIO target",
    )
    parser.add_argument("--port", default=DEFAULT_PORT, help="COM port (default COM4)")
    ns = parser.parse_args()
    if not FIRMWARE_DIR.is_dir():
        print(f"missing firmware dir: {FIRMWARE_DIR}", file=sys.stderr)
        return 1
    if ns.command == "build":
        return pio("run")
    if ns.command == "upload":
        return pio("run", "-t", "upload", "--upload-port", ns.port)
    return pio("device", "monitor", "--port", ns.port, "--baud", "115200")


if __name__ == "__main__":
    raise SystemExit(main())
