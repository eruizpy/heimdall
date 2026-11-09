"""Tests for the release packages (spec 008-release-packages)."""

import hashlib
import importlib.util
import os
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
VERSION = "9.9.9"
EXPECTED_FILES = {
    "README.md",
    "SHA256SUMS",
    "flash.sh",
    "setup-dev.sh",
    "firmware/heimdall-assistant-esp32-2432s028.bin",
    "firmware/heimdall-face-esp32-2432s028.bin",
    "tools/heimdall_ask.py",
    "tools/heimdall_face.py",
}

spec = importlib.util.spec_from_file_location("package_release", ROOT / "scripts/package_release.py")
package_release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package_release)


class PackageFixture(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.firmware = {
            "assistant": self.tmp / "assistant.bin",
            "face": self.tmp / "face.bin",
        }
        self.firmware["assistant"].write_bytes(b"A" * 1024)
        self.firmware["face"].write_bytes(b"F" * 2048)
        self.out = self.tmp / "dist"

    def assemble(self, os_name):
        return package_release.assemble(os_name, VERSION, self.firmware, self.out)

    def extract(self, os_name):
        tarball = self.assemble(os_name)
        dest = self.tmp / f"x-{os_name}"
        with tarfile.open(tarball) as tar:
            tar.extractall(dest, filter="tar")
        return dest / f"heimdall-v{VERSION}-{os_name}"


class PackageContentTests(PackageFixture):
    """CA1-CA4."""

    def test_tarball_name_and_contents(self):
        for os_name in ("macos", "linux"):
            with self.subTest(os=os_name):
                tarball = self.assemble(os_name)
                self.assertEqual(f"heimdall-v{VERSION}-{os_name}.tar.gz", tarball.name)
                prefix = f"heimdall-v{VERSION}-{os_name}/"
                with tarfile.open(tarball) as tar:
                    files = {m.name[len(prefix):] for m in tar.getmembers() if m.isfile()}
                    root = prefix.rstrip("/")
                    self.assertTrue(all(m.name == root or m.name.startswith(prefix) for m in tar.getmembers()))
                self.assertEqual(EXPECTED_FILES, files)

    def test_scripts_are_executable(self):
        pkg = self.extract("linux")
        for name in ("flash.sh", "setup-dev.sh", "tools/heimdall_ask.py", "tools/heimdall_face.py"):
            self.assertTrue(os.access(pkg / name, os.X_OK), name)

    def test_firmware_is_copied_unchanged(self):
        pkg = self.extract("macos")
        self.assertEqual(b"A" * 1024, (pkg / "firmware/heimdall-assistant-esp32-2432s028.bin").read_bytes())
        self.assertEqual(b"F" * 2048, (pkg / "firmware/heimdall-face-esp32-2432s028.bin").read_bytes())

    def test_checksums_match_the_files(self):
        pkg = self.extract("macos")
        lines = (pkg / "SHA256SUMS").read_text().splitlines()
        listed = {}
        for line in lines:
            digest, name = line.split("  ", 1)
            listed[name] = digest
        self.assertEqual(EXPECTED_FILES - {"SHA256SUMS"}, set(listed))
        for name, digest in listed.items():
            self.assertEqual(hashlib.sha256((pkg / name).read_bytes()).hexdigest(), digest, name)

    def test_setup_script_matches_the_os(self):
        mac = (self.extract("macos") / "setup-dev.sh").read_text()
        linux = (self.extract("linux") / "setup-dev.sh").read_text()
        self.assertIn("brew install", mac)
        self.assertNotIn("apt-get", mac)
        self.assertIn("apt-get install", linux)
        self.assertNotIn("brew install", linux)

    def test_readme_explains_flash_and_server_url(self):
        readme = (self.extract("linux") / "README.md").read_text()
        self.assertIn("./flash.sh assistant", readme)
        self.assertIn("./flash.sh face", readme)
        self.assertIn("Custom OTA URL", readme)
        self.assertIn(VERSION, readme)

    def test_unknown_os_is_rejected(self):
        with self.assertRaises(ValueError):
            self.assemble("windows")

    def test_refuses_to_package_with_private_server_url(self):
        with mock.patch.dict(os.environ, {"HEIMDALL_OTA_URL": "https://example.com/ota/"}):
            with self.assertRaises(SystemExit):
                package_release.check_environment()
        with mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("HEIMDALL_OTA_URL", None)
            package_release.check_environment()  # no error


class FlashScriptTests(PackageFixture):
    """CA5."""

    def run_flash(self, pkg, *args):
        return subprocess.run(["sh", str(pkg / "flash.sh"), *args], cwd=self.tmp, capture_output=True,
                              text=True, timeout=30)

    def test_dry_run_shows_the_esptool_command(self):
        pkg = self.extract("macos")
        result = self.run_flash(pkg, "face", "--port", "/dev/fake0", "--dry-run")
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("esptool==5.6.0", result.stdout)
        self.assertIn("pyserial==3.5", result.stdout)
        command = next(line for line in result.stdout.splitlines() if "write-flash" in line)
        self.assertIn("--port /dev/fake0", command)
        self.assertIn("--chip esp32", command)
        self.assertIn("write-flash 0x0", command)
        self.assertIn("firmware/heimdall-face-esp32-2432s028.bin", command)
        self.assertFalse((pkg / ".venv").exists(), "dry run must not create the venv")

    def test_assistant_target(self):
        pkg = self.extract("linux")
        result = self.run_flash(pkg, "assistant", "--port", "/dev/ttyFAKE", "--dry-run")
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("firmware/heimdall-assistant-esp32-2432s028.bin", result.stdout)

    def test_rejects_tampered_firmware(self):
        pkg = self.extract("macos")
        (pkg / "firmware/heimdall-face-esp32-2432s028.bin").write_bytes(b"tampered")
        result = self.run_flash(pkg, "face", "--port", "/dev/fake0", "--dry-run")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("checksum", (result.stdout + result.stderr).lower())

    def test_usage_errors(self):
        pkg = self.extract("macos")
        self.assertEqual(2, self.run_flash(pkg).returncode)
        self.assertEqual(2, self.run_flash(pkg, "toaster", "--dry-run").returncode)


class SetupScriptTests(unittest.TestCase):
    """CA6."""

    def dry_run(self, script):
        result = subprocess.run(["sh", str(ROOT / "tools" / script), "--dry-run"], capture_output=True, text=True,
                                timeout=30)
        self.assertEqual(0, result.returncode, result.stderr)
        return result.stdout

    def test_common_steps(self):
        for script in ("setup-macos.sh", "setup-linux.sh"):
            with self.subTest(script=script):
                out = self.dry_run(script)
                self.assertIn("release/v6.1", out)
                self.assertIn("install.sh esp32", out)
                self.assertIn("clang-format==23.1.5", out)
                self.assertIn("core.hooksPath .githooks", out)

    def test_os_specific_package_managers(self):
        self.assertIn("brew install", self.dry_run("setup-macos.sh"))
        linux = self.dry_run("setup-linux.sh")
        self.assertIn("apt-get install", linux)
        self.assertIn("dialout", linux)

    def test_shell_syntax(self):
        for script in sorted((ROOT / "tools").glob("*.sh")) + [ROOT / "scripts/ci.sh"]:
            with self.subTest(script=script.name):
                result = subprocess.run(["sh", "-n", str(script)], capture_output=True, text=True)
                self.assertEqual(0, result.returncode, result.stderr)


if __name__ == "__main__":
    unittest.main()
