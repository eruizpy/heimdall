#!/usr/bin/env python3
"""Send a typed question to Heimdall over the USB serial port and print its reply.

The esp32-2432s028 board forwards each serial line to the server as a text query
(Application::SendTextQuery), so this works without a microphone.

Usage (with the ESP-IDF environment, which provides pyserial):
    python3 scripts/heimdall_ask.py "¿Qué hora es?" [--port /dev/cu.usbserial-10] [--wait 25]
"""

import argparse
import glob
import re
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: run '. ~/esp/esp-idf-v6.1/export.sh' first or 'pip install pyserial'")

ANSI = re.compile(r"\x1b\[[0-9;]*m")
INTERESTING = re.compile(r"Text query|>> |<< |State: |Text query ignored|^E \(")


def clean_line(raw):
    """Decode one log line from the board and strip color codes and the trailing CR."""
    return ANSI.sub("", raw.decode("utf-8", errors="replace")).strip()


def is_relevant(line):
    """Lines worth showing: the query, what the server understood, the reply and errors."""
    return bool(line) and bool(INTERESTING.search(line))


def is_reply_done(line):
    return "State: speaking -> idle" in line


def default_port():
    candidates = sorted(
        glob.glob("/dev/cu.usbserial-*") + glob.glob("/dev/cu.wchusbserial*") + glob.glob("/dev/ttyUSB*")
    )
    if not candidates:
        sys.exit("No serial port found: connect the board or pass --port")
    return candidates[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("question")
    parser.add_argument("--port", default=None)
    parser.add_argument("--wait", type=float, default=25.0, help="seconds to wait for the reply")
    args = parser.parse_args()

    port = args.port or default_port()
    # Opening without touching DTR/RTS explicitly keeps the board running (no reset).
    with serial.Serial(port, 115200, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.write((args.question.replace("\n", " ") + "\n").encode("utf-8"))
        ser.flush()

        buffer = b""
        deadline = time.time() + args.wait
        spoke = False
        while time.time() < deadline:
            buffer += ser.read(4096)
            *lines, buffer = buffer.split(b"\n")
            for raw in lines:
                line = clean_line(raw)
                if is_relevant(line):
                    print(line)
                if is_reply_done(line):
                    spoke = True
            if spoke:
                break
        if not spoke:
            print(f"(no complete reply within {args.wait:.0f}s)")


if __name__ == "__main__":
    main()
