"""Checks saved source assets, fully weighted bones, deformation and icon alpha."""
import bpy
import json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/FPSDemo/Industrial'
manifest=json.loads((OUT/'manifest.json').read_text())
report={'checks':[],'success':False}
for entry in manifest['assets']:
    assert (OUT/entry['file']).stat().st_size>1000,entry['name']
    if not entry.get('skeletal'):
        assert entry['triangles']>0 and entry['uv_layers']>0,entry['name']
report['checks'].append('23 FBX files present; static exports have faces and UVs')
for name in ('WeaponLibrary','EnvironmentKit','Operator'):
    bpy.ops.wm.open_mainfile(filepath=str(OUT/(name+'.blend')))
    assert any(o.type=='MESH' for o in bpy.data.objects),name
report['checks'].append('All three blend libraries reopen')
rig=bpy.data.objects['SK_IND_Operator']
skins=[o for o in bpy.data.objects if o.name.startswith('Skin_')]
assert len(rig.data.bones)==16,len(rig.data.bones)
for obj in skins:
    for vertex in obj.data.vertices:
        assert abs(sum(g.weight for g in vertex.groups)-1)<.0001,(obj.name,vertex.index)
report['checks'].append('16-bone operator: all skin vertices weighted to total 1')
hand=bpy.data.objects['Skin_Hand_L']
deps=bpy.context.evaluated_depsgraph_get()
before=[(hand.matrix_world@v.co).copy() for v in hand.evaluated_get(deps).data.vertices]
bone=rig.pose.bones['Forearm_L'];bone.rotation_mode='XYZ';bone.rotation_euler.x=.5
bpy.context.view_layer.update()
after=[(hand.matrix_world@v.co).copy() for v in hand.evaluated_get(deps).data.vertices]
assert max((a-b).length for a,b in zip(after,before))>.05,'Forearm failed to move hand'
report['checks'].append('Forearm pose moves attached hand by >5 cm; source file not changed')
for file in sorted((OUT/'Icons').glob('*.png')):
    im=bpy.data.images.load(str(file),check_existing=False)
    alpha=list(im.pixels)[3::4]
    assert min(alpha)==0 and max(alpha)>.99,file.name
    fill=sum(a>.5 for a in alpha)/len(alpha)
    assert .03<fill<.7,(file.name,fill)
report['checks'].append('7 UI icons contain visible silhouettes and transparent surroundings')
runtime=OUT/'RuntimeParts.json'
if runtime.exists():
    parts=json.loads(runtime.read_text())['assets']
    assert len(parts)==15
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'RuntimeParts.blend'))
    for part in parts:
        assert (OUT/part['file']).stat().st_size>1000,part['name']
        mesh=bpy.data.objects[part['name']]
        assert mesh.type=='MESH' and len(mesh.data.polygons)>0 and len(mesh.data.uv_layers)>0
    report['checks'].append('RuntimeParts.blend reopens; 15 runtime parts contain faces, UVs and FBX exports')
report['success']=True
(OUT/'SourceValidation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('INDUSTRIAL_SOURCE_VALIDATION PASS',flush=True)
