"""Render actual authored weapon geometry to transparent silhouette source assets."""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/FPSDemo/Industrial'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'WeaponLibrary.blend'))
scene=bpy.context.scene
scene.render.resolution_x=512;scene.render.resolution_y=256;scene.render.resolution_percentage=100
scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG'
scene.render.image_settings.color_mode='RGBA'
scene.cycles.samples=16
white=bpy.data.materials.new('SilhouetteWhite');white.use_nodes=True
nodes=white.node_tree.nodes;nodes.clear()
em=nodes.new('ShaderNodeEmission');em.inputs[0].default_value=(1,1,1,1)
out=nodes.new('ShaderNodeOutputMaterial');white.node_tree.links.new(em.outputs[0],out.inputs[0])
scene.view_layers[0].material_override=white
scene.view_settings.view_transform='Standard'
cols=[c for c in bpy.data.collections if c.name.startswith('SM_IND_')]
for col in cols:
    for other in cols:other.hide_render=other!=col
    points=[]
    for obj in col.objects:
        if obj.type=='MESH':points.extend(obj.matrix_world @ Vector(v) for v in obj.bound_box)
    lo=Vector([min(v[i] for v in points) for i in range(3)])
    hi=Vector([max(v[i] for v in points) for i in range(3)])
    center=(lo+hi)/2
    cam=scene.camera;cam.location=center+Vector((0,-4,0))
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=max(hi.x-lo.x,(hi.z-lo.z)*2)*1.16
    scene.render.filepath=str(OUT/'Icons'/('T_IND_'+col.name.removeprefix('SM_IND_')+'.png'))
    bpy.ops.render.render(write_still=True)
print('INDUSTRIAL_ICONS_COMPLETE',len(cols),flush=True)
