"""Apply the project's restrained industrial presentation to this sample."""
import bpy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
VERSION='industrial-light-v1'

def lighting(scene):
    if scene.get('m7_style_lighting')==VERSION:return
    for obj in scene.objects:
        if obj.type!='LIGHT':continue
        if obj.name.endswith('_Key'):obj.data.energy*=.75
        elif obj.name.endswith('_Rim'):obj.data.energy*=.85
    scene.view_settings.exposure-=.12
    scene['m7_style_lighting']=VERSION

def main():
    path=ROOT/'AssetManifest.json';manifest=json.loads(path.read_text(encoding='utf-8'))
    for scene in bpy.data.scenes:
        if scene.name in manifest['saved_scenes']:lighting(scene)
    changed=[]
    for obj in bpy.data.objects:
        if obj.type=='FONT' and obj.data.body=='M7  PRACTICAL':
            obj.data.body='M7';changed.append(obj.name)
        if obj.type=='FONT':assert 'MCP' not in obj.data.body.upper(),'Project codename must not appear on the asset.'
    config=json.loads((ROOT/'Scripts/material_settings.json').read_text(encoding='utf-8'))
    report_path=ROOT/'MaterialReport.json';report=json.loads(report_path.read_text(encoding='utf-8'))
    for data in (manifest,report):
        data['style_basis']=config['style_basis']
        data['style_interpretation']=config['style_interpretation']
        data['lighting_recipe']={'version':VERSION,'key_power_multiplier':.75,'rim_power_multiplier':.85,'exposure_offset':-.12,
                                 'comparison':'The same recipe is applied when rendering the historical baseline.'}
    report['functional_marking_changes']=changed
    path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.shading.use_scene_lights=True
                area.spaces.active.shading.use_scene_world=True
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source/M7_Material_Study.blend'),compress=True)
    print(json.dumps({'style_basis':config['style_basis'],'lighting_recipe':VERSION,'functional_marking_changes':changed},ensure_ascii=False))
if __name__=='__main__':main()
