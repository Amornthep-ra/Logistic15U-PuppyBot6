from pathlib import Path
import subprocess
from PIL import Image
root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/wait-sw1-tests'
out.mkdir(parents=True, exist_ok=True)
# Compile the production ADC implementations, not a rewritten mock of their routing.
source = (root/'Logistic15U-PuppyBot6/src/puppybot.h').read_text(encoding='utf-8')
def block(start):
    begin = source.index(start)
    brace = source.index('{', begin)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]
(out/'ActualAdc.h').write_text('\n'.join(block(s) for s in ['int ADC(int analog_CH)', 'namespace PuppyBotAdcDetail', 'int ADC_F(int analog_CH)', 'int ADC_B(int analog_CH)']), encoding='utf-8')
exe = out/'wait.exe'
gfx = Path.home()/'Documents/Arduino/libraries/Adafruit_GFX_Library'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
    '-I'+str(here/'fakes'), '-I'+str(root/'tests/calibration/fakes'), '-I'+str(root/'tests/rgb/fakes'), '-I'+str(gfx),
    '-I'+str(root/'Logistic15U-PuppyBot6/src'), '-I'+str(out), str(here/'test_wait.cpp'), '-o',str(exe)], check=True)
result = subprocess.run([str(exe),str(out)],capture_output=True,text=True)
print(result.stdout+result.stderr,end='')
(out/'results.txt').write_text(result.stdout+result.stderr,encoding='utf-8')
result.check_returncode()
for file in out.glob('*.ppm'):
    im = Image.open(file)
    im.resize((im.width*4,im.height*4),Image.Resampling.NEAREST).save(file.with_suffix('.png'))
