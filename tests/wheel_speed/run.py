"""Run real wheel PI and board/line adapter against deterministic encoder/plant fixtures."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/wheel-speed-tests'
out.mkdir(parents=True, exist_ok=True)
logs = []
actions = root.parents[2] / 'Logistic15U-PuppyBot6/RobotActions.h'
text = actions.read_text(encoding='utf-8')
start = text.index('inline bool lineFrontCross')
(out / 'ActualUserFront.h').write_text(text[start:], encoding='utf-8')
for name in ['controller', 'adapter', 'user_front']:
    exe = out / (name + '.exe')
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I' + str(root / 'tests/wait_sw1/fakes'),
        '-I' + str(root / 'tests/rgb/fakes'),
        '-I' + str(root / 'Logistic15U-PuppyBot6/src'),
        '-I' + str(out),
        str(here / ('test_' + name + '.cpp')), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], text=True, capture_output=True, timeout=10)
    logs.append(result.stdout + result.stderr)
    print(logs[-1], end='')
    (out / 'results.txt').write_text(''.join(logs), encoding='utf-8')
    result.check_returncode()
