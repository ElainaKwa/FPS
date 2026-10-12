"""Render three presentation views from the saved source; no source mutation."""
import bpy
import math
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
scene=bpy.context.scene
cam=scene.camera
scene.render.engine='CYCLES'
scene.cycles.samples=64
scene.cycles.use_denoising=True
preferences=bpy.context.preferences.addons['cycles'].preferences
available=[i.identifier for i in preferences.bl_rna.properties['compute_device_type'].enum_items]
if 'OPTIX' in available:
    preferences.compute_device_type='OPTIX'
    preferences.get_devices()
    for device in preferences.devices:device.use=device.type=='OPTIX'
    scene.cycles.device='GPU'
else:
    scene.cycles.device='CPU'

def render(name,loc,target,scale,res):
    cam.location=loc
    cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=scale
    scene.render.resolution_x,scene.render.resolution_y=res
    scene.render.resolution_percentage=100
    scene.render.filepath=str(ROOT/name)
    print('RENDER_START',name,flush=True)
    bpy.ops.render.render(write_still=True)
    print('RENDER_DONE',name,flush=True)

profile_only='--profile-only' in sys.argv
if not profile_only:
    render('Preview_Hero.png',(.95,-2.1,.72),(.13,0,-.025),1.40,(2000,1100))
bpy.data.objects['M7_StudioGround'].hide_render=True
render('Preview_Profile.png',(.15,-2,-.02),(.15,0,-.02),1.35,(2000,880))
bpy.data.objects['M7_StudioGround'].hide_render=False
if not profile_only:
    render('Preview_Detail.png',(.75,-1.45,.53),(.13,0,.026),.80,(1600,1000))
