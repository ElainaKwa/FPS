"""Unreal Python commandlet: build isolated Chinese font comparison widgets."""
import hashlib
import json
from pathlib import Path
import unreal

root = Path(__file__).resolve().parents[1]
command = unreal.SystemLibrary.get_command_line()
variant = next((name for name in ('MiSans', 'HarmonyOS')
                if f'-FPSUIFontVariant={name}' in command), None)
assert variant, 'Pass -FPSUIFontVariant=MiSans or HarmonyOS'
baseline = {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in (root/'Content/FPSDemo/Art/Industrial/UI').rglob('*.uasset')}
result = unreal.FPSUIAssetBuilder.build_assets()
assert result.startswith('PASS'), result
destination = f'/Game/FPSDemo/Art/FontComparisons/{variant}/UI'
assets = list(unreal.EditorAssetLibrary.list_assets(destination, recursive=True, include_folder=False))
widgets = [path for path in assets if '/Widgets/' in path]
assert len(widgets) == 6, widgets
for path in widgets:
    assert unreal.load_asset(path).generated_class(), path
for path, expected in baseline.items():
    assert hashlib.sha256((root/path).read_bytes()).hexdigest() == expected, f'Base asset changed: {path}'
out = root/'ArtSource/FPSDemo/Samples/UI/FontComparisons'
out.mkdir(parents=True, exist_ok=True)
(out/f'{variant}-Assets.json').write_text(json.dumps({
    'generated_by': 'Tools/build_fps_font_comparison.py', 'status': 'PASS',
    'variant': variant, 'assets': assets, 'base_ui_assets_unchanged': len(baseline),
    'profile': 'Regular/Bold Chinese; existing Rajdhani English/numbers; same sizes/layout',
}, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log(f'FPS_FONT_COMPARISON PASS {variant} assets={len(assets)}')
