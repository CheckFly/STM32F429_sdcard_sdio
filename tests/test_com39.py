#!/usr/bin/env python3
"""COM39 serial smoke test for the STM32 SD card presence protocol.

Reads lines from the board's virtual COM port and optionally checks that
expected markers are observed. Run it while inserting/removing the SD card.

Requires pyserial:
    python -m pip install pyserial
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError as exc:  # pragma: no cover - dependency check
    raise SystemExit(
        "pyserial is required. Install it with: python -m pip install pyserial"
    ) from exc


DEFAULT_CONFIG = Path(__file__).resolve().parent / "config" / "test_config.json"


def load_config(path: Path) -> dict:
    if path.exists():
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    return {}


def main() -> int:
    config = load_config(DEFAULT_CONFIG)

    parser = argparse.ArgumentParser(description="Read and validate COM39 SD-card messages.")
    parser.add_argument("--port", default=config.get("port", "COM39"))
    parser.add_argument("--baud", type=int, default=config.get("baudrate", 115200))
    parser.add_argument("--duration", type=int, default=config.get("duration_seconds", 20))
    parser.add_argument("--log", default="")
    parser.add_argument(
        "--expect",
        action="append",
        default=config.get("expected_patterns", []),
        help="Expected substring; repeat for multiple patterns.",
    )
    args = parser.parse_args()

    expected = list(dict.fromkeys(args.expect or []))
    seen_lines: list[str] = []
    deadline = time.monotonic() + args.duration

    print(f"Opening {args.port} at {args.baud} baud for {args.duration}s")
    if expected:
        print("Expected: " + ", ".join(expected))
    print("Insert/remove the SD card now...")

    try:
        with serial.Serial(
            args.port,
            args.baud,
            timeout=0.5,
            rtscts=False,
            dsrdtr=False,
        ) as port:
            while time.monotonic() < deadline:
                raw = port.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                stamp = time.strftime("%Y-%m-%d %H:%M:%S")
                output = f"{stamp} {line}"
                print(output)
                seen_lines.append(line)
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        return 2

    if args.log:
        log_path = Path(args.log)
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text("\n".join(seen_lines) + "\n", encoding="utf-8")

    if not seen_lines:
        print("FAIL: no serial lines received.", file=sys.stderr)
        return 1

    missing = [pattern for pattern in expected if not any(pattern in line for line in seen_lines)]
    if missing:
        print("FAIL: missing patterns: " + ", ".join(missing), file=sys.stderr)
        return 1

    print("PASS: all expected patterns observed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
