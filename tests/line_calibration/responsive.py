from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2]
here=Path(__file__).resolve().parent
out=root/'.arduino-test/line-calibration-tests'
gfx=Path.home()/'Documents/Arduino/libraries/Adafruit_GFX_Library'
exe=out/'responsive.exe'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
 '-I'+str(root/'tests/calibration/fakes'),'-I'+str(root/'tests/rgb/fakes'),
 '-I'+str(gfx),'-I'+str(root/'Logistic15U-PuppyBot6/src'),'-I'+str(out),
 str(here/'test_responsive.cpp'),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
print(r.stdout+r.stderr,end='')
(out/'responsive-results.txt').write_text(r.stdout+r.stderr,encoding='utf-8')
r.check_returncode()
