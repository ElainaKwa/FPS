"""Create missing lobby resources; existing Designer composition is preserved."""
from pathlib import Path
import unreal
SUITE = Path(__file__).resolve().parents[1]
DEST = '/Game/FPSDemo/Art/UI/ExtractionTerminal'
def ensure_assets(reimport=False):
    for name in ('T_LobbyVignette', 'T_LobbyIndustrialCrossing'):
        package = DEST + '/Textures/' + name
        if not unreal.EditorAssetLibrary.does_asset_exist(package) or reimport:
            task = unreal.AssetImportTask()
            task.filename = str(SUITE / 'Textures' / (name + '.png'))
            task.destination_path = DEST + '/Textures'
            task.destination_name = name
            task.automated = True
            task.replace_existing = True
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            texture = unreal.load_asset(package)
            texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    if not unreal.EditorAssetLibrary.does_asset_exist(DEST + '/DA_LobbyPresentation'):
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.FPSExtractionLobbyPresentation)
        composition = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_LobbyPresentation', DEST, unreal.FPSExtractionLobbyPresentation, factory)
        assert composition
        composition.set_editor_property('look_at_offset', unreal.Vector(0,840,-255))
        composition.set_editor_property('robot_offset', unreal.Vector(45,720,-245))
        composition.set_editor_property('field_of_view',65)
        assert composition and unreal.EditorAssetLibrary.save_loaded_asset(composition)
