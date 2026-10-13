"""Derive modular prototype preview meshes from the existing M4 source, never save that library.
Run in an isolated Blender background process. No weapon gameplay definition is changed.
"""
from pathlib import Path
import importlib.util,json,hashlib
import bpy
ROOT=Path(__file__).resolve().parents[6]
OUT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('industrial_geometry',ROOT/'Tools/build_industrial_art.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
g.OUT=OUT;g.ASSETS=[];g.reset()
source=ROOT/'ArtSource/FPSDemo/Industrial/WeaponLibrary.blend'
source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
with bpy.data.libraries.load(str(source),link=False) as (src,dst):
    assert 'SM_IND_M4' in src.collections,src.collections
    dst.collections=['SM_IND_M4']
original=dst.collections[0];bpy.context.scene.collection.children.link(original)
# WeaponLibrary stores gallery positions; M4 was raised by 3 * 0.45 m after its original export.
for obj in original.objects: obj.location.z -= 1.35
bpy.context.preferences.filepaths.save_version=0
def derive(name,predicate):
    col=g.group(name)
    for obj in original.objects:
        if predicate(obj.name):col.objects.link(obj)
    assert len(col.objects)>0,name
    g.export(col,'Preview')
    return col
derive('SM_EXP_M4_Core',lambda n:not n.startswith(('Magazine','Muzzle device','Bore','Brake port')))
derive('SM_EXP_M4_MagazineStandard',lambda n:n.startswith('Magazine'))
derive('SM_EXP_M4_MuzzleStandard',lambda n:n.startswith(('Muzzle device','Bore','Brake port')))
col=g.group('SM_EXP_M4_Optic')
g.cylinder('Optic body',(.095,0,.104),.024,.145,'Graphite','X')
for x in (.020,.170):
    g.cylinder('Objective ring',(x,0,.104),.027,.014,'Steel','X')
    g.cylinder('Optical lens',(x+(.008 if x>.1 else -.008),0,.104),.021,.002,'Visor','X')
for x in (.052,.137):
    g.box('Optic foot',(x,0,.066),(.035,.040,.019),'Graphite',.002)
    g.box('Ring riser',(x,0,.088),(.013,.043,.030),'Steel',.002)
g.cylinder('Elevation turret',(.100,0,.132),.012,.018,'Graphite','Z')
g.cylinder('Windage turret',(.100,.028,.106),.011,.016,'Steel','Y')
g.export(col,'Preview')
col=g.group('SM_EXP_M4_Brake')
g.cylinder('Brake body',(.58,0,0),.023,.09,'Steel','X')
g.cylinder('Bore',(.626,0,0),.009,.002,'Rubber','X')
for y in (-.022,.022):
    for x in (.555,.575,.595):g.box('Brake vent',(x,y,0),(.009,.004,.024),'Rubber',.001)
g.export(col,'Preview')
col=g.group('SM_EXP_M4_MagazineExtended')
g.magazine([(.10,-.035),(.165,-.035),(.174,-.147),(.215,-.299),(.15,-.313),(.111,-.156)],.037)
g.box('Extension base',(.18,0,-.30),(.075,.042,.019),'Rubber',.003)
g.export(col,'Preview')
# Only the derivative file is saved; original source stays byte-identical.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Source/M4_ExtractionPreview.blend'))
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
(OUT/'Manifest.json').write_text(json.dumps(dict(source=str(source.relative_to(ROOT)),source_sha256=source_hash,
    generator=str(Path(__file__).relative_to(ROOT)),scope='Experimental modular workbench preview only',assets=g.ASSETS),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('EXTRACTION_PREVIEW_MESHES PASS',len(g.ASSETS))
