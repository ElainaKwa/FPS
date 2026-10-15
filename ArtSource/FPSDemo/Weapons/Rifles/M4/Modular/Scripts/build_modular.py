"""Derive replaceable M4 parts without touching the Industrial library or M7.
Run with Blender --background --factory-startup --python-exit-code 1 --python this_file.
"""
from pathlib import Path
import importlib.util, hashlib, json
import bpy

OUT = Path(__file__).resolve().parents[1]
ROOT = next(p for p in OUT.parents if (p / 'MCP_Test.uproject').exists())
for folder in ('Exports', 'Source', 'Previews'):
    (OUT / folder).mkdir(parents=True, exist_ok=True)
source = ROOT / 'Tools/build_industrial_art.py'
spec = importlib.util.spec_from_file_location('industrial', source)
g = importlib.util.module_from_spec(spec); spec.loader.exec_module(g)
export = g.export; g.export = lambda *args: None
g.OUT = OUT; g.reset(); original = g.weapon('M4'); g.export = export
bpy.context.preferences.filepaths.save_version = 0

def kind(name):
    if name.startswith(('Pistol grip', 'Grip traction')): return 'RearGrip'
    if name.startswith(('Adjustable stock', 'Stock inset', 'Butt pad')): return 'Stock'
    if name.startswith('Barrel'): return 'Barrel'
    return None

entries = []
def save(col, category=None):
    row = export(col, 'Modular')
    row['destination'] = ('/Game/FPSDemo/Art/Weapons/Attachments/' + category + '/M4/Meshes') if category else '/Game/FPSDemo/Art/Weapons/Rifles/M4/Modular/Meshes'
    row['sha256'] = hashlib.sha256((OUT / row['file']).read_bytes()).hexdigest()
    entries.append(row)

def derive(suffix, predicate, category=None):
    col = g.group('SM_M4_Modular' + suffix)
    for obj in original.objects:
        if predicate(obj.name): col.objects.link(obj)
    assert col.objects, suffix
    save(col, category)

derive('Body', lambda n: kind(n) is None)
derive('ReloadBody', lambda n: kind(n) is None and not n.startswith(('Magazine', 'Charging handle')))
derive('WorkbenchBody', lambda n: kind(n) is None and not n.startswith(('Magazine', 'Muzzle device', 'Bore', 'Brake port')))
for part, folder in [('RearGrip','RearGrips'),('Stock','Stocks'),('Barrel','Barrels')]:
    derive(part+'Standard', lambda n, p=part: kind(n)==p, folder)

for variant, wide, tilt, mat in [('Ergo',.034,-.005,'Ceramic'),('Control',.046,-.025,'Polymer')]:
    col=g.group('SM_M4_ModularRearGrip'+variant)
    g.profile('Grip body',[(-.035,-.028),(.020,-.038),(tilt,-.145),(tilt-.052,-.142)],wide,mat,.004)
    for i in range(5): g.box('Grip traction',(-.025-i*.002,0,-.073-i*.013),(.047,wide+.003,.004),'Rubber',.001)
    g.box('Grip heel',(tilt-.022,0,-.144),(.056,wide+.004,.012),'Rubber',.002)
    save(col,'RearGrips')

col=g.group('SM_M4_ModularStockLight')
for z in (.015,-.045): g.box('Skeleton arm',(-.266,0,z),(.183,.031,.012),'Steel',.003)
g.profile('Rear brace',[(-.35,.035),(-.325,.035),(-.325,-.084),(-.35,-.084)],.042,'Polymer',.003)
g.box('Butt pad',(-.36,0,-.027),(.019,.048,.133),'Rubber',.005)
save(col,'Stocks')
col=g.group('SM_M4_ModularStockStable')
g.profile('Stock body',[(-.35,.040),(-.22,.040),(-.165,0),(-.185,-.045),(-.29,-.09),(-.35,-.09)],.059,'Ceramic',.005)
g.box('Cheek support',(-.27,0,.042),(.13,.065,.018),'Polymer',.004)
g.box('Butt pad',(-.36,0,-.027),(.019,.065,.143),'Rubber',.005)
save(col,'Stocks')
for variant, radius, mat in [('Light',.007,'Graphite'),('Heavy',.014,'Steel')]:
    col=g.group('SM_M4_ModularBarrel'+variant)
    g.cylinder('Barrel',(.4825,0,0),radius,.165,mat,'X')
    if variant=='Heavy':
        for x in (.439,.451,.463,.475,.487,.499):g.cylinder('Cooling collar',(x,0,0),.017,.006,'Graphite','X')
    else:
        for x in (.425,.48,.54):g.cylinder('Light barrel band',(x,0,0),.009,.009,'Steel','X')
    save(col,'Barrels')

bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Source/M4_Modular.blend'))
(OUT/'Manifest.json').write_text(json.dumps(dict(version=1, scope='M4 experimental modular assembly; original art preserved',
    generator=str(Path(__file__).relative_to(ROOT)).replace('\\','/'), source=str(source.relative_to(ROOT)).replace('\\','/'),
    source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), assets=entries),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('FPS_MODULAR_EXPORT PASS meshes='+str(len(entries)),flush=True)
