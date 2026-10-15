"""Import the M7 baked maps and build owned Unreal PBR materials.

Called by the M7 importer inside the editor or Python commandlet. This module
does not run on import. Blender/OpenGL normal maps are converted through the
Unreal texture setting; the source PNG files are never changed.
"""
import json
import math
import struct
from pathlib import Path

import unreal


OWNER_KEY = "m7_import_owner"
OWNER = "M7UnrealImport"
CHANNELS = ("BaseColor", "Normal", "ORM")


def _owned_or_missing(path, expected_type=None):
    lib = unreal.EditorAssetLibrary
    if not lib.does_asset_exist(path):
        return None
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError("Existing M7 destination cannot be loaded: " + path)
    if str(lib.get_metadata_tag(asset, OWNER_KEY)) != OWNER:
        raise RuntimeError("Refusing to overwrite an asset without M7 ownership: " + path)
    if expected_type is not None and not isinstance(asset, expected_type):
        raise TypeError("M7 destination has a different asset type: " + path)
    return asset


def _tag_and_save(asset, source=None):
    lib = unreal.EditorAssetLibrary
    lib.set_metadata_tag(asset, OWNER_KEY, OWNER)
    if source is not None:
        lib.set_metadata_tag(asset, "m7_source", str(source))
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError("Failed to save M7 material resource: " + asset.get_path_name())


def _create_or_load(path, asset_type, factory):
    asset = _owned_or_missing(path, asset_type)
    if asset is None:
        folder, name = path.rsplit("/", 1)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, asset_type, factory)
        if asset is None:
            raise RuntimeError("Failed to create M7 material resource: " + path)
        unreal.EditorAssetLibrary.set_metadata_tag(asset, OWNER_KEY, OWNER)
    return asset


def _png_size(path):
    with path.open("rb") as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        raise ValueError("Invalid M7 source PNG: " + str(path))
    return list(struct.unpack(">II", header[16:24]))


def _source_file(asset_root, relative):
    root = Path(asset_root).resolve()
    path = (root / relative).resolve()
    if not path.is_relative_to(root) or not path.is_file():
        raise ValueError("Missing or out-of-root M7 source resource: " + str(relative))
    return path


def _glb_materials(manifest, asset_root):
    """Read GLB JSON chunks without decoding their embedded image/buffer data."""
    required = set(manifest.get("preserved_constant_materials", ()))
    for row in manifest["assets"]:
        required.update(row.get("preserved_constant_materials", ()))
    found = {}
    for row in manifest["assets"]:
        path = _source_file(asset_root, row["exports"]["glb"])
        with path.open("rb") as stream:
            magic, version, total = struct.unpack("<4sII", stream.read(12))
            if magic != b"glTF" or version != 2 or total != path.stat().st_size:
                raise ValueError("Invalid GLB material source: " + str(path))
            length, kind = struct.unpack("<II", stream.read(8))
            if kind != 0x4E4F534A:
                raise ValueError("GLB does not begin with a JSON chunk: " + str(path))
            payload = json.loads(stream.read(length).decode("utf-8"))
        for material in payload.get("materials", ()):
            name = material.get("name")
            if name not in required:
                continue
            pbr = material.get("pbrMetallicRoughness", {})
            extensions = material.get("extensions", {})
            info = {
                "base_color_linear": pbr.get("baseColorFactor", [1, 1, 1, 1]),
                "metallic": pbr.get("metallicFactor", 1),
                "roughness": pbr.get("roughnessFactor", 1),
                "transmission": extensions.get("KHR_materials_transmission", {}).get("transmissionFactor", 0),
                "ior": extensions.get("KHR_materials_ior", {}).get("ior", 1.5),
                "two_sided": bool(material.get("doubleSided", False)),
            }
            if len(info["base_color_linear"]) != 4:
                raise ValueError("Invalid GLB constant color: " + name)
            values = [*info["base_color_linear"], info["metallic"], info["roughness"], info["transmission"], info["ior"]]
            if not all(isinstance(value, (int, float)) and math.isfinite(value) for value in values):
                raise ValueError("Non-finite GLB constant material: " + name)
            if name in found and info != found[name][0]:
                raise ValueError("Conflicting constant material definitions in GLBs: " + name)
            found[name] = (info, row["exports"]["glb"])
    missing = required - set(found)
    if missing:
        raise ValueError("Preserved M7 materials missing from GLB JSON: " + ", ".join(sorted(missing)))
    return found


def _expression(material, expression_class, x=0, y=0):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
    if node is None:
        raise RuntimeError("Failed to create M7 material expression: " + str(expression_class))
    return node


