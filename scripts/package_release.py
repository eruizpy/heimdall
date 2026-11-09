#!/usr/bin/env python3
"""Build the Heimdall release packages for macOS and Linux (docs/specs/008-release-packages).

    . ~/esp/esp-idf-v6.1/export.sh
    python3 scripts/package_release.py --version 0.2.0              # build firmwares + packages
    python3 scripts/package_release.py --version 0.2.0 --skip-build \\
        --assistant path/a.bin --face path/f.bin                    # package existing images

Output in dist/: heimdall-v<version>-macos.tar.gz, heimdall-v<version>-linux.tar.gz, SHA256SUMS.
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARD = "esp32-2432s028"
FIRMWARE_NAMES = {
    "assistant": f"heimdall-assistant-{BOARD}.bin",
    "face": f"heimdall-face-{BOARD}.bin",
}
OS_INFO = {
    "macos": {
        "label": "macOS",
        "setup": "tools/setup-macos.sh",
        "port_notes": "- macOS: la placa aparece como `/dev/cu.usbserial-*`. Si no aparece, permití el accesorio USB "
                      "en Configuración > Privacidad y seguridad y reconectala.",
        "checksum_command": "shasum -a 256 -c SHA256SUMS",
    },
    "linux": {
        "label": "Linux",
        "setup": "tools/setup-linux.sh",
        "port_notes": "- Linux: la placa aparece como `/dev/ttyUSB0`. Si falta permiso: "
                      "`sudo usermod -aG dialout $USER` y volvé a iniciar sesión.",
        "checksum_command": "sha256sum -c SHA256SUMS",
    },
}
EXECUTABLE = {"flash.sh", "setup-dev.sh", "tools/heimdall_ask.py", "tools/heimdall_face.py"}


def check_environment():
    """Release firmware must never embed the private server URL (spec 003)."""
    if os.environ.get("HEIMDALL_OTA_URL", "").strip():
        sys.exit("HEIMDALL_OTA_URL is set: release firmware must not embed the private server URL. "
                 "Run 'unset HEIMDALL_OTA_URL' and try again.")


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        for block in iter(lambda: file.read(1 << 16), b""):
            digest.update(block)
    return digest.hexdigest()


def write_checksums(directory, names, output):
    lines = [f"{sha256(directory / name)}  {name}\n" for name in sorted(names)]
    output.write_text("".join(lines), encoding="utf-8")
    return output


def assemble(os_name, version, firmware, out_dir):
    """Create dist/heimdall-v<version>-<os>.tar.gz from firmware images {"assistant": path, "face": path}."""
    if os_name not in OS_INFO:
        raise ValueError(f"unknown OS {os_name!r}; expected one of {sorted(OS_INFO)}")
    info = OS_INFO[os_name]
    name = f"heimdall-v{version}-{os_name}"
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        pkg = Path(tmp) / name
        files = {
            "flash.sh": ROOT / "tools/flash.sh",
            "setup-dev.sh": ROOT / info["setup"],
            "tools/heimdall_ask.py": ROOT / "scripts/heimdall_ask.py",
            "tools/heimdall_face.py": ROOT / "scripts/heimdall_face.py",
        }
        for kind, image in firmware.items():
            files[f"firmware/{FIRMWARE_NAMES[kind]}"] = Path(image)
        for rel, src in files.items():
            dest = pkg / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, dest)
            dest.chmod(0o755 if rel in EXECUTABLE else 0o644)

        readme = (ROOT / "tools/package-README.md").read_text(encoding="utf-8").format(
            version=version, os_label=info["label"], port_notes=info["port_notes"],
            checksum_command=info["checksum_command"])
        (pkg / "README.md").write_text(readme, encoding="utf-8")
        (pkg / "README.md").chmod(0o644)
        write_checksums(pkg, [*files, "README.md"], pkg / "SHA256SUMS")
        (pkg / "SHA256SUMS").chmod(0o644)

        def normalize(info_):
            info_.uid = info_.gid = 0
            info_.uname = info_.gname = ""
            return info_

        tarball = out_dir / f"{name}.tar.gz"
        with tarfile.open(tarball, "w:gz") as tar:
            tar.add(pkg, arcname=name, filter=normalize)
    return tarball


def kconfig_default_ota_url():
    text = (ROOT / "main/Kconfig.projbuild").read_text(encoding="utf-8")
    block = text[text.index("config OTA_URL"):]
    return block.split('default "', 1)[1].split('"', 1)[0]


def build_firmware(work):
    """Build both firmwares from source and return their merged images."""
    env = {key: value for key, value in os.environ.items() if key != "HEIMDALL_OTA_URL"}
    subprocess.run(["python3", "scripts/build.py", BOARD, "--name", BOARD, "--language", "es-ES"],
                   cwd=ROOT, env=env, check=True)
    sdkconfig = (ROOT / "sdkconfig").read_text(encoding="utf-8")
    expected = f'CONFIG_OTA_URL="{kconfig_default_ota_url()}"'
    if expected not in sdkconfig:
        sys.exit("The assistant firmware was built with a custom OTA URL; refusing to package it.")
    face = ROOT / "face"
    if not (face / "sdkconfig").exists():
        subprocess.run(["idf.py", "set-target", "esp32"], cwd=face, env=env, check=True)
    subprocess.run(["idf.py", "build"], cwd=face, env=env, check=True)
    subprocess.run(["idf.py", "merge-bin"], cwd=face, env=env, check=True)

    images = {"assistant": ROOT / "build/merged-binary.bin", "face": face / "build/merged-binary.bin"}
    copies = {}
    for kind, image in images.items():
        copies[kind] = Path(work) / FIRMWARE_NAMES[kind]
        shutil.copyfile(image, copies[kind])
    return copies


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", required=True, help="release version, e.g. 0.2.0")
    parser.add_argument("--out", default=str(ROOT / "dist"))
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--assistant", help="assistant merged image (with --skip-build)")
    parser.add_argument("--face", help="face merged image (with --skip-build)")
    args = parser.parse_args(argv)

    check_environment()
    out = Path(args.out)
    with tempfile.TemporaryDirectory() as work:
        if args.skip_build:
            if not (args.assistant and args.face):
                parser.error("--skip-build needs --assistant and --face")
            firmware = {"assistant": Path(args.assistant), "face": Path(args.face)}
        else:
            firmware = build_firmware(work)
        tarballs = [assemble(os_name, args.version, firmware, out) for os_name in OS_INFO]
    sums = write_checksums(out, [t.name for t in tarballs], out / "SHA256SUMS")
    for path in [*tarballs, sums]:
        print(path)


if __name__ == "__main__":
    main()
