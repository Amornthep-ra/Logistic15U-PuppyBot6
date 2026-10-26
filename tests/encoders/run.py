"""Run the actual encoder API against a fake driver; requires host g++."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
output = ROOT / ".arduino-test/encoder-tests"
output.mkdir(parents=True, exist_ok=True)
executable = output / "encoders.exe"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
    "-I" + str(HERE / "fakes"),
    "-I" + str(ROOT / "Logistic15U-PuppyBot6/src"),
    str(HERE / "test_encoders.cpp"), "-o", str(executable),
], check=True)
for scenario in ("order12", "order21", "lazy", "repeat_init", "reset_each",
                 "reset_before_init", "invalid_id", "init_both"):
    subprocess.run([str(executable), scenario], check=True)
