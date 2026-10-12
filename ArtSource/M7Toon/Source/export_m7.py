"""Bake derived meshes, unwrap source, export only the new rifle asset."""
import bpy
import json
from pathlib import Path
from mathutils import Vector, Matrix

ROOT=Path(r'D:/Unreal Projects/MCP_Test/ArtSource/M7Toon')
OUT=ROOT/'Exports'
OUT.mkdir(exist_ok=True)
source_scene=bpy.context.scene
asset=bpy.data.collections['ASSET_M7_BLACK']
source_meshes=[o for o in asset.all_objects if o.type=='MESH']

# Every source piece gets UV coordinates before modifiers are baked.
bpy.ops.object.select_all(action='DESELECT')
for o in source_meshes:o.select_set(True)
bpy.context.view_layer.objects.active=source_meshes[0]
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.151917, island_margin=.018, scale_to_bounds=True)
bpy.ops.object.mode_set(mode='OBJECT')
deps=bpy.context.evaluated_depsgraph_get()
bpy.ops.object.select_all(action='DESELECT')

def part_name(name):
    mapping=[('Stock_','Stock'),('Grip_','Grip'),('Magazine_','Magazine'),('Optic_','Optic'),
        ('SightRear_','RearSight'),('SightFront_','FrontSight'),('Foregrip_','Foregrip'),
        ('Muzzle_','Muzzle'),('Trigger_','Trigger')]
    return next((part for prefix,part in mapping if name.startswith(prefix)),'ReceiverHandguardBarrel')

export_scene=bpy.data.scenes.new('M7_TEMP_Export')
temp=bpy.data.collections.new('M7_TEMP_BakedMeshes')
export_scene.collection.children.link(temp)
groups={}
for o in asset.all_objects:
    if o.type not in ('MESH','FONT'):continue
    ev=o.evaluated_get(deps)
    data=bpy.data.meshes.new_from_object(ev,preserve_all_data_layers=True,depsgraph=deps)
    copy=bpy.data.objects.new(o.name+'_Baked',data)
    temp.objects.link(copy)
    copy.matrix_world=o.matrix_world.copy()
    groups.setdefault(part_name(o.name),[]).append(copy)
bpy.context.window.scene=export_scene
pivots={'Stock':(-.18,0,0),'Grip':(-.08,0,-.06),'Magazine':(.07,0,-.075),
    'Optic':(0,0,.07),'RearSight':(-.15,0,.067),'FrontSight':(.48,0,.067),
    'Foregrip':(.32,0,-.04),'Muzzle':(.73,0,.012),'Trigger':(-.01,0,-.055),
    'ReceiverHandguardBarrel':(0,0,0)}
parts=[]
for name,objects in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.join()
    o=bpy.context.object
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    # Preserve world geometry while giving detachable parts sensible pivots.
    p=Vector(pivots[name])
    offset=o.location-p
    o.data.transform(Matrix.Translation(offset))
    o.location=p
    o.name='M7_'+name
    # Text geometry has no UV at creation: unwrap once on the baked part.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.018,scale_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    parts.append(o)
aroot=bpy.data.objects.new('M7_BLACK',None)
temp.objects.link(aroot)
for o in parts:o.parent=aroot
bpy.ops.object.select_all(action='SELECT')
gltf_op=bpy.ops.export_scene.gltf.get_rna_type()
allowed_formats=[i.identifier for i in gltf_op.properties['export_format'].enum_items]
# Some Blender dynamic enum providers report an empty list in RNA inspection.
# The operator validates the actual value and reports accepted formats itself.
bpy.ops.export_scene.gltf(filepath=str(OUT/'SM_TW_M7_BLACK.glb'),
    export_format='GLB',use_selection=True,use_active_scene=True,export_apply=False,export_yup=True,
    export_cameras=False,export_lights=False,export_animations=False)

# Single combined static mesh for FBX import, original parts stay in .blend/GLB.
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
bpy.ops.object.join()
combined=bpy.context.object
combined.parent=None
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
combined.name='SM_TW_M7_BLACK'
# Remove repeated material slots introduced by joining, preserving assignments.
materials=[]
index_map={}
for i,m in enumerate(combined.data.materials):
    if m not in materials:materials.append(m)
    index_map[i]=materials.index(m)
new_indices=[index_map[p.material_index] for p in combined.data.polygons]
combined.data.materials.clear()
for m in materials:combined.data.materials.append(m)
for p,i in zip(combined.data.polygons,new_indices):p.material_index=i
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_TW_M7_BLACK.fbx'),
    use_selection=True,object_types={'MESH'},apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',axis_forward='X',axis_up='Z',
    use_space_transform=True,mesh_smooth_type='FACE',use_mesh_modifiers=True,
    add_leaf_bones=False,bake_anim=False,path_mode='AUTO')
combined.data.calc_loop_triangles()
bounds=[combined.matrix_world@Vector(c) for c in combined.bound_box]
stats={'asset':'M7_BLACK','source_file':'M7_BLACK_Stylized.blend',
       'editable_source_objects':len(asset.all_objects),'source_meshes':len(source_meshes),
       'gltf_detachable_parts':len(groups),'triangles':len(combined.data.loop_triangles),
       'vertices':len(combined.data.vertices),'materials':[m.name for m in materials],
       'dimensions_m':[max(v[k] for v in bounds)-min(v[k] for v in bounds) for k in range(3)],
       'axis':'+X muzzle / +Z up; metres','uv_layers':len(combined.data.uv_layers),
       'style':'Realistic M7-inspired silhouette, black/charcoal realistic palette, restrained stylized edges',
       'exports':['Exports/SM_TW_M7_BLACK.fbx','Exports/SM_TW_M7_BLACK.glb'],
       'rigged':False,'textures':'Named black/gray Principled materials, UVs, geometric surface details; no external textures',
       'parts':list(groups),'unreal_import_status':'not imported or tested in Unreal Editor',
       'source_hierarchy':'10 named M7_PART_ empties under M7_Root',
       'fbx_axis_note':'FBX metadata +X forward +Z up; use_space_transform=True. Default Blender reimport restores source XYZ geometry.'}
(ROOT/'AssetManifest.json').write_text(json.dumps(stats,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.context.window.scene=source_scene
# Only delete the isolated temporary export scene and derived objects.
for o in list(temp.objects):
    data=o.data
    bpy.data.objects.remove(o,do_unlink=True)
    if data and data.users==0 and isinstance(data,bpy.types.Mesh):bpy.data.meshes.remove(data)
bpy.data.scenes.remove(export_scene)
bpy.data.collections.remove(temp)
with bpy.context.temp_override(id=asset):
    if (ROOT/'Preview_Hero.png').exists():
        bpy.ops.ed.lib_id_load_custom_preview(filepath=str(ROOT/'Preview_Hero.png'))
bpy.ops.object.select_all(action='DESELECT')
bpy.data.libraries.write(str(ROOT/'M7_BLACK_Stylized.blend'),{source_scene},fake_user=True,compress=True)
print(json.dumps(stats,ensure_ascii=False))

