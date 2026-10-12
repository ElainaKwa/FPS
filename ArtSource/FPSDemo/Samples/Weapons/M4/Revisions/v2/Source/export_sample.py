"""Evaluate exterior parts, export FBX and generate an auditable source manifest."""
import bpy,json,math,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
asset=bpy.data.collections['M4_PRECISION_SAMPLE']
shape_hash=hashlib.sha256((ROOT/'Source'/'refine_shapes.py').read_text(encoding='utf-8').encode('utf-8')).hexdigest()
assert bpy.context.scene.get('shape_source_sha256')==shape_hash,'Rebuild the blend after changing refine_shapes.py'
deps=bpy.context.evaluated_depsgraph_get()
originals=list(asset.all_objects)
out=bpy.data.collections.new('TEMP_EXPORT');bpy.context.scene.collection.children.link(out)
bpy.ops.object.select_all(action='DESELECT')
rows=[];points=[];exports=[]
for src in originals:
    if src.type not in {'MESH','CURVE','FONT'}:continue
    evaluated=src.evaluated_get(deps)
    data=bpy.data.meshes.new_from_object(evaluated,preserve_all_data_layers=True,depsgraph=deps)
    if not data.vertices:raise RuntimeError('Empty exterior part: '+src.name)
    data.calc_loop_triangles()
    obj=bpy.data.objects.new(src.name+'_Export',data);out.objects.link(obj);obj.matrix_world=src.matrix_world
    obj.select_set(True);exports.append(obj)
    coordinates=[obj.matrix_world@v.co for v in data.vertices]
    assert all(math.isfinite(x) for p in coordinates for x in p)
    points.extend(coordinates)
    rows.append(dict(name=src.name,triangles=len(data.loop_triangles),vertices=len(data.vertices),uv_layers=len(data.uv_layers)))
bpy.context.view_layer.objects.active=exports[0]
target=ROOT/'Exports'/'M4_Precision_Study.fbx'
bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,object_types={'MESH'},use_mesh_modifiers=False,add_leaf_bones=False,bake_anim=False,axis_forward='-Z',axis_up='Y',path_mode='AUTO')
bounds=[[min(p[i] for p in points) for i in range(3)],[max(p[i] for p in points) for i in range(3)]]
report=dict(version='M4-Exterior-Study-20261012-v2',blender=bpy.app.version_string,
    scope='Standalone exterior visual sample; no Unreal integration',shape_source_sha256=shape_hash,
    source_mesh_parts=sum(o.type=='MESH' for o in originals),export_mesh_parts=len(exports),
    triangles=sum(r['triangles'] for r in rows),bounds_metres=bounds,
    materials=[m.name for m in bpy.data.materials if m.name.startswith('M4_') and 'Studio' not in m.name],
    checks=dict(finite_vertices=True,nonempty_parts=True,source_mesh_uvs=all(r['uv_layers'] for r in rows if bpy.data.objects[r['name']].type=='MESH')),
    limits=['No retopology, LOD, rig or game integration','Blender procedural materials are authoritative; FBX carries basic material values, not baked PBR maps','First person preview is a Blender camera, not an Unreal screenshot'],
    sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/'M4_Precision_Study.blend',target]},parts=rows)
(ROOT/'AssetManifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('EXPORT_PASS',len(exports),'parts',report['triangles'],'triangles',flush=True)
# Deliberately do not save the temporary export scene over the editable source.
