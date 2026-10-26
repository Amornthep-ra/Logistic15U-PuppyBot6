from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/servo-tests'
out.mkdir(parents=True, exist_ok=True)
exe = out / 'servos.exe'
subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
               '-I'+str(here/'fakes'), '-I'+str(root/'Logistic15U-PuppyBot6/src'),
               str(here/'test_servos.cpp'), '-o', str(exe)], check=True)
logs = []
for hz in [125000000, 133000000, 150000000, 250000000, 300000000, 133333333]:
    result = subprocess.run([str(exe), str(hz)], text=True, capture_output=True)
    logs.append(result.stdout + result.stderr)
    print(logs[-1], end='')
    (out/'results.txt').write_text(''.join(logs), encoding='utf-8')
    result.check_returncode()
