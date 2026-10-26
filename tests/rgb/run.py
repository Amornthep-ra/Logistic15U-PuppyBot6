"""Exercise the real RGB/monitor headers using host sensor/TFT doubles."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
output = ROOT / ".arduino-test/rgb-tests"
output.mkdir(parents=True, exist_ok=True)
gfx = Path.home() / "Documents/Arduino/libraries/Adafruit_GFX_Library"
if not (gfx / "glcdfont.c").exists():
    raise SystemExit("Install Adafruit GFX Library: its standard font is used without copying it.")
executable = output / "rgb.exe"
subprocess.run([
    "g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
    "-I" + str(HERE / "fakes"), "-I" + str(gfx),
    "-I" + str(ROOT / "Logistic15U-PuppyBot6/src"),
    str(HERE / "test_rgb.cpp"), "-o", str(executable),
], check=True)
result = subprocess.run([str(executable), str(output)], text=True, capture_output=True)
print(result.stdout, end="")
print(result.stderr, end="")
(output / "results.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
result.check_returncode()

from PIL import Image
for path in output.glob("*.ppm"):
    image = Image.open(path)
    image.save(path.with_suffix(".png"))
    image.resize((image.width * 4, image.height * 4), Image.Resampling.NEAREST).save(
        path.with_name(path.stem + "-4x.png"))
print("Saved simulated TFT previews using the standard Adafruit GFX font.")
