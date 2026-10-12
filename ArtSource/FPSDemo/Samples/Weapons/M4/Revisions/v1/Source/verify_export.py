"""Reimport the exported FBX in a factory-startup process and check geometry."""
import bpy,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Exports'/'M4_Precision_Study.fbx'))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
data=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
assert len(meshes)==data['export_mesh_parts']
points=[];triangles=0
for o in meshes:
    assert len(o.data.vertices)>0
    o.data.calc_loop_triangles();triangles+=len(o.data.loop_triangles)
    points.extend(o.matrix_world@v.co for v in o.data.vertices)
assert triangles==data['triangles']
bounds=[[min(p[i] for p in points) for i in range(3)],[max(p[i] for p in points) for i in range(3)]]
error=max(abs(bounds[j][i]-data['bounds_metres'][j][i]) for j in range(2) for i in range(3))
assert error<.00001,error
data['checks']['fbx_reimport']={'mesh_parts':len(meshes),'triangles':triangles,'max_bounds_error_metres':error,'passed':True}
(ROOT/'AssetManifest.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
print('FBX_REIMPORT_PASS',len(meshes),triangles,'bounds error',error,flush=True)
