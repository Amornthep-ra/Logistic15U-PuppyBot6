"""Exercise public RGB mode calls with real calibration, storage and display headers."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
output = root / ".arduino-test/rgb-mode-tests"
output.mkdir(parents=True, exist_ok=True)
exe = output / "mode.exe"
gfx = Path.home() / "Documents/Arduino/libraries/Adafruit_GFX_Library"
subprocess.run([
    "g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
    "-I" + str(root / "tests/calibration/fakes"),
    "-I" + str(root / "tests/rgb/fakes"), "-I" + str(gfx),
    "-I" + str(root / "Logistic15U-PuppyBot6/src"),
    str(Path(__file__).with_name("test_mode.cpp")), "-o", str(exe),
], check=True)
logs = []
for args in ([], ["saved"]):
    result = subprocess.run([str(exe), *args], capture_output=True, text=True)
    logs.append(result.stdout + result.stderr)
    print(logs[-1], end="")
    (output / "results.txt").write_text("".join(logs), encoding="utf-8")
    result.check_returncode()
