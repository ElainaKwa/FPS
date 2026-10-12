"""Render default M7, interchangeable build and labeled component catalog."""
import bpy,math,sys,json
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
prefs=bpy.context.preferences.addons['cycles'].preferences
enum=[i.identifier for i in prefs.bl_rna.properties['compute_device_type'].enum_items]
try:prefs.compute_device_type='OPTIX'
except TypeError:
    try:prefs.compute_device_type='CUDA'
    except TypeError:pass
prefs.get_devices()
gpu=False
for d in prefs.devices:
    d.use=d.type in ('OPTIX','CUDA') and d.type==prefs.compute_device_type
    gpu=gpu or d.use

def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
def render(scene_name,filename,loc,target,scale,res,alpha=False):
    scene=bpy.data.scenes[scene_name];bpy.context.window.scene=scene
    scene.render.engine='CYCLES'
    scene.cycles.device='GPU' if gpu else 'CPU'
    scene.cycles.samples=64;scene.cycles.use_denoising=True
    cam=scene.camera;cam.location=loc;aim(cam,target);cam.data.ortho_scale=scale
    scene.render.resolution_x,scene.render.resolution_y=res
    scene.render.resolution_percentage=100
    scene.render.film_transparent=alpha
    scene.render.image_settings.color_mode='RGBA'
    scene.render.filepath=str(ROOT/filename)
    print('RENDER_START',filename,flush=True)
    bpy.ops.render.render(write_still=True)
    print('RENDER_DONE',filename,flush=True)

if '--suppressed-only' in sys.argv:
    render('M7_Suppressed_Assembly','Preview_Suppressed.png',(.89,1.95,.53),(.17,0,-.035),1.32,(2200,1100))
    raise SystemExit(0)

render('M7_Default_Assembly','Preview_Default.png',(.89,1.95,.53),(.125,0,-.042),1.13,(2200,1100))
# Exact pixel mapping for comparison against the reference's side silhouette.
render('M7_Default_Assembly','Preview_Reference_Profile.png',(.125,2,-.060),(.125,0,-.060),1.0,(2000,1000),True)
if '--skip-detail' not in sys.argv:
    render('M7_Default_Assembly','Preview_Core_Detail.png',(.27,1,.27),(.14,0,-.035),.64,(1700,1100))
if '--default-only' not in sys.argv:
    render('M7_Suppressed_Assembly','Preview_Suppressed.png',(.89,1.95,.53),(.17,0,-.035),1.32,(2200,1100))

catalog=bpy.data.scenes['M7_Attachment_Catalog']
label=bpy.data.materials.new('M7_Catalog_Label');label.use_nodes=True
n=next(n for n in label.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
n.inputs['Base Color'].default_value=(.40,.42,.44,1)
n.inputs['Emission Color'].default_value=(.25,.27,.30,1);n.inputs['Emission Strength'].default_value=.45
data=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
items=[r for r in data['assets'] if r['category']!='Core']
for i,row in enumerate(items):
    if bpy.data.objects.get('Label_'+row['name']):continue
    col=i%4;rr=i//4
    x=(col-1.5)*.45;z=(1.5-rr)*.31
    font=bpy.data.curves.new('Label_'+row['name'],'FONT')
    font.body=row['name'].removeprefix('SM_M7_').replace('_',' / ')
    font.size=.013
    font.materials.append(label)
    o=bpy.data.objects.new(font.name,font);catalog.collection.objects.link(o)
    o.location=(x+.17,.08,z-.144);o.rotation_euler=(math.pi/2,0,math.pi)
render('M7_Attachment_Catalog','Preview_Attachments.png',(0,2.6,.10),(0,0,.02),2.05,(2200,1500))
