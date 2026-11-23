"""One-time import of authored values. Unreal runs from the exported JSON only."""
from pathlib import Path
import json, re
root = Path(__file__).resolve().parent.parent
source = root.parent / 'Assets' / '_Project' / 'Resources'
def fields(path):
    result = {}
    for key, value in re.findall(r'^  (\w+): (.*)$', path.read_text(encoding='utf-8-sig'), re.M):
        if value.startswith('"'):
            result[key] = json.loads(value)
        elif value.startswith('{x:'):
            result[key] = [float(v) for v in re.findall(r'[xyz]: ([\d.e+-]+)',value)]
        else:
            try: result[key] = float(value)
            except ValueError: result[key] = value
    return result
anomalies = []
for path in sorted((source/'Anomalies').glob('*.asset')):
    a = fields(path)
    anomalies.append({k:a[k] for k in ('anomalyId','displayName','roomId','targetId','routeIndex','tier','selectionWeight','minProgress','effect','value','enabledState','replacementText')})
stories = []
for path in sorted((source/'Story').glob('*.asset')):
    a=fields(path)
    stories.append({k:a[k] for k in ('fragmentId','title','unlockAfterRounds','text')})
stories.sort(key=lambda a:a['unlockAfterRounds'])
data=root/'Content'/'Data'
data.mkdir(parents=True,exist_ok=True)
(data/'content.json').write_text(json.dumps({'anomalies':anomalies,'story':stories,'targetStreak':8,'anomalyChance':0.65,'walkSpeed':200,'reversalDistance':65},indent=2),encoding='utf-8')
print(f'Exported {len(anomalies)} anomalies and {len(stories)} story fragments.')
