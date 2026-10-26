"""Exercise production calibration, flash records, line readers and TFT/menu flow."""
from pathlib import Path
import subprocess
from PIL import Image

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/line-calibration-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'Logistic15U-PuppyBot6/src/puppybot.h').read_text(encoding='utf-8')
def body(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
(out / 'ActualLineReaders.h').write_text('\n'.join(body(s) for s in [
    'int readline()   ', 'int readline_B()   ', 'int Read_sumValue_sensor()',
    'int Read_sumValue_sensor_B()', 'bool Read_status_sensor(', 'bool Read_status_sensor_B(',
    'void setCalibrate(int round)', 'void setCalibrate_B(int round)']), encoding='utf-8')
gfx = Path.home() / 'Documents/Arduino/libraries/Adafruit_GFX_Library'
exe = out / 'line.exe'
subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
    '-I'+str(root/'tests/calibration/fakes'), '-I'+str(root/'tests/rgb/fakes'),
    '-I'+str(gfx), '-I'+str(root/'Logistic15U-PuppyBot6/src'), '-I'+str(out),
    str(here/'test_line.cpp'), '-o',str(exe)], check=True)
result = subprocess.run([str(exe), str(out)], capture_output=True, text=True)
print(result.stdout + result.stderr, end='')
(out/'results.txt').write_text(result.stdout+result.stderr, encoding='utf-8')
result.check_returncode()
for file in out.glob('*.ppm'):
    im = Image.open(file)
    im.resize((im.width*4,im.height*4),Image.Resampling.NEAREST).save(file.with_suffix('.png'))
