#!/usr/bin/env python3
"""Talk to the Heimdall face (CYD running face/) over USB, acting as the robot core.

Protocol: docs/heimdall/face-protocol.md. Needs pyserial (load the ESP-IDF environment).

    python3 scripts/heimdall_face.py monitor               # print touch events
    python3 scripts/heimdall_face.py send emo happy        # one command
    python3 scripts/heimdall_face.py send text "Hola, soy Heimdall"
    python3 scripts/heimdall_face.py send look -0.8 0
    python3 scripts/heimdall_face.py send raw '{"t":"bright","v":40}'
    python3 scripts/heimdall_face.py demo                  # cycle expressions and react to touches
"""

import argparse
import glob
import json
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is missing: run '. ~/esp/esp-idf-v6.1/export.sh' first or 'pip install pyserial'")

EMOTIONS = ["neutral", "happy", "sad", "angry", "surprised", "thinking", "sleepy", "loving", "winking", "embarrassed"]
SWIPE_LOOK = {"swipe_left": (-1, 0), "swipe_right": (1, 0), "swipe_up": (0, -1), "swipe_down": (0, 1)}


def default_port():
    ports = sorted(glob.glob("/dev/cu.usbserial-*") + glob.glob("/dev/cu.wchusbserial*") + glob.glob("/dev/ttyUSB*"))
    if not ports:
        sys.exit("No serial port found: connect the face or pass --port")
    return ports[0]


class Face:
    def __init__(self, port):
        # Opening without touching DTR/RTS keeps the board running (no reset).
        self.ser = serial.Serial(port, 115200, timeout=0.05)
        self.buf = b""

    def send(self, msg):
        self.ser.write((json.dumps(msg, ensure_ascii=False) + "\n").encode("utf-8"))
        self.ser.flush()

    def events(self):
        """Protocol messages received since the last call (log lines are skipped)."""
        self.buf += self.ser.read(4096)
        *lines, self.buf = self.buf.split(b"\n")
        out = []
        for raw in lines:
            line = raw.decode("utf-8", errors="replace").strip()
            if line.startswith("{"):
                try:
                    out.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
        return out


def build_message(args):
    kind, values = args.kind, args.values
    if kind == "raw":
        return json.loads(values[0])
    if kind in ("emo", "status"):
        return {"t": kind, "v": values[0] if values else ""}
    if kind == "text":
        return {"t": "text", "role": args.role, "v": " ".join(values)}
    if kind == "look":
        return {"t": "look", "x": float(values[0]), "y": float(values[1]) if len(values) > 1 else 0.0}
    if kind == "bright":
        return {"t": "bright", "v": int(values[0])}
    return {"t": kind}  # blink, ping, calibrate


def cmd_send(face, args):
    face.send(build_message(args))
    end = time.time() + 1.0
    while time.time() < end:
        for ev in face.events():
            print(json.dumps(ev, ensure_ascii=False))


def cmd_monitor(face, args):
    print("Listening for face events (Ctrl+C to stop)...")
    while True:
        for ev in face.events():
            print(time.strftime("%H:%M:%S"), json.dumps(ev, ensure_ascii=False), flush=True)


def react(face, ev):
    """What the core would do with a touch event."""
    kind = ev.get("ev")
    if kind == "tap":
        face.send({"t": "emo", "v": "happy"})
        face.send({"t": "text", "role": "assistant", "v": f"¡Me tocaste en ({ev['x']}, {ev['y']})!"})
    elif kind == "long":
        face.send({"t": "emo", "v": "sleepy"})
        face.send({"t": "text", "role": "system", "v": "Toque largo"})
    elif kind in SWIPE_LOOK:
        x, y = SWIPE_LOOK[kind]
        face.send({"t": "look", "x": x, "y": y})
        face.send({"t": "text", "role": "user", "v": kind.replace("_", " ")})


def cmd_demo(face, args):
    print("Demo: cycling expressions; touch the screen to see events (Ctrl+C to stop)...")
    face.send({"t": "status", "v": "Demo de la cara"})
    i, next_change = 0, 0.0
    while True:
        now = time.time()
        if now >= next_change:
            emo = EMOTIONS[i % len(EMOTIONS)]
            face.send({"t": "emo", "v": emo})
            face.send({"t": "text", "role": "assistant", "v": emo})
            i += 1
            next_change = now + args.interval
        for ev in face.events():
            print(time.strftime("%H:%M:%S"), json.dumps(ev, ensure_ascii=False), flush=True)
            if ev.get("t") == "touch":
                react(face, ev)
                next_change = time.time() + args.interval
        time.sleep(0.02)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default=None)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("monitor")
    demo = sub.add_parser("demo")
    demo.add_argument("--interval", type=float, default=3.0, help="seconds per expression")
    send = sub.add_parser("send")
    send.add_argument("kind", choices=["emo", "status", "text", "look", "blink", "bright", "calibrate", "ping", "raw"])
    send.add_argument("values", nargs="*")
    send.add_argument("--role", default="assistant", choices=["assistant", "user", "system"])
    args = parser.parse_args()

    face = Face(args.port or default_port())
    try:
        {"monitor": cmd_monitor, "demo": cmd_demo, "send": cmd_send}[args.command](face, args)
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