def _connect(node, output, prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(node, output, prop):
        raise RuntimeError("Cannot connect M7 material property: " + str(prop))


def _scalar(material, value, prop, x=-350, y=0):
    node = _expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", float(value))
    _connect(node, "", prop)
    return node


def _vector(material, values, x=-350, y=0):
    node = _expression(material, unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property("constant", unreal.LinearColor(*values[:3], 1.0))
    return node


def _import_texture(source, path, channel, report):
    _owned_or_missing(path, unreal.Texture2D)
    folder, name = path.rsplit("/", 1)
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("M7 PNG import failed: " + path + "; imported=" + str(task.imported_object_paths))
    compression = {
        "BaseColor": unreal.TextureCompressionSettings.TC_DEFAULT,
        "Normal": unreal.TextureCompressionSettings.TC_NORMALMAP,
        "ORM": unreal.TextureCompressionSettings.TC_MASKS,
    }[channel]
    texture.set_editor_property("compression_settings", compression)
    texture.set_editor_property("srgb", channel == "BaseColor")
    texture.set_editor_property("flip_green_channel", channel == "Normal")
    _tag_and_save(texture, source)
    observed = {
        "path": path,
        "source": str(source),
        "channel": channel,
        "source_resolution": _png_size(source),
        "srgb": bool(texture.get_editor_property("srgb")),
        "compression": str(texture.get_editor_property("compression_settings")),
        "flip_green_channel": bool(texture.get_editor_property("flip_green_channel")),
        "saved": True,
    }
    if observed["srgb"] != (channel == "BaseColor") or observed["flip_green_channel"] != (channel == "Normal"):
        raise RuntimeError("M7 texture settings did not persist: " + path)
    if texture.get_editor_property("compression_settings") != compression:
        raise RuntimeError("M7 texture compression did not persist: " + path)
    report["textures"].append(observed)
    return texture


def _baked_master(path, textures, report):
    mel = unreal.MaterialEditingLibrary
    master = _create_or_load(path, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(master)
    master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    master.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    master.set_editor_property("two_sided", True)
    for index, channel in enumerate(CHANNELS):
        node = _expression(master, unreal.MaterialExpressionTextureSampleParameter2D, -550, index * 240)
        node.set_editor_property("parameter_name", channel)
        node.set_editor_property("texture", textures[channel])
        node.set_editor_property("sampler_type", {
            "BaseColor": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
            "Normal": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
            "ORM": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
        }[channel])
        if channel == "ORM":
            _connect(node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
            _connect(node, "G", unreal.MaterialProperty.MP_ROUGHNESS)
            _connect(node, "B", unreal.MaterialProperty.MP_METALLIC)
        else:
            _connect(node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR if channel == "BaseColor" else unreal.MaterialProperty.MP_NORMAL)
    mel.layout_material_expressions(master)
    errors = list(mel.recompile_material(master))
    if errors:
        raise RuntimeError("M7 baked master compilation failed: " + "; ".join(str(error) for error in errors))
    _tag_and_save(master)
    report["materials"].append({"path": path, "type": "baked_master", "parameters": list(CHANNELS),
                                "orm_channels": {"R": "AmbientOcclusion", "G": "Roughness", "B": "Metallic"}, "saved": True})
    return master


def _constant_material(path, name, info, glb_source, report):
    mel = unreal.MaterialEditingLibrary
    material = _create_or_load(path, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    glass = info["transmission"] > 0
    material.set_editor_property("two_sided", info["two_sided"] or glass)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT if glass else unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT if glass else unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    if glass:
        material.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    color = _vector(material, info["base_color_linear"], -600, 0)
    _connect(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _scalar(material, info["metallic"], unreal.MaterialProperty.MP_METALLIC, -600, 160)
    _scalar(material, info["roughness"], unreal.MaterialProperty.MP_ROUGHNESS, -600, 280)
    # UE Specular is F0 / 0.08. Retain the GLB dielectric IOR without adding
    # volume/refraction behavior to this thin optical lens surface.
    f0 = ((info["ior"] - 1.0) / (info["ior"] + 1.0)) ** 2
    _scalar(material, min(1.0, max(0.0, f0 / .08)), unreal.MaterialProperty.MP_SPECULAR, -600, 400)
    if glass:
        _scalar(material, info["base_color_linear"][3], unreal.MaterialProperty.MP_OPACITY, -600, 520)
        transmittance = [min(1.0, max(0.0, value * info["transmission"])) for value in info["base_color_linear"][:3]]
        tint = _vector(material, transmittance, -600, 680)
        output = _expression(material, unreal.MaterialExpressionThinTranslucentMaterialOutput, -100, 680)
        input_names = list(mel.get_material_expression_input_names(output))
        target = next((str(value) for value in input_names if str(value).replace(" ", "").replace("_", "").lower() == "transmittancecolor"), None)
        if target is None or not mel.connect_material_expressions(tint, "", output, target):
            raise RuntimeError("Cannot connect ThinTranslucent transmittance color: " + str(input_names))
    mel.layout_material_expressions(material)
    errors = list(mel.recompile_material(material))
    if errors:
        raise RuntimeError("M7 constant material compilation failed: " + name + ": " + "; ".join(str(error) for error in errors))
    _tag_and_save(material, name)
    report["materials"].append({"path": path, "type": "preserved_constant", "source_slot": name,
                                "glb_source": glb_source, **info, "specular_f0": f0,
                                "shading": "ThinTranslucent/SurfaceForwardShading" if glass else "DefaultLit",
                                "transmittance_color_linear": transmittance if glass else None, "saved": True})
    return material


def build_materials(manifest, asset_root: Path, core_root: str, folder_for, report: dict):
    """Return (source slot -> MaterialInterface, source asset -> atlas MI).

    Every mesh has one unique baked atlas shared by all its ordinary material
    slots. Preserved glass/recess/engraving slots retain their GLB constants.
    All target paths are ownership-checked before the first resource mutation.
    """
    report.setdefault("textures", [])
    report.setdefault("materials", [])
    rows = list(manifest["assets"])
    constants = _glb_materials(manifest, asset_root)
    master_path = core_root + "/Materials/M_M7_BakedPBR"
    plans = []
    for row in rows:
        maps = row.get("texture_sets", {})
        unique = {tuple(channels[channel] for channel in CHANNELS) for channels in maps.values()}
        if len(unique) != 1:
            raise ValueError("M7 requires exactly one shared atlas per asset: " + row["name"])
        triple = next(iter(unique))
        folder = core_root if row["category"] == "Core" else folder_for(row)
        stem = row["name"].removeprefix("SM_")
        paths = {channel: folder + "/Textures/T_" + stem + "_" + channel for channel in CHANNELS}
        sources = {channel: _source_file(asset_root, relative) for channel, relative in zip(CHANNELS, triple)}
        plans.append((row, paths, sources, folder + "/Materials/MI_" + stem))
    _owned_or_missing(master_path, unreal.Material)
    for row, paths, sources, instance_path in plans:
        for channel in CHANNELS:
            _owned_or_missing(paths[channel], unreal.Texture2D)
            _png_size(sources[channel])
        _owned_or_missing(instance_path, unreal.MaterialInstanceConstant)
    constant_paths = {name: core_root + "/Materials/M_" + name for name in constants}
    for path in constant_paths.values():
        _owned_or_missing(path, unreal.Material)
    slot_materials = {}
    by_asset = {}
    master = None
    mel = unreal.MaterialEditingLibrary
    for row, paths, sources, instance_path in plans:
        textures = {channel: _import_texture(sources[channel], paths[channel], channel, report) for channel in CHANNELS}
        if master is None:
            master = _baked_master(master_path, textures, report)
        instance = _create_or_load(instance_path, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(instance, master)
        for channel in CHANNELS:
            # UE 5.8.1 MaterialEditingLibrary.cpp initializes bResult=false and
            # never sets it true, even after applying and updating the value.
            # The getter is the authoritative success check for this API.
            mel.set_material_instance_texture_parameter_value(instance, channel, textures[channel])
            assigned = mel.get_material_instance_texture_parameter_value(instance, channel)
            if assigned != textures[channel]:
                raise RuntimeError("Failed to set M7 material texture parameter: " + instance_path + ":" + channel)
        mel.update_material_instance(instance)
        _tag_and_save(instance, row["name"])
        observed = {}
        for channel in CHANNELS:
            assigned = mel.get_material_instance_texture_parameter_value(instance, channel)
            if assigned != textures[channel]:
                raise RuntimeError("M7 material texture parameter mismatch: " + instance_path + ":" + channel)
            observed[channel] = paths[channel]
        for slot in row["texture_sets"]:
            if slot in slot_materials and slot_materials[slot] != instance:
                raise ValueError("Ambiguous M7 atlas material slot: " + slot)
            slot_materials[slot] = instance
        by_asset[row["name"]] = instance
        report["materials"].append({"path": instance_path, "type": "baked_instance", "source_asset": row["name"],
                                    "parent": master_path, "parameters": observed,
                                    "source_slots": sorted(row["texture_sets"]), "saved": True})
    for name, (info, glb_source) in sorted(constants.items()):
        if name in slot_materials:
            raise ValueError("Preserved M7 constant unexpectedly assigned an atlas: " + name)
        slot_materials[name] = _constant_material(constant_paths[name], name, info, glb_source, report)
    return slot_materials, by_asset
