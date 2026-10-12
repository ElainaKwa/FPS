"""Bake derived meshes, unwrap source, export only the new rifle asset."""
import bpy
import json
from pathlib import Path
from mathutils import Vector, Matrix

ROOT=Path(r'D:/Unreal Projects/MCP_Test/ArtSource/ToonRifle')
OUT=ROOT/'Exports'
OUT.mkdir(exist_ok=True)
source_scene=bpy.context.scene
asset=bpy.data.collections['ASSET_TIDELINE_AR01']
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
    if name.startswith('Magazine_'):return 'Magazine'
    if name.startswith('Stock_'):return 'Stock'
    if name.startswith('Grip_'):return 'PistolGrip'
    if name.startswith('Foregrip_'):return 'Foregrip'
    if name.startswith('Muzzle_'):return 'Muzzle'
    if name.startswith('Trigger_'):return 'Trigger'
    if name.startswith('ACC_Optic_'):return 'Optic'
    if name.startswith('ACC_SideModule_'):return 'SideModule'
    if name.startswith('ACC_RearIron_'):return 'RearSight'
    if name.startswith('ACC_FrontIron_'):return 'FrontSight'
    return 'ReceiverAndHandguard'

export_scene=bpy.data.scenes.new('AR01_TEMP_Export')
temp=bpy.data.collections.new('AR01_TEMP_BakedMeshes')
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
pivots={'Magazine':(.12,0,-.08),'Stock':(-.154,0,.025),'PistolGrip':(-.07,0,-.06),
        'Foregrip':(.312,0,-.04),'Muzzle':(.56,0,.02),'Trigger':(.012,0,-.055),
        'Optic':(0,0,.108),'SideModule':(.37,-.05,.05),'RearSight':(-.118,0,.108),
        'FrontSight':(.44,0,.108),'ReceiverAndHandguard':(0,0,0)}
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
    o.name='AR01_'+name
    # Text geometry has no UV at creation: unwrap once on the baked part.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.018,scale_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    parts.append(o)
aroot=bpy.data.objects.new('TIDELINE_AR01',None)
temp.objects.link(aroot)
for o in parts:o.parent=aroot
bpy.ops.object.select_all(action='SELECT')
gltf_op=bpy.ops.export_scene.gltf.get_rna_type()
allowed_formats=[i.identifier for i in gltf_op.properties['export_format'].enum_items]
# Some Blender dynamic enum providers report an empty list in RNA inspection.
# The operator validates the actual value and reports accepted formats itself.
bpy.ops.export_scene.gltf(filepath=str(OUT/'SM_TW_TIDELINE_AR01.glb'),
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
combined.name='SM_TW_TIDELINE_AR01'
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
bpy.ops.export_scene.fbx(filepath=str(OUT/'SM_TW_TIDELINE_AR01.fbx'),
    use_selection=True,object_types={'MESH'},apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',axis_forward='X',axis_up='Z',
    use_space_transform=True,mesh_smooth_type='FACE',use_mesh_modifiers=True,
    add_leaf_bones=False,bake_anim=False,path_mode='AUTO')
combined.data.calc_loop_triangles()
bounds=[combined.matrix_world@Vector(c) for c in combined.bound_box]
stats={'asset':'TIDELINE_AR01','source_file':'TIDELINE_AR01.blend',
       'editable_source_objects':len(asset.all_objects),'source_meshes':len(source_meshes),
       'gltf_detachable_parts':len(groups),'triangles':len(combined.data.loop_triangles),
       'vertices':len(combined.data.vertices),'materials':[m.name for m in materials],
       'dimensions_m':[max(v[k] for v in bounds)-min(v[k] for v in bounds) for k in range(3)],
       'axis':'+X muzzle / +Z up; metres','uv_layers':len(combined.data.uv_layers),
       'style':'original cartoon tactical assault rifle inspired by game attachment detailing',
       'exports':['Exports/SM_TW_TIDELINE_AR01.fbx','Exports/SM_TW_TIDELINE_AR01.glb'],
       'rigged':False,'textures':'9 named Principled materials, geometry details, UV0; no external dependencies',
       'parts':list(groups),'unreal_import_status':'not imported or tested in Unreal Editor',
       'source_hierarchy':'11 named PART_ empties under AR01_Root_GripOrigin',
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
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'TIDELINE_AR01.blend'))
print(json.dumps(stats,ensure_ascii=False))
