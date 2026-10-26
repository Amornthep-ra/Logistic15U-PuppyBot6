"""Exercise actual PD/adapter and monitor rendering without running robot hardware."""
from pathlib import Path
import subprocess
from PIL import Image

root = Path(__file__).resolve().parents[2]
here = Path(__file__).resolve().parent
out = root / '.arduino-test/line-pd-tests'
out.mkdir(parents=True, exist_ok=True)
source = (root/'Logistic15U-PuppyBot6/src/puppybot.h').read_text(encoding='utf-8')
def body(signature):
    start = source.index(signature)
    end = source.index('{', start)+1
    depth = 1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}')
        end += 1
    return source[start:end]
(out/'ActualLegacyPD.h').write_text('\n'.join(body(s) for s in [
    'void Run_PID(int', 'void Run_PID_B(int',
    'void Run_PID_until_frontSensor(int', 'void Run_PID_B_until_backSensor(int']), encoding='utf-8')
gfx = Path.home()/'Documents/Arduino/libraries/Adafruit_GFX_Library'
logs = []
for name in ['pd','monitor']:
    exe = out/(name+'.exe')
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
        '-I'+str(root/'tests/wait_sw1/fakes'),'-I'+str(root/'tests/rgb/fakes'),
        '-I'+str(gfx),'-I'+str(root/'Logistic15U-PuppyBot6/src'),'-I'+str(out),
        str(here/('test_'+name+'.cpp')),'-o',str(exe)],check=True)
    result = subprocess.run([str(exe),str(out)],capture_output=True,text=True,timeout=15)
    logs.append(result.stdout+result.stderr)
    print(logs[-1],end='')
    (out/'results.txt').write_text(''.join(logs),encoding='utf-8')
    result.check_returncode()
for file in out.glob('*.ppm'):
    im = Image.open(file)
    im.resize((im.width*4,im.height*4),Image.Resampling.NEAREST).save(file.with_suffix('.png'))
