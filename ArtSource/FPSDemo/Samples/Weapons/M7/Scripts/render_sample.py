"""Render the revised grip and the two actually replaceable barrel variants."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
requested=set(sys.argv[sys.argv.index('--')+1:]) if '--' in sys.argv else set()
baseline='--baseline' in requested
requested.discard('--baseline')
import importlib.util
spec=importlib.util.spec_from_file_location('m7_style',ROOT/'Scripts/align_style.py')
style=importlib.util.module_from_spec(spec);spec.loader.exec_module(style)
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
    style.lighting(scene)
    scene.render.engine='CYCLES';scene.cycles.device='GPU' if gpu else 'CPU'
    scene.cycles.samples=80;scene.cycles.use_denoising=True
    scene.camera.location=loc;aim(scene.camera,target);scene.camera.data.ortho_scale=scale
    scene.render.resolution_x,scene.render.resolution_y=res;scene.render.resolution_percentage=100
    scene.render.image_settings.color_mode='RGBA';scene.render.film_transparent=False
    filename='Before_'+name.removeprefix('Preview_') if baseline else name
    scene.render.filepath=str(ROOT/'Previews'/filename)
    print('RENDER_START',name,flush=True);bpy.ops.render.render(write_still=True);print('RENDER_DONE',name,flush=True)
render('M7_Equipped_Tidal','Preview_Equipped_Tidal.png',(.99,2.10,.59),(.25,0,-.051),1.52,(2600,1300))
render('M7_Equipped_Tidal','Preview_Equipped_Tidal_Side.png',(.25,3.,-.051),(.25,0,-.051),1.52,(2600,1300))
render('M7_Equipped_Standard','Preview_Equipped_Standard.png',(.90,2,.56),(.17,0,-.048),1.32,(2400,1200))
render('M7_Equipped_Tidal','Preview_ResonanceGen1_Detail.png',(.514,.70,.15),(.441,0,-.092),.335,(1500,1300))
render('M7_Equipped_Tidal','Preview_Receiver_Magazine_Detail.png',(.22,.90,.25),(.09,0,-.087),.53,(1800,1350))
render('M7_Equipped_Tidal','Preview_UpperRail_Detail.png',(.69,.85,.20),(.600,0,.022),.245,(1800,1200))
render('M7_Default_Assembly','Preview_Default.png',(.89,1.95,.53),(.125,0,-.042),1.13,(2200,1100))
render('M7_Attachment_Catalog','Preview_Attachments.png',(0,2.6,.10),(0,0,-.07),2.40,(2400,1850))
# Temporary inspection scene: the barrel interfaces are aligned to compare
# actual lengths and geometry. It is never included in an asset export.
scene=bpy.data.scenes.new('M7_RENDER_Barrel_Comparison')
base=bpy.data.scenes['M7_Default_Assembly'];scene.world=base.world
style.lighting(base)
scene['m7_style_lighting']=style.VERSION
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
# A separate, nearly side-on view of the reference-driven Tidal combo.
single=bpy.data.scenes.new('M7_RENDER_TidalReference')
single.world=base.world
single['m7_style_lighting']=style.VERSION
for key in ('view_transform','look','exposure','gamma'):setattr(single.view_settings,key,getattr(base.view_settings,key))
for obj in base.objects:
    if obj.type not in ('LIGHT','CAMERA'):continue
    copy=obj.copy();copy.data=obj.data.copy();single.collection.objects.link(copy)
    if obj==base.camera:single.camera=copy
instance=bpy.data.objects.new('TidalReference_Only',None);single.collection.objects.link(instance)
instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections['SM_M7_Barrel_Tidal'];instance.location=(.158,0,0)
render(single.name,'Preview_Tidal_Reference.png',(.481,2.4,.006),(.481,0,.006),.77,(2400,1200))
render(single.name,'Preview_Tidal_ThreeQuarter.png',(.91,1.25,.42),(.481,0,.006),.78,(2400,1200))
render(single.name,'Preview_Tidal_Nose_Detail.png',(.73,.90,.14),(.600,0,.002),.17,(1400,1400))
render(single.name,'Preview_Tidal_Nose_Side.png',(.610,2.4,.001),(.610,0,.001),.150,(1600,1600))
# Explicit muzzle-direction cross-section review of the triangular shoulders.
if not requested or 'Preview_Tidal_Front_Profile.png' in requested:
    data=bpy.data.lights.new('Profile soft front fill','AREA');data.energy=55;data.shape='DISK';data.size=.6
    fill=bpy.data.objects.new('Profile soft front fill',data);single.collection.objects.link(fill)
    fill.location=(1.05,0,.35);aim(fill,(.46,0,.003))
render(single.name,'Preview_Tidal_Front_Profile.png',(1.25,0,.003),(.46,0,.003),.160,(1600,1600))
# Reference-crop inspection: receiver, magazine and rear grip only. This
# transient scene avoids a cropped optic covering the receiver side detail.
receiver=bpy.data.scenes.new('M7_RENDER_CoreReference');receiver.world=base.world
receiver['m7_style_lighting']=style.VERSION
for key in ('view_transform','look','exposure','gamma'):setattr(receiver.view_settings,key,getattr(base.view_settings,key))
for obj in base.objects:
    if obj.type not in ('LIGHT','CAMERA'):continue
    copy=obj.copy();copy.data=obj.data.copy();receiver.collection.objects.link(copy)
    if obj==base.camera:receiver.camera=copy
for name,mount in [('SM_M7_Core',(0,0,0)),('SM_M7_Mag_20',(.08,0,-.1)),('SM_M7_Grip_Default',(-.0625,0,-.065))]:
    instance=bpy.data.objects.new(name+'_CoreReference',None);receiver.collection.objects.link(instance)
    instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections[name];instance.location=mount
render(receiver.name,'Preview_Core_Reference_Side.png',(-.004,2.4,-.074),(-.004,0,-.074),.43,(2200,1800))
render(receiver.name,'Preview_RearRail_Detail.png',(-.13,2.4,.028),(-.13,0,.028),.125,(1600,1100))
# Standalone collection-instance previews of the two new replaceable assets.
for asset,mount,target,quarter,scale,res,stem in [
    ('SM_M7_Stock_ShadowRail',(-.179,0,-.0025),(-.30,0,-.04),(-.12,.70,.14),.35,(2200,1200),'ShadowRail'),
    ('SM_M7_Grip_Phantom',(-.0625,0,-.065),(-.10,0,-.131),(.08,.62,-.015),.235,(1700,1700),'Phantom')]:
    if not bpy.data.collections.get(asset):continue
    part=bpy.data.scenes.new('M7_RENDER_'+stem);part.world=base.world
    part['m7_style_lighting']=style.VERSION
    for key in ('view_transform','look','exposure','gamma'):setattr(part.view_settings,key,getattr(base.view_settings,key))
    for obj in base.objects:
        if obj.type not in ('LIGHT','CAMERA'):continue
        copy=obj.copy();copy.data=obj.data.copy();part.collection.objects.link(copy)
        if obj==base.camera:part.camera=copy
    instance=bpy.data.objects.new(asset+'_Preview_Only',None);part.collection.objects.link(instance)
    instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections[asset];instance.location=mount
    render(part.name,'Preview_'+stem+'_ThreeQuarter.png',quarter,target,scale,res)
    render(part.name,'Preview_'+stem+'_Side.png',(target[0],2.4,target[2]),target,scale,res)

# Both corrected silhouettes in one actual orthographic Blender render.
# Collection instances keep the source geometry and material graphs intact.
if 'Preview_Shadow_Phantom_Side.png' in requested or not requested:
    pair=bpy.data.scenes.new('M7_RENDER_ShadowPhantomSide');pair.world=base.world
    pair['m7_style_lighting']=style.VERSION
    for key in ('view_transform','look','exposure','gamma'):setattr(pair.view_settings,key,getattr(base.view_settings,key))
    for obj in base.objects:
        if obj.type not in ('LIGHT','CAMERA'):continue
        copy=obj.copy();copy.data=obj.data.copy();pair.collection.objects.link(copy)
        if obj==base.camera:pair.camera=copy
    for name,mount,offset,label,x in [
        ('SM_M7_Stock_ShadowRail',(-.179,0,-.0025),(.50,0,0),'SHADOW RAIL',.20),
        ('SM_M7_Grip_Phantom',(-.0625,0,-.065),(-.12,0,.055),'PHANTOM',-.22)]:
        instance=bpy.data.objects.new(name+'_SideReview',None);pair.collection.objects.link(instance)
        instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections[name]
        instance.location=Vector(mount)+Vector(offset)
        curve=bpy.data.curves.new(label+'_ReviewLabel','FONT');curve.body=label
        curve.size=.011;curve.align_x='CENTER';curve.materials.append(bpy.data.materials['M7R_07_EngravingGray'])
        text=bpy.data.objects.new(curve.name,curve);pair.collection.objects.link(text)
        text.location=(x,.08,-.180);text.rotation_euler=(math.pi/2,0,math.pi)
    render(pair.name,'Preview_Shadow_Phantom_Side.png',(0,2.4,-.06),(0,0,-.06),.72,(2400,1050))

# Compare the exposed accessory silhouettes in their actual receiver sockets.
# The internal grip tongue is naturally occluded by Core, as in the user crop.
if 'Preview_Shadow_Phantom_Installed_Side.png' in requested or not requested:
    installed=bpy.data.scenes.new('M7_RENDER_ShadowPhantomInstalled');installed.world=base.world
    installed['m7_style_lighting']=style.VERSION
    for key in ('view_transform','look','exposure','gamma'):setattr(installed.view_settings,key,getattr(base.view_settings,key))
    for obj in base.objects:
        if obj.type not in ('LIGHT','CAMERA'):continue
        copy=obj.copy();copy.data=obj.data.copy();installed.collection.objects.link(copy)
        if obj==base.camera:installed.camera=copy
    for name,mount in [('SM_M7_Core',(0,0,0)),
        ('SM_M7_Stock_ShadowRail',(-.179,0,-.0025)),
        ('SM_M7_Grip_Phantom',(-.0625,0,-.065))]:
        instance=bpy.data.objects.new(name+'_InstalledSideReview',None);installed.collection.objects.link(instance)
        instance.instance_type='COLLECTION';instance.instance_collection=bpy.data.collections[name];instance.location=mount
    render(installed.name,'Preview_Shadow_Phantom_Installed_Side.png',(-.214,2.4,-.063),(-.214,0,-.063),.455,(2400,1600))
