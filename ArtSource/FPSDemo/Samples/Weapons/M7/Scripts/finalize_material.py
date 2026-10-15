"""Save a normal editable material-study .blend with asset previews.

Run in a dedicated background Blender process after exporting and rendering
Previews/.  Cameras and lights are retained from the material-study renders;
this step does not rebake textures, modify geometry or rerender the previews.
"""

import ast
import json
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector
from bpy_extras.object_utils import world_to_camera_view


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_ATTACHMENTS = {"SM_M7_Stock_ShadowRail", "SM_M7_Grip_Phantom"}


def main():
    if not bpy.app.background:
        raise RuntimeError("Finalize the material sample in a separate background Blender process.")
    manifest = json.loads((ROOT / "AssetManifest.json").read_text(encoding="utf-8"))
    source = (ROOT / manifest.get("source_file", "Source/M7_Material_Study.blend")).resolve()
    if not source.is_relative_to((ROOT / "Source").resolve()) or source.suffix != ".blend":
        raise RuntimeError("The material source must be a .blend inside this sample's Source directory.")
    if len(manifest["assets"]) != 19:
        raise RuntimeError("Expected nineteen modular material-study assets.")
    if not REQUIRED_ATTACHMENTS.issubset({asset["name"] for asset in manifest["assets"]}):
        raise RuntimeError("The sample must include independent ShadowRail stock and Phantom grip assets.")
    if Path(bpy.data.filepath).resolve() != source:
        bpy.ops.wm.open_mainfile(filepath=str(source), load_ui=False)
    settings = manifest.get("scenes", {})
    edit = bpy.data.scenes[settings.get("edit", "M7_Library_Edit")]
    catalog = bpy.data.scenes[settings.get("catalog", "M7_Attachment_Catalog")]
    hero_name = manifest.get("preview_scene", "M7_Equipped_Tidal")
    hero = bpy.data.scenes.get(hero_name)
    if hero is None:
        hero = bpy.data.scenes[manifest["presets"][0]["scene"]]
    if hero.camera is None or catalog.camera is None:
        raise RuntimeError("Material hero and catalog scenes require their rendered cameras.")
    # The renderer intentionally does not save its input. Copy its actual
    # hero/catalog framing into the delivered file and thumbnail projection.
    recipe=ast.parse((ROOT/'Scripts/render_sample.py').read_text(encoding='utf-8'))
    for scene,filename in ((hero,'Preview_Equipped_Tidal.png'),(catalog,'Preview_Attachments.png')):
        call=next(node for node in ast.walk(recipe) if isinstance(node,ast.Call)
                  and isinstance(node.func,ast.Name) and node.func.id=='render'
                  and len(node.args)>=6 and isinstance(node.args[1],ast.Constant)
                  and node.args[1].value==filename)
        location,target,scale,res=[ast.literal_eval(arg) for arg in call.args[2:6]]
        scene.camera.location=location
        scene.camera.rotation_euler=(Vector(target)-Vector(location)).to_track_quat('-Z','Y').to_euler()
        scene.camera.data.ortho_scale=scale
        scene.render.resolution_x,scene.render.resolution_y=res
        scene.render.resolution_percentage=100
    bpy.context.window.scene = hero
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.region_3d.view_perspective = "CAMERA"
                area.spaces.active.shading.type = "MATERIAL"
                area.spaces.active.overlay.show_overlays = False
    images = {}
    source_materials = set()
    exclusions = [(layer, layer.exclude) for layer in edit.view_layers[0].layer_collection.children]

    def pixels(relative):
        path = (ROOT / relative).resolve()
        if not path.is_relative_to((ROOT / "Previews").resolve()):
            raise RuntimeError("Asset preview must be inside Previews/.")
        key = str(path)
        if key not in images:
            image = bpy.data.images.load(key, check_existing=False)
            width, height = image.size
            if width <= 0 or height <= 0:
                raise RuntimeError("Preview image has invalid dimensions: " + key)
            array = np.empty(width * height * 4, dtype=np.float32)
            image.pixels.foreach_get(array)
            images[key] = (image, array.reshape((height, width, 4)))
        return images[key][1]

    def thumbnail(array):
        height, width = array.shape[:2]
        if not width or not height:
            raise RuntimeError("A catalog asset lies outside the rendered preview frame.")
        scale = min(240 / width, 240 / height)
        w, h = max(1, round(width * scale)), max(1, round(height * scale))
        xs, ys = np.linspace(0, width - 1, w).astype(int), np.linspace(0, height - 1, h).astype(int)
        canvas = np.zeros((256, 256, 4), dtype=np.float32)
        x, y = (256 - w) // 2, (256 - h) // 2
        canvas[y:y + h, x:x + w] = array[ys[:, None], xs[None, :]]
        return canvas

    try:
        for layer, _ in exclusions:
            layer.exclude = False
        edit.view_layers[0].update()
        catalog.view_layers[0].update()
        with bpy.context.temp_override(scene=edit, view_layer=edit.view_layers[0]):
            depsgraph = bpy.context.evaluated_depsgraph_get()
            for row in manifest["assets"]:
                collection = bpy.data.collections[row["collection"]]
                if collection.asset_data is None:
                    collection.asset_mark()
                if row["category"] == "Core":
                    array = pixels(manifest.get("hero_preview", "Previews/Preview_Equipped_Tidal.png"))
                else:
                    array = pixels(manifest.get("catalog_preview", "Previews/Preview_Attachments.png"))
                    instance = next((obj for obj in catalog.objects if obj.instance_collection == collection), None)
                    if instance is None:
                        raise RuntimeError("Catalog instance missing for " + row["name"])
                    points = []
                    for original in collection.all_objects:
                        if original.type not in {"MESH", "FONT"}:
                            continue
                        source_materials.update(mat for mat in original.data.materials if mat is not None)
                        evaluated = original.evaluated_get(depsgraph)
                        transform = instance.matrix_world @ Matrix.Translation(-collection.instance_offset) @ original.matrix_world
                        points.extend(world_to_camera_view(catalog, catalog.camera, transform @ Vector(corner))
                                      for corner in evaluated.bound_box)
                    if not points:
                        raise RuntimeError("Asset has no preview geometry: " + row["name"])
                    height, width = array.shape[:2]
                    x0, x1 = max(0, int(min(point.x for point in points) * width) - 16), min(width, int(max(point.x for point in points) * width) + 17)
                    y0, y1 = max(0, int(min(point.y for point in points) * height) - 16), min(height, int(max(point.y for point in points) * height) + 17)
                    array = array[y0:y1, x0:x1]
                # Core also contributes material assets, despite using the hero
                # shot rather than a catalog crop for its collection preview.
                source_materials.update(mat for obj in collection.all_objects if obj.type in {"MESH", "FONT"}
                                        for mat in obj.data.materials if mat is not None)
                preview = collection.preview_ensure()
                preview.image_size = (256, 256)
                preview.image_pixels_float = thumbnail(array).ravel()
                preview.is_image_custom = True
    finally:
        for layer, excluded in exclusions:
            layer.exclude = excluded
        for image, _ in images.values():
            bpy.data.images.remove(image)
    for material in source_materials:
        if material.asset_data is None:
            material.asset_mark()
        if not material.asset_data.author:
            material.asset_data.author = "FPSDemo"
        tags = {tag.name for tag in material.asset_data.tags}
        for name in ("M7", "PBR", "MaterialSample"):
            if name not in tags:
                material.asset_data.tags.new(name)
        # Pack every referenced image, keeping source texture nodes and UV
        # bindings intact.  Legacy preserved materials remain self-contained.
        if material.use_nodes:
            for node in material.node_tree.nodes:
                if node.type == "TEX_IMAGE" and node.image is not None and node.image.packed_file is None:
                    node.image.pack()
    for name, relative in (("M7_Barrel_Switch.py", "Scripts/switch_barrel.py"),
                           ("M7_AssetManifest.json", "AssetManifest.json")):
        path = ROOT / relative
        if not path.is_file():
            raise RuntimeError("Required embedded sample script/data is missing: " + str(path))
        text = bpy.data.texts.get(name) or bpy.data.texts.new(name)
        text.clear()
        text.from_string(path.read_text(encoding="utf-8"))
        text.use_fake_user = True
    core_review=ROOT/'CoreReferenceReview.json'
    if core_review.is_file():
        review=json.loads(core_review.read_text(encoding='utf-8'))
        core=next(row for row in manifest['assets'] if row['name']=='SM_M7_Core')
        if not core.get('texture_sets'):
            raise RuntimeError('The revised Core must be freshly baked before finalization.')
        review['atlas_status']='Baked BaseColor, Normal and ORM; final export proof is in the matching Validation.json'
        review['atlas_resolution']=core['atlas_resolution']
        core_review.write_text(json.dumps(review,ensure_ascii=False,indent=2),encoding='utf-8')
    attachment_review=ROOT/'ShadowPhantomReview.json'
    if attachment_review.is_file():
        data=json.loads(attachment_review.read_text(encoding='utf-8'))
        new_rows=[row for row in manifest['assets'] if row['name'] in ('SM_M7_Stock_ShadowRail','SM_M7_Grip_Phantom')]
        if len(new_rows)!=2 or not all(row.get('texture_sets') for row in new_rows):
            raise RuntimeError('Both new attachments require baked portable atlases.')
        data['baked_atlases']={row['name']:{'resolution':row['atlas_resolution'],
             'files':next(iter(row['texture_sets'].values()))} for row in new_rows}
        data['atlas_status']='Baked portable BaseColor, Normal and ORM; final export proof is in matching Validation.json'
        attachment_review.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    source.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(source), compress=True)
    print(json.dumps({"source": str(source), "asset_previews": len(manifest["assets"]),
                      "marked_materials": len(source_materials), "ordinary_blend_saved": True}))


if __name__ == "__main__":
    main()
