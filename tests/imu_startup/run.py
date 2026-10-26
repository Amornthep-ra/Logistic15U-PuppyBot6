from pathlib import Path
import subprocess
from PIL import Image

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/imu-startup-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root / 'Logistic15U-PuppyBot6/src/PuppyBotIMU.h').read_text(encoding='utf-8')

def extract(signature):
    begin = source.index(signature)
    end = source.index('{', begin) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

(out / 'ActualIMU.h').write_text('namespace PuppyBotIMU {\n' + '\n'.join(
    extract(s) for s in ['bool initIMU(', 'bool updateIMU_fine_offset_Yaw(']) + '\n}', encoding='utf-8')
gfx = Path.home() / 'Documents/Arduino/libraries/Adafruit_GFX_Library'
exe = out / 'imu-startup.exe'
subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
    '-I' + str(root / 'tests/rgb/fakes'), '-I' + str(gfx),
    '-I' + str(root / 'Logistic15U-PuppyBot6/src'), '-I' + str(out),
    str(here / 'test_startup.cpp'), '-o', str(exe)], check=True)
result = subprocess.run([str(exe), str(out)], capture_output=True, text=True, timeout=10)
print(result.stdout + result.stderr, end='')
(out / 'results.txt').write_text(result.stdout + result.stderr, encoding='utf-8')
result.check_returncode()
for file in out.glob('*.ppm'):
    im = Image.open(file)
    im.resize((im.width * 4, im.height * 4), Image.Resampling.NEAREST).save(file.with_suffix('.png'))
