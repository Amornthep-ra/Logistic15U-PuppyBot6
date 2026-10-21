"""Build reproducible Arduino distribution ZIPs using only project sources."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.1.0"
BASE = "https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main"


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def archive(source, name, prefix):
    target = ROOT / "dist" / name
    members = []
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as out:
        for path in sorted(source.rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(source).as_posix()
            if any(part.lower() in {"invn", "legacy-imu", ".git", ".arduino-test"} for part in path.parts):
                raise ValueError(f"Excluded source in distribution: {path}")
            if path.name in {"ICM20948wire1.h", "HALwire1.h"}:
                raise ValueError(f"Legacy SDK adapter in distribution: {path}")
            data = path.read_bytes()
            info = zipfile.ZipInfo(f"{prefix}/{relative}", (2026, 10, 21, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            out.writestr(info, data)
            members.append({"path": info.filename, "sha256": hashlib.sha256(data).hexdigest()})
    data = target.read_bytes()
    return {"archiveFileName": name, "url": f"{BASE}/dist/{name}",
            "checksum": "SHA-256:" + hashlib.sha256(data).hexdigest(),
            "size": str(len(data)), "files": members}


def main():
    (ROOT / "dist").mkdir(exist_ok=True)
    library = archive(ROOT / "Logistic15U-PuppyBot6", f"Logistic15U-PuppyBot6-{VERSION}.zip", "Logistic15U-PuppyBot6")
    board = archive(ROOT / "hardware/logistic15u/rp2040", f"logistic15u-rp2040-{VERSION}.zip", f"logistic15u-rp2040-{VERSION}")
    platform = {"name": "Logistic15U-PuppyBot6", "architecture": "rp2040", "version": VERSION,
                "category": "Contributed", "help": {"online": "https://github.com/Amornthep-ra/Logistic15U-PuppyBot6"},
                "boards": [{"name": "Logistic15U-PuppyBot6"}], "toolsDependencies": []}
    platform.update({key: value for key, value in board.items() if key != "files"})
    package = {"name": "logistic15u", "maintainer": "PrinceBot", "websiteURL": "https://github.com/Amornthep-ra/Logistic15U-PuppyBot6",
               "email": "", "help": platform["help"], "platforms": [platform], "tools": []}
    write_json(ROOT / "package_logistic15u_index.json", {"packages": [package]})
    write_json(ROOT / "dist/manifest.json", {"version": VERSION, "base_core": "rp2040:rp2040@6.2.0", "library": library, "board": board})
    for item in (library, board):
        print(f"{item['archiveFileName']}: {item['size']} bytes {item['checksum']}")


if __name__ == "__main__":
    main()
