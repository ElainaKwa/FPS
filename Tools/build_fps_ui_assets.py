"""Run in Unreal Python: import UI artwork and save editable Widget Blueprint trees."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / 'ArtSource/FPSDemo/Industrial/UI'
DEST = '/Game/FPSDemo/Art/Industrial/UI'
textures = [('T_UI_Panel', UI / 'Panel.png'), ('T_UI_Foundry', UI / 'Foundry.png')]
textures += [(p.stem, p) for p in sorted(UI.glob('T_UI_*.png'))]
assert len(textures) == 26
widgets_only = '-FPSUIWidgetsOnly' in unreal.SystemLibrary.get_command_line()
for name, source in ([] if widgets_only else textures):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f'{DEST}/{name}')
    assert texture, name
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture), f'Failed to save {name}'

result = unreal.FPSUIAssetBuilder.build_assets()
assert result.startswith('PASS'), result
assets = unreal.EditorAssetLibrary.list_assets(DEST, recursive=True, include_folder=False)
widgets = [x for x in assets if '/Widgets/' in x]
assert len(widgets) == 6, widgets
for path in widgets:
    bp = unreal.load_asset(path)
    assert bp and bp.generated_class(), path
report = {'status': 'PASS', 'generator': 'Tools/build_fps_ui_assets.py + FPSUIAssetBuilder.cpp',
          'text_font': 'MiSans Regular/Bold; Rajdhani Latin subfamily',
          'textures_reimported': not widgets_only,
          'widget_blueprints': widgets, 'theme': DEST + '/DA_FPSUITheme',
          'fonts': [x for x in assets if '/Fonts/' in x],
          'textures': [DEST + '/' + name for name, _ in textures],
          'note': 'Editable Designer widget trees; runtime delegates remain native C++.'}
UI.mkdir(parents=True, exist_ok=True)
(UI / 'AssetReport.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('FPS_UI_ASSETS ' + result)

