"""Prepare an allowlisted repository snapshot for user-controlled publication."""
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.2.0'
# 0.1.0 archives stay published because the package index still lists that platform.
files = [ROOT / name for name in (
    '.gitignore', '.gitattributes',
    'BOARD_PROFILE.json', 'COPY_MANIFEST.json', 'DEPENDENCIES.json',
    'package_logistic15u_index.json', 'dist/manifest.json',
    'dist/Logistic15U-PuppyBot6-0.1.0.zip', 'dist/logistic15u-rp2040-0.1.0.zip',
    f'dist/Logistic15U-PuppyBot6-{VERSION}.zip', f'dist/logistic15u-rp2040-{VERSION}.zip',
    'reference/board/context.json', 'reference/original-main/main/main.ino'
)]
files.extend(ROOT.glob('*.md'))  # README, PORTING, PUBLISHING and the *_VERIFICATION notes it links
for directory in ('Logistic15U-PuppyBot6', 'hardware', 'main', 'scripts'):
    files.extend(path for path in (ROOT / directory).rglob('*')
                 if path.is_file() and '__pycache__' not in path.parts)
output = ROOT / f'dist/Logistic15U-PuppyBot6-repository-{VERSION}.zip'
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
    for path in sorted(set(files)):
        name = path.relative_to(ROOT).as_posix()
        assert 'legacy-imu' not in name and 'Invn/' not in name
        archive.write(path, name)
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    print(f'{output.name}: {len(archive.namelist())} files, {output.stat().st_size} bytes')
