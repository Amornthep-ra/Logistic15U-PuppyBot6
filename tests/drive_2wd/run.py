from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
src = root/'Logistic15U-PuppyBot6/src'
out = root/'.arduino-test/drive-2wd-tests'
out.mkdir(parents=True, exist_ok=True)
source = (src/'puppybot.h').read_text(encoding='utf-8')

def block(start):
    a = source.index(start)
    b = source.index('{', a) + 1
    depth = 1
    while depth:
        depth += (source[b] == '{') - (source[b] == '}')
        b += 1
    return source[a:b]

# Execute actual production driver and movement bodies with recorded GPIO writes.
names = ['motor', 'motor_control', 'ao', 'aoS', 'motorStop', 'motorBreak',
         'fd', 'fd2', 'bk', 'bk2', 'tl', 'tr', 'sl', 'sr', 'Run_PID', 'Run_PID_B',
         'Run_PID_until_frontSensor', 'Run_PID_B_until_backSensor']
parts = re.findall(r'^#define motor\d[AB].*$', source, re.M)
parts += [block('void '+name+'(') for name in names]
parts += [block('void motorBreak(int')]
(out/'ActualDrive.h').write_text('\n'.join(parts), encoding='utf-8')
turn = (src/'PuppyBotTurnPID.h').read_text(encoding='utf-8')
(out/'ActualTurn.h').write_text(turn.replace('#include "PuppyBotIMU.h"',''), encoding='utf-8')
for p in src.glob('*.h'):
    text = p.read_text(encoding='utf-8')
    assert not re.search(r'4WD|4DW|Set_Mode_Gyro|setModeGyro|driveMode|mecanum', text), p.name
exe = out/'drive.exe'
subprocess.run(['g++','-std=c++17', '-I'+str(out), '-I'+str(src),
                str(Path(__file__).with_name('test_drive.cpp')), '-o',str(exe)], check=True)
result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
(out/'results.txt').write_text(result.stdout+result.stderr, encoding='utf-8')
print(result.stdout+result.stderr, end='')
result.check_returncode()
