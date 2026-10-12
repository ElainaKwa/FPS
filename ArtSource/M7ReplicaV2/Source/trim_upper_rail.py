"""Keep the Tidal rail inside its handguard roof without changing tooth pitch."""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
previous_scene=bpy.context.window.scene
edit=bpy.data.scenes['M7_Library_Edit']
bpy.context.window.scene=edit
layers=list(edit.view_layers[0].layer_collection.children)
previous_exclusion=[(layer,layer.exclude) for layer in layers]
for layer in layers:layer.exclude=False
edit.view_layers[0].update()

def xmax(obj):
    return max((obj.matrix_world@Vector(v)).x for v in obj.bound_box)

base=bpy.data.objects['TIDAL_UpperRailBase']
if not base.get('roof_trim_version'):
    assert all(abs(s-1)<1e-6 for s in base.scale)
    assert max(abs(a) for a in base.rotation_euler)<1e-6
    for vertex in base.data.vertices:
        vertex.co.x*=.402/.434
    base.location.x=.359
    base.data.update()
    base['roof_trim_version']=1
for i in range(30,33):
    obj=bpy.data.objects.get('TIDAL_UpperRailTooth_%02d'%i)
    if obj:bpy.data.objects.remove(obj,do_unlink=True)
bpy.data.objects['M7_BarrelTidal_Mount_FrontIron'].location=(.552,0,.055)
bpy.context.view_layer.update()
manifest=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
row=next(r for r in manifest['assets'] if r['name']=='SM_M7_Barrel_Tidal')
row['local_mounts_m']['FrontIron']=[.394,0,.055]
review={
    'tidal_roof_flat_end_x_m':.567,
    'tidal_rail_base_end_x_m':xmax(base),
    'tidal_last_tooth_end_x_m':max(xmax(o) for o in bpy.data.objects if o.name.startswith('TIDAL_UpperRailTooth_')),
    'tidal_tooth_count':sum(o.name.startswith('TIDAL_UpperRailTooth_') for o in bpy.data.objects),
    'tidal_front_iron_world_m':[.552,0,.055],
    'standard_rail_base_end_x_m':xmax(bpy.data.objects['BARREL_Default_FrontTopRailBase']),
    'standard_last_tooth_end_x_m':max(xmax(o) for o in bpy.data.collections['SM_M7_Barrel_Default'].all_objects if 'RailTooth' in o.name and 'Lower' not in o.name),
    'standard_roof_flat_end_x_m':.537,
}
assert review['tidal_rail_base_end_x_m']<.567-.006
assert review['tidal_last_tooth_end_x_m']<.567-.009
assert review['tidal_tooth_count']==30
assert review['standard_last_tooth_end_x_m']<.537
review['success']=True
manifest['upper_rail_review']=review
(ROOT/'AssetManifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
for layer,value in previous_exclusion:layer.exclude=value
bpy.context.window.scene=previous_scene
print(json.dumps(review))
