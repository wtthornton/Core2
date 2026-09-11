"""Probe an M5Stack Core2 over USB-UART (esptool). Read-only by default."""

from __future__ import annotations

import argparse
import sys

try:
    import serial.tools.list_ports
except ImportError:  # pragma: no cover
    serial = None  # type: ignore[assignment]

DEFAULT_PORT = "COM4"


def list_ports() -> None:
    if serial is None:
        print("pyserial is not installed; pip install pyserial", file=sys.stderr)
        return
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return
    for p in ports:
        print(f"{p.device}\t{p.description}\t{p.hwid}")


def run_esptool(port: str, *args: str) -> int:
    import subprocess

    argv = [sys.executable, "-m", "esptool", "--chip", "esp32", "--port", port, *args]
    print(" ".join(argv), flush=True)
    completed = subprocess.run(argv, check=False)
    return int(completed.returncode)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT, help="COM port (default COM4)")
    parser.add_argument("--list", action="store_true", help="List serial ports and exit")
    parser.add_argument(
        "--partitions",
        action="store_true",
        help="Also dump 3 KiB from flash offset 0x8000",
    )
    ns = parser.parse_args()
    if ns.list:
        list_ports()
        return 0
    print("=== ports ===")
    list_ports()
    rc = run_esptool(ns.port, "chip-id")
    if rc:
        return rc
    rc = run_esptool(ns.port, "flash-id")
    if rc:
        return rc
    if ns.partitions:
        rc = run_esptool(ns.port, "read-flash", "0x8000", "3072", "partitions.bin")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
