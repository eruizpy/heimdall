"""Tests for Heimdall-specific tooling (specs 001, 002 and 003 in docs/specs)."""

import argparse
import contextlib
import importlib.util
import io
import json
import os
import sys
import types
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]


def load_script(name):
    """Import a script from scripts/ without needing a real serial port library."""
    fake_serial = types.ModuleType("serial")
    fake_serial.Serial = mock.MagicMock()
    with mock.patch.dict(sys.modules, {"serial": fake_serial}):
        spec = importlib.util.spec_from_file_location(name, ROOT / "scripts" / f"{name}.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
    return module


build_spec = importlib.util.spec_from_file_location("build", ROOT / "scripts/build.py")
build = importlib.util.module_from_spec(build_spec)
build_spec.loader.exec_module(build)


class OtaUrlFromEnvironmentTests(unittest.TestCase):
    """Spec 003, CA1 and CA2."""

    BOARD = "esp32-2432s028"

    @contextlib.contextmanager
    def mock_idf(self):
        with (
            mock.patch.object(build, "_prepare_target"),
            mock.patch.object(build, "_configure_build") as configure,
            mock.patch.object(build, "_validate_configured_symbols"),
            mock.patch.object(build, "_validate_configured_options"),
            mock.patch.object(build, "_run_idf"),
            mock.patch.object(build, "merge_bin"),
            contextlib.redirect_stdout(io.StringIO()),
            contextlib.redirect_stderr(io.StringIO()),
        ):
            yield configure

    def fragment(self, env):
        with mock.patch.dict(os.environ, env, clear=False), self.mock_idf() as configure:
            if "HEIMDALL_OTA_URL" not in env:
                os.environ.pop("HEIMDALL_OTA_URL", None)
            build.build_board(self.BOARD, name_filter=self.BOARD, idf_version=(6, 1, 0))
        configure.assert_called_once()
        return configure.call_args.args[1]

    def test_env_url_is_added_to_sdkconfig(self):
        fragment = self.fragment({"HEIMDALL_OTA_URL": "https://example.com/ota/"})
        self.assertIn('CONFIG_OTA_URL="https://example.com/ota/"', fragment)

    def test_no_env_means_no_ota_override(self):
        fragment = self.fragment({})
        self.assertFalse([item for item in fragment if item.startswith("CONFIG_OTA_URL")])

    def test_blank_env_is_ignored(self):
        fragment = self.fragment({"HEIMDALL_OTA_URL": "   "})
        self.assertFalse([item for item in fragment if item.startswith("CONFIG_OTA_URL")])

    def test_board_config_does_not_pin_a_server(self):
        config = json.loads((ROOT / "main/boards" / self.BOARD / "config.json").read_text(encoding="utf-8"))
        for entry in config["builds"]:
            for item in entry.get("sdkconfig_append", []):
                self.assertFalse(item.startswith("CONFIG_OTA_URL"), item)


class FaceScriptTests(unittest.TestCase):
    """Spec 002, CA6."""

    @classmethod
    def setUpClass(cls):
        cls.face = load_script("heimdall_face")

    def message(self, kind, *values, role="assistant"):
        return self.face.build_message(argparse.Namespace(kind=kind, values=list(values), role=role))

    def test_build_message_for_each_command(self):
        self.assertEqual({"t": "emo", "v": "happy"}, self.message("emo", "happy"))
        self.assertEqual({"t": "status", "v": "Hola"}, self.message("status", "Hola"))
        self.assertEqual({"t": "text", "role": "user", "v": "¿Qué tal?"},
                         self.message("text", "¿Qué", "tal?", role="user"))
        self.assertEqual({"t": "look", "x": -0.5, "y": 0.0}, self.message("look", "-0.5"))
        self.assertEqual({"t": "look", "x": 1.0, "y": -1.0}, self.message("look", "1", "-1"))
        self.assertEqual({"t": "bright", "v": 40}, self.message("bright", "40"))
        self.assertEqual({"t": "ping"}, self.message("ping"))
        self.assertEqual({"t": "bright", "v": 5}, self.message("raw", '{"t":"bright","v":5}'))

    def test_messages_are_single_json_lines(self):
        sent = []
        fake = mock.MagicMock()
        fake.write.side_effect = sent.append
        face = self.face.Face.__new__(self.face.Face)
        face.ser, face.buf = fake, b""
        face.send({"t": "text", "v": "Línea\ncon salto"})
        self.assertEqual(1, len(sent))
        self.assertTrue(sent[0].endswith(b"\n"))
        self.assertEqual(1, sent[0].count(b"\n"))
        self.assertEqual("Línea\ncon salto", json.loads(sent[0].decode("utf-8"))["v"])

    def test_events_skip_logs_and_partial_lines(self):
        face = self.face.Face.__new__(self.face.Face)
        face.buf = b""
        face.ser = mock.MagicMock()
        face.ser.read.side_effect = [
            b'W (100) touch: calibration\n{"t":"hello","fw":"x"}\n{"t":"to',
            b'uch","ev":"tap","x":1,"y":2}\n{broken\n',
        ]
        self.assertEqual([{"t": "hello", "fw": "x"}], face.events())
        self.assertEqual([{"t": "touch", "ev": "tap", "x": 1, "y": 2}], face.events())

    def test_react_to_touch_events(self):
        face = mock.MagicMock()
        self.face.react(face, {"t": "touch", "ev": "tap", "x": 10, "y": 20})
        sent = [call.args[0] for call in face.send.call_args_list]
        self.assertIn({"t": "emo", "v": "happy"}, sent)
        self.assertTrue(any(m.get("t") == "text" and "10" in m["v"] for m in sent))

        face.reset_mock()
        self.face.react(face, {"t": "touch", "ev": "swipe_left", "x": 0, "y": 0})
        self.assertIn({"t": "look", "x": -1, "y": 0}, [c.args[0] for c in face.send.call_args_list])


class AskScriptTests(unittest.TestCase):
    """Spec 001, CA5."""

    @classmethod
    def setUpClass(cls):
        cls.ask = load_script("heimdall_ask")

    def test_clean_line_strips_colors_and_decodes_utf8(self):
        raw = "\x1b[0;32mI (29981) Application: >> ¿Qué hora es?\x1b[0m\r".encode("utf-8")
        self.assertEqual("I (29981) Application: >> ¿Qué hora es?", self.ask.clean_line(raw))

    def test_relevant_lines(self):
        relevant = [
            "I (1) Application: Text query: hola",
            "I (2) Application: >> hola",
            "I (3) Application: << Hola, ¿cómo estás?",
            "I (4) StateMachine: State: idle -> speaking",
            "W (5) Application: Text query ignored: device is not idle",
            "E (6) WS: Websocket error",
        ]
        noise = ["I (7) SystemInfo: free sram: 1000", "I (8) wifi:Set ps type: 0", ""]
        for line in relevant:
            self.assertTrue(self.ask.is_relevant(line), line)
        for line in noise:
            self.assertFalse(self.ask.is_relevant(line), line)

    def test_reply_finished_detection(self):
        self.assertTrue(self.ask.is_reply_done("I (9) StateMachine: State: speaking -> idle"))
        self.assertFalse(self.ask.is_reply_done("I (9) StateMachine: State: idle -> speaking"))


if __name__ == "__main__":
    unittest.main()
