"""Check the actual release archives, hashes and package index."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / "dist/manifest.json").read_text(encoding="utf-8"))
platform = json.loads((ROOT / "package_logistic15u_index.json").read_text(encoding="utf-8"))["packages"][0]["platforms"][0]
for kind in ("library", "board"):
    entry = manifest[kind]
    path = ROOT / "dist" / entry["archiveFileName"]
    data = path.read_bytes()
    assert str(len(data)) == entry["size"], path
    assert "SHA-256:" + hashlib.sha256(data).hexdigest() == entry["checksum"], path
    source = ROOT / ("Logistic15U-PuppyBot6" if kind == "library" else "hardware/logistic15u/rp2040")
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        assert sorted(archive.namelist()) == sorted(item["path"] for item in entry["files"])
        for item in entry["files"]:
            name = item["path"]
            assert not any(part.lower() in {"invn", "legacy-imu", ".arduino-test"} for part in Path(name).parts)
            assert Path(name).name not in {"ICM20948wire1.h", "HALwire1.h"}
            content = archive.read(name)
            assert hashlib.sha256(content).hexdigest() == item["sha256"], name
            assert content == (source / name.split("/", 1)[1]).read_bytes(), name
    if kind == "board":
        for key in ("archiveFileName", "url", "size", "checksum"):
            assert platform[key] == entry[key], key
    print(f"PASS {kind}: {len(entry['files'])} files, archive integrity and source hashes match")

assert (ROOT / "main/main.ino").read_bytes() == (ROOT / "Logistic15U-PuppyBot6/examples/OriginalMain/OriginalMain.ino").read_bytes()
print("PASS example matches main; original InvenSense SDK excluded")
