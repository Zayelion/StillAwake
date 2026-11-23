"""Verify a local player, or explicitly create a source/player ZIP with a checksum."""
import hashlib
import json
import sys
import zipfile
from pathlib import Path

def verify_player(source):
    launcher = source / 'StillAwake.exe'
    assert launcher.is_file() and launcher.stat().st_size > 0, 'Missing Windows launcher'
    binaries = source / 'StillAwake/Binaries/Win64'
    assert any(p.stat().st_size > 0 for p in binaries.glob('StillAwake*.exe')), 'Missing game runtime executable'
    assert any(p.stat().st_size > 0 for p in source.rglob('*.pak')), 'Missing cooked package'
    containers = list(source.rglob('*.utoc'))
    assert containers and all(p.stat().st_size > 0 and p.with_suffix('.ucas').is_file()
                              and p.with_suffix('.ucas').stat().st_size > 0 for p in containers), 'Missing cooked IO Store data'
    data = source / 'StillAwake/Content/Data/content.json'
    assert data.is_file() and data.stat().st_size > 0, 'Authored anomaly data was not staged'
    stamp = source / 'StillAwake/build-info.json'
    assert stamp.is_file(), 'Missing build-info.json'
    try:
        identity = json.loads(stamp.read_text(encoding='utf-8-sig'))
    except json.JSONDecodeError as error:
        raise ValueError('Malformed build-info.json') from error
    assert isinstance(identity, dict), 'Invalid build-info.json'
    fields = ('version', 'buildNumber', 'buildVersion', 'runId', 'configuration')
    assert all(isinstance(identity.get(field), str) and identity[field].strip() for field in fields), 'Build stamp requires nonempty strings'
    number = identity['buildNumber']
    assert number.isascii() and number.isdecimal() and not number.startswith('0'), 'Invalid build number'
    separator = '+'
    if '+' in identity['version']:
        separator = '.'
    assert identity['buildVersion'] == f"{identity['version']}{separator}build.{number}", 'Build version differs from version and build number'
    assert identity['configuration'] in {'Development', 'Shipping'}, 'Invalid build configuration'


mode, source, *outputs = sys.argv[1:]
source = Path(source).resolve()
if mode == 'verify-player':
    assert not outputs, 'verify-player does not create an archive'
    verify_player(source)
    print(f'PASS: local Windows player and required runtime files; {source / "StillAwake.exe"}')
    sys.exit(0)
assert len(outputs) == 1, 'source/player requires an output ZIP path'
output = Path(outputs[0]).resolve()
if mode == 'source':
    allowed = {'Source', 'Config', 'Content', 'Tools', 'Tests', 'Documentation', '.github'}
    paths = [p for p in source.rglob('*') if p.is_file() and (p.relative_to(source).parts[0] in allowed or (p.parent == source and (p.suffix in {'.uproject','.ps1','.md'} or p.name in {'.gitignore','.clang-format'}))) and '__pycache__' not in p.parts]
    assert source.joinpath('StillAwake.uproject') in paths
elif mode == 'player':
    verify_player(source)
    paths = [p for p in source.rglob('*') if p.is_file() and p.suffix.lower() not in {'.pdb','.log'}]
else:
    raise ValueError('Expected source or player')
output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in sorted(paths):
        archive.write(path, path.relative_to(source).as_posix())
    # The portable project keeps the same CI entry point when extracted on its own.
    workflow = source.parent / '.github/workflows/build.yml'
    if mode == 'source' and workflow.is_file() and not source.joinpath('.github/workflows/build.yml').is_file():
        archive.write(workflow, '.github/workflows/build.yml')
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None, 'ZIP integrity failure'
with output.open('rb') as stream:
    digest = hashlib.file_digest(stream, 'sha256').hexdigest()
output.with_suffix(output.suffix+'.sha256').write_text(f'{digest}  {output.name}\n')
print(f'PASS: {mode}, {len(paths)} files; {output.stat().st_size:,} bytes; SHA256 {digest}')
