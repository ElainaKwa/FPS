"""Finalize the isolated library scene into a normal standalone editor file."""
import bpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
assert bpy.app.background, 'Use a separate background process; never replace the live editing scene.'
scene=bpy.data.scenes['M7_BLACK_Studio']
bpy.context.window.scene=scene
assert len(bpy.data.scenes)==1
asset=bpy.data.collections['ASSET_M7_BLACK']
with bpy.context.temp_override(id=asset):
    bpy.ops.ed.lib_id_load_custom_preview(filepath=str(ROOT/'Preview_Hero.png'))
bpy.ops.object.select_all(action='DESELECT')
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            space=area.spaces.active
            space.region_3d.view_perspective='CAMERA'
            space.shading.type='MATERIAL'
            space.overlay.show_overlays=False
bpy.context.view_layer.update()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'M7_BLACK_Stylized.blend'),compress=True)
print('M7 standalone source finalized, collection preview embedded')
