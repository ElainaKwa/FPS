"""Render saved Blender source without modifying it."""
import bpy,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
scene=bpy.context.scene
prefs=bpy.context.preferences.addons['cycles'].preferences
try:
    prefs.compute_device_type='OPTIX';prefs.get_devices()
    for d in prefs.devices:d.use=d.type=='OPTIX'
    if any(d.use for d in prefs.devices):scene.cycles.device='GPU'
except Exception:pass
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for name in args or ['Hero','Front','Rear']:
    scene.camera=bpy.data.objects['CAM_'+name];scene.render.filepath=str(ROOT/'Previews'/(name+'.png'))
    bpy.ops.render.render(write_still=True);print('AMMO_CRATE_RENDER_PASS',name,flush=True)
