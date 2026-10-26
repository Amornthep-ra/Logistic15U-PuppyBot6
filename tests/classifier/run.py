"""Check the actual classifier against captured synthetic calibration and EEPROM doubles."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
output = ROOT / ".arduino-test/classifier-tests"
output.mkdir(parents=True, exist_ok=True)
# Read the actual installed High implementation for differential testing; do not vendor it.
reference = Path.home() / "AppData/Local/Programs/kbide/boards/High-PuppyBot6/include/HighRGBCheck.h"
source = reference.read_text(encoding="utf-8-sig")
reference_code = source[:source.index("inline bool Read_RGB(")] + source[source.index("inline RGBColor Check_Color("):source.index("inline RGBReading &HighRGB_LastSample(")]
(output / "HighRGBClassifierReference.h").write_text(reference_code, encoding="utf-8")
executable = output / "classifier.exe"
subprocess.run([
    "g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
    "-I" + str(output), "-I" + str(ROOT / "tests/calibration/fakes"), "-I" + str(ROOT / "tests/rgb/fakes"),
    "-I" + str(ROOT / "Logistic15U-PuppyBot6/src"),
    str(HERE / "test_classifier.cpp"), "-o", str(executable),
], check=True)
result = subprocess.run([str(executable)], text=True, capture_output=True)
print(result.stdout + result.stderr, end="")
(output / "results.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
result.check_returncode()
