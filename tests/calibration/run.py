"""Run actual calibration/storage/rendering code against sensor, EEPROM and TFT doubles."""
from pathlib import Path
import subprocess
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
output = ROOT / ".arduino-test/calibration-tests"
output.mkdir(parents=True, exist_ok=True)
gfx = Path.home() / "Documents/Arduino/libraries/Adafruit_GFX_Library"
executable = output / "calibration.exe"
subprocess.run([
    "g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
    "-I" + str(HERE / "fakes"), "-I" + str(ROOT / "tests/rgb/fakes"),
    "-I" + str(gfx), "-I" + str(ROOT / "Logistic15U-PuppyBot6/src"),
    str(HERE / "test_calibration.cpp"), "-o", str(executable),
], check=True)
result = subprocess.run([str(executable), str(output)], text=True, capture_output=True)
print(result.stdout + result.stderr, end="")
(output / "results.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
result.check_returncode()
for path in output.glob("*.ppm"):
    image = Image.open(path)
    image.save(path.with_suffix(".png"))
    image.resize((image.width * 4, image.height * 4), Image.Resampling.NEAREST).save(
        path.with_name(path.stem + "-4x.png"))
print("Saved simulated TFT calibration previews using the actual GFX font.")
