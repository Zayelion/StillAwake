"""Static validation; no Unreal installation or third-party Python packages needed."""
import json
from math import isfinite
from pathlib import Path

root = Path(__file__).resolve().parents[1]
project = json.loads((root / 'StillAwake.uproject').read_text(encoding='utf-8-sig'))
assert project['EngineAssociation'] == '5.8'
content = json.loads((root / 'Content/Data/content.json').read_text(encoding='utf-8-sig'))
anomalies, stories = content['anomalies'], content['story']
anomalies_by_id = {anomaly['anomalyId']: anomaly for anomaly in anomalies}
assert len(anomalies) == 14 and len(stories) == 6
assert len(anomalies_by_id) == len(anomalies)
assert {a['routeIndex'] for a in anomalies} == set(range(1, 7))
assert all(a['effect'] in range(7) and a['tier'] in range(1,4) and a['selectionWeight'] > 0 for a in anomalies)
assert content['walkSpeed'] == 300
house = '\n'.join(path.read_text(encoding='utf-8') for path in (root / 'Source/StillAwake').glob('SAHouse*.cpp'))
assert all('"' + a['targetId'] + '"' in house for a in anomalies), 'Missing authored prop target'

targets = set(content['inspectableTargets'])
assert len(targets) == len(content['inspectableTargets']), 'Duplicate inspectable target'
assert all('"' + target + '"' in house for target in targets), 'Missing inspectable prop target'
hunts = content['hunts']
assert len(hunts) == 3
hunt_targets = set()
pressure = 0
for hunt in hunts:
    active = [anomalies_by_id[anomaly_id] for anomaly_id in hunt['anomalyIds']]
    active_targets = frozenset(anomaly['targetId'] for anomaly in active)
    assert active_targets <= targets and len(active_targets) == len(active), 'Invalid active target set'
    assert active_targets not in hunt_targets, 'Duplicate hunt configuration'
    hunt_targets.add(active_targets)
    assert any(a['routeIndex'] <= 3 for a in active) and any(a['routeIndex'] > 3 for a in active), 'Hunt needs clues on both floors'
    assert hunt['startLine'].strip() and hunt['sleepLine'].strip(), 'Missing cycle story'
    assert isfinite(hunt['sleepPressure']) and pressure < hunt['sleepPressure'], 'Sleep must escalate across nights'
    pressure = hunt['sleepPressure']

profiles = content['sleepProfiles']
assert [p['name'] for p in profiles] == ['gentle', 'manageable', 'pressured', 'severe', 'extreme']
sleep_fields = ('anxietyRiseRate', 'anxietyRecoveryRate', 'closetAdvanceRate', 'closetRetreatRate',
                'anxietyFailureThreshold', 'closetFailureThreshold', 'warningSeconds', 'sleepDuration')
for profile in profiles:
    assert all(isfinite(profile[field]) and profile[field] > 0 for field in sleep_fields), 'Invalid sleep tuning'
    assert profile['warningSeconds'] * profile['closetAdvanceRate'] * pressure < profile['closetFailureThreshold'], 'Closet warning must start after a chance to settle'
assert content['endingLine'].strip(), 'Missing ending text'

for path in ['Source/StillAwake.Target.cs','Source/StillAwakeEditor.Target.cs','Config/DefaultEngine.ini','Config/DefaultGame.ini','Config/DefaultInput.ini','Tools/Prepare-Content.py']:
    assert (root / path).is_file(), path
assert 'DirectoriesToAlwaysStageAsNonUFS=(Path="Data")' in (root/'Config/DefaultGame.ini').read_text()
assert 'DirectoriesToAlwaysCook=(Path="/Engine/BasicShapes")' in (root/'Config/DefaultGame.ini').read_text(), 'Runtime-loaded meshes must be cooked'
print('PASS: Unreal source, three authored hunts, five sleep difficulty profiles, story and staging configuration.')

hud = (root / 'Source/StillAwake/SAHUD.cpp').read_text(encoding='utf-8')
assert 'Rain against the windows' not in hud, 'Removed caption reintroduced'
for source in ['SAHUD.cpp', 'SAGameMode.cpp']:
    text = (root / 'Source/StillAwake' / source).read_text(encoding='utf-8')
    assert '\u00e2\u20ac' not in text, 'UTF-8 text was re-encoded through a Windows code page'
print('PASS: authored ending, caption regression check and UTF-8 game text.')
