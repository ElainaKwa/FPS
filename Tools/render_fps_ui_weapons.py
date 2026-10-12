"""Render existing saved weapon geometry as shaded UI thumbnails, without modifying the blend."""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/FPSDemo/Industrial/UI'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(OUT.parent/'WeaponLibrary.blend'))
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=24
scene.cycles.use_denoising=True
scene.render.resolution_x=1024
scene.render.resolution_y=384
scene.render.resolution_percentage=100
scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG'
scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='AgX'
scene.view_settings.exposure=1.0
for obj in list(bpy.data.objects):
    if obj.type=='LIGHT': bpy.data.objects.remove(obj,do_unlink=True)
lights=[]
for name,offset,power,size in [('Key',(0,-2,3),550,4),('Rim',(1,2,1),750,3),('Fill',(-2,-1,.3),250,3)]:
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);lights.append((obj,Vector(offset)))
cols=[c for c in bpy.data.collections if c.name.startswith('SM_IND_')]
for col in cols:
    for other in cols:other.hide_render=other!=col
    points=[obj.matrix_world@Vector(v) for obj in col.objects if obj.type=='MESH' for v in obj.bound_box]
    lo=Vector([min(v[i] for v in points) for i in range(3)]);hi=Vector([max(v[i] for v in points) for i in range(3)])
    center=(lo+hi)/2
    for light,offset in lights:
        light.location=center+offset;light.rotation_euler=(-offset).to_track_quat('-Z','Y').to_euler()
    cam=scene.camera;cam.data.type='ORTHO';cam.location=center+Vector((.22,-4,.18))
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=max(hi.x-lo.x,(hi.z-lo.z)*1024/384)*1.12
    scene.render.filepath=str(OUT/('T_UI_'+col.name.removeprefix('SM_IND_')+'.png'))
    bpy.ops.render.render(write_still=True)
print('FPS_UI_WEAPONS PASS',len(cols),flush=True)
