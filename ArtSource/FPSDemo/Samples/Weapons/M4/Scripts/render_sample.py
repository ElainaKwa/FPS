"""Render the saved standalone study; never opens or modifies the live Blender session."""
import bpy, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
quick='--quick' in args
names=[x for x in args if not x.startswith('--')] or ['Hero','Profile','Receiver','FirstPerson','Grip','Stock']
scene=bpy.context.scene
scene.render.engine='CYCLES'
prefs=bpy.context.preferences.addons['cycles'].preferences
for kind in ['OPTIX','CUDA']:
    try:
        prefs.compute_device_type=kind;prefs.get_devices()
        devices=[d for d in prefs.devices if d.type==kind]
        if devices:
            for d in prefs.devices:d.use=d.type==kind
            scene.cycles.device='GPU';print('RENDER_DEVICE',kind,flush=True);break
    except Exception:pass
scene.cycles.samples=32 if quick else 128
scene.cycles.use_denoising=True
scene.render.resolution_x=1600 if quick else 2560
scene.render.resolution_y=900 if quick else 1440
scene.render.resolution_percentage=100
for name in names:
    if name=='Stock' and 'CAM_Stock' not in bpy.data.objects:
        data=bpy.data.cameras.new('CAM_Stock');cam=bpy.data.objects.new('CAM_Stock',data)
        bpy.data.collections['STUDIO_NOT_FOR_EXPORT'].objects.link(cam)
        cam.location=(-.50,-.53,.14);cam.rotation_euler=(Vector((-.285,0,-.015))-cam.location).to_track_quat('-Z','Y').to_euler()
        data.lens=58;data.clip_start=.01
    if name=='Grip' and 'CAM_Grip' not in bpy.data.objects:
        data=bpy.data.cameras.new('CAM_Grip');cam=bpy.data.objects.new('CAM_Grip',data)
        bpy.data.collections['STUDIO_NOT_FOR_EXPORT'].objects.link(cam)
        cam.location=(-.23,-.48,.074);cam.rotation_euler=(Vector((-.06,0,-.067))-cam.location).to_track_quat('-Z','Y').to_euler()
        data.lens=55;data.clip_start=.01
    scene.camera=bpy.data.objects['CAM_'+name]
    # A clean neutral backdrop makes the close perspective easier to judge.
    bpy.data.objects['Studio_ground'].hide_render=name=='FirstPerson'
    scene.render.filepath=str(ROOT/'Previews'/((name+'_Draft' if quick else name)+'.png'))
    bpy.ops.render.render(write_still=True)
    print('RENDER_PASS',name,flush=True)
