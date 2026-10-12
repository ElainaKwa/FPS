"""Derive runtime rigid-joint meshes from Operator.blend without replacing authoring sources.
Run in Blender; leaves the derived library open and saves RuntimeParts.blend.
"""
from pathlib import Path
import bpy
import math
import json
from mathutils import Matrix, Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/FPSDemo/Industrial'
# Append evaluated skin meshes from the existing library to an isolated new scene.
scene=bpy.data.scenes.new('IndustrialRuntimeParts')
bpy.context.window.scene=scene
if Path(bpy.data.filepath).resolve()==(OUT/'Operator.blend').resolve():
    loaded=[o for o in bpy.data.objects if o.name.startswith('Skin_')]
else:
    with bpy.data.libraries.load(str(OUT/'Operator.blend'),link=False) as (src,dst):
        dst.objects=[n for n in src.objects if n.startswith('Skin_')]
    loaded=dst.objects
sources={o.name.split('.')[0].removeprefix('Skin_'):o for o in loaded if o}
rotation=Matrix.Rotation(math.pi/2,4,'Z')
report=[]


def export(name,source,pivot,segment_length=None):
    src=sources[source]
    mesh=src.data.copy()
    obj=bpy.data.objects.new('SM_IND_RT_'+name,mesh)
    scene.collection.objects.link(obj)
    # Source skin vertices have world positions and rigid bind-pose coordinates.
    for vertex in mesh.vertices:
        v=src.matrix_world@vertex.co-Vector(pivot)
        if segment_length:
            # Existing cosmetic rig stretches a 1 cm +X segment; cross section stays in meters.
            vertex.co=( -v.z/segment_length*.01, v.x, -v.y )
        else:
            vertex.co=rotation@v
    for item in bpy.context.selected_objects:item.select_set(False)
    obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/(obj.name+'.fbx')),use_selection=True,
        object_types={'MESH'},axis_forward='X',axis_up='Z',use_space_transform=False,
        add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE')
    points=[v.co for v in mesh.vertices]
    report.append({'name':obj.name,'source':source,'file':'Exports/'+obj.name+'.fbx',
        'bounds_m':{'min':[min(v[i] for v in points) for i in range(3)],'max':[max(v[i] for v in points) for i in range(3)]}})
    obj.select_set(False)


export('Torso','Torso',(0,0,1.04))
export('Helmet','Head',(0,0,1.61))
export('Pelvis','Pelvis',(0,0,.91))
for runtime,source,sign in [('L','R',1),('R','L',-1)]:
    for kind,z,length in [('UpperArm',1.38,.32),('Forearm',1.06,.27),('Thigh',.89,.42),('Shin',.47,.365)]:
        x=sign*(.266 if kind in ('UpperArm','Forearm') else .111)
        export(runtime+kind,kind+'_'+source,(x,0,z),length)
    export(runtime+'Glove','Hand_'+source,(sign*.266,0,.74))
    export(runtime+'Boot','Foot_'+source,(sign*.111,0,.05))
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
# Keep authoring scene and every original file intact; save the combined working session as a derivative.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'RuntimeParts.blend'))
(OUT/'RuntimeParts.json').write_text(json.dumps({'generator':'Tools/export_industrial_runtime_parts.py','assets':report},indent=2),encoding='utf-8')
print('RUNTIME_PARTS_EXPORTED',len(report))
