"""Render the saved standalone study; never opens or modifies the live Blender session."""
import bpy, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
quick='--quick' in args
names=[x for x in args if not x.startswith('--')] or ['Hero','Profile','Receiver','FirstPerson']
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
    scene.camera=bpy.data.objects['CAM_'+name]
    scene.render.filepath=str(ROOT/'Previews'/((name+'_Draft' if quick else name)+'.png'))
    bpy.ops.render.render(write_still=True)
    print('RENDER_PASS',name,flush=True)
