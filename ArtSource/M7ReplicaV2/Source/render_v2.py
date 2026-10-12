"""Render the revised grip and the two actually replaceable barrel variants."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
requested=set(sys.argv[sys.argv.index('--')+1:]) if '--' in sys.argv else set()
prefs=bpy.context.preferences.addons['cycles'].preferences
try:prefs.compute_device_type='OPTIX'
except TypeError:
    try:prefs.compute_device_type='CUDA'
    except TypeError:pass
prefs.get_devices();gpu=False
for device in prefs.devices:
    device.use=device.type==prefs.compute_device_type and device.type in ('OPTIX','CUDA')
    gpu=gpu or device.use
def aim(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
def render(scene_name,name,loc,target,scale,res):
    if requested and name not in requested:return
    scene=bpy.data.scenes[scene_name];bpy.context.window.scene=scene
    scene.render.engine='CYCLES';scene.cycles.device='GPU' if gpu else 'CPU'
    scene.cycles.samples=80;scene.cycles.use_denoising=True
    scene.camera.location=loc;aim(scene.camera,target);scene.camera.data.ortho_scale=scale
    scene.render.resolution_x,scene.render.resolution_y=res;scene.render.resolution_percentage=100
    scene.render.image_settings.color_mode='RGBA';scene.render.film_transparent=False
    scene.render.filepath=str(ROOT/name)
    print('RENDER_START',name,flush=True);bpy.ops.render.render(write_still=True);print('RENDER_DONE',name,flush=True)
render('M7_Equipped_Tidal','Preview_Equipped_Tidal.png',(.99,2.10,.59),(.25,0,-.051),1.52,(2600,1300))
render('M7_Equipped_Standard','Preview_Equipped_Standard.png',(.90,2,.56),(.17,0,-.048),1.32,(2400,1200))
render('M7_Equipped_Tidal','Preview_ResonanceGen1_Detail.png',(.42,.70,.15),(.347,0,-.092),.335,(1500,1300))
render('M7_Equipped_Tidal','Preview_Receiver_Magazine_Detail.png',(.22,.90,.25),(.09,0,-.087),.53,(1800,1350))
render('M7_Equipped_Tidal','Preview_UpperRail_Detail.png',(.63,.85,.20),(.546,0,.022),.245,(1800,1200))
render('M7_Default_Assembly','Preview_Default.png',(.89,1.95,.53),(.125,0,-.042),1.13,(2200,1100))
render('M7_Attachment_Catalog','Preview_Attachments.png',(0,2.6,.10),(0,0,.05),2.12,(2400,1850))
# Temporary inspection scene: the barrel interfaces are aligned to compare
# actual lengths and geometry. It is never included in an asset export.
scene=bpy.data.scenes.new('M7_RENDER_Barrel_Comparison')
base=bpy.data.scenes['M7_Default_Assembly'];scene.world=base.world
for key in ('view_transform','look','exposure','gamma'):setattr(scene.view_settings,key,getattr(base.view_settings,key))
for obj in base.objects:
    if obj.type not in ('LIGHT','CAMERA'):continue
    copy=obj.copy();copy.data=obj.data.copy();scene.collection.objects.link(copy)
    if obj==base.camera:scene.camera=copy
for name,z,title in [('SM_M7_Barrel_Default',.12,'STANDARD'),('SM_M7_Barrel_Tidal',-.13,'M7 TIDAL')]:
    instance=bpy.data.objects.new(name+'_Comparison',None);scene.collection.objects.link(instance)
    instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections[name]
    instance.location=(.158,0,z)
    font=bpy.data.curves.new(title,'FONT');font.body=title;font.size=.016
    font.materials.append(bpy.data.materials['M7R_07_EngravingGray'])
    label=bpy.data.objects.new(title,font);scene.collection.objects.link(label)
    label.location=(.20,.05,z+.075);label.rotation_euler=(math.pi/2,0,math.pi)
render(scene.name,'Preview_Barrel_Comparison.png',(.475,2.4,.04),(.475,0,.015),.86,(2300,1050))
