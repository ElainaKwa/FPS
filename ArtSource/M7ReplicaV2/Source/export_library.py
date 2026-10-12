"""Export evaluated M7 V2 barrel combos and resolved modular attachments.

Run once through Blender Python after build_library.py and visual corrections.
The only operator targets are derived objects in a temporary export scene.
Existing source UVs and packed material images are retained.  Individual
assets use the specified mount as their local origin; assembled presets use
the world coordinates resolved by the selected Barrel combo.
"""

import bpy
import json
import math
import secrets
from pathlib import Path
from mathutils import Matrix, Vector


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Exports"
MANIFEST_PATH = ROOT / "AssetManifest.json"
DEPENDENT_MOUNT_KEYS = ("Foregrip", "Muzzle", "FrontIron")


def fixed_mounts(manifest):
    """Receiver-owned sockets; forward sockets belong to the selected barrel."""
    return {key: list(value) for key, value in manifest["mounts"].items()
            if key not in {"Core", *DEPENDENT_MOUNT_KEYS}}


def resolve_preset(manifest, rows_by_name, preset):
    """Resolve attachment origins without altering their authored asset origin."""
    parts = list(preset["parts"])
    if not parts or len(parts) != len(set(parts)):
        raise RuntimeError("Preset parts must be nonempty and unique: " + preset["name"])
    if not all(name in rows_by_name for name in parts):
        raise RuntimeError("Preset references an unknown asset: " + preset["name"])
    barrels = [name for name in parts if rows_by_name[name]["mount_key"] == "Barrel"]
    cores = [name for name in parts if rows_by_name[name]["category"] == "Core"]
    if len(barrels) != 1 or len(cores) != 1:
        raise RuntimeError("Every preset must contain exactly one Core and one Barrel combo.")
    occupied_keys = [rows_by_name[name]["mount_key"] for name in parts]
    if len(occupied_keys) != len(set(occupied_keys)):
        raise RuntimeError("A preset cannot install multiple assets at the same mount.")
    barrel_name = barrels[0]
    barrel = rows_by_name[barrel_name]
    local = barrel["local_mounts_m"]
    if not all(key in local for key in DEPENDENT_MOUNT_KEYS):
        raise RuntimeError("Barrel must declare local Foregrip, Muzzle and FrontIron mounts: " + barrel_name)
    sockets = fixed_mounts(manifest)
    if "Barrel" not in sockets:
        raise RuntimeError("Manifest must declare the receiver's stable Barrel mount.")
    barrel_origin = sockets["Barrel"]
    for key, value in local.items():
        if len(value) != 3 or not all(math.isfinite(float(x)) for x in value):
            raise RuntimeError("Invalid barrel socket: " + barrel_name + ":" + key)
        if key in sockets:
            raise RuntimeError("Barrel-owned socket conflicts with a fixed receiver socket: " + key)
        sockets[key] = [barrel_origin[i] + value[i] for i in range(3)]
    placements = {}
    for name in parts:
        asset = rows_by_name[name]
        key = asset["mount_key"]
        allowed = asset.get("compatible_barrels")
        if allowed and barrel_name not in allowed:
            raise RuntimeError(name + " is not compatible with " + barrel_name)
        if key == "Core":
            placements[name] = list(asset["mount_position_m"])
        elif key in sockets:
            placements[name] = list(sockets[key])
        else:
            raise RuntimeError("No resolved socket for asset " + name + ":" + key)
    return {"barrel_asset": barrel_name, "barrel_local_mounts_m": dict(local),
            "part_mounts_m": placements, "mounts_m": sockets}


def main():
    if bpy.context.window is None:
        raise RuntimeError("This exporter requires the live Blender window context.")
    if bpy.context.mode != "OBJECT":
        raise RuntimeError("Return Blender to Object Mode before exporting the library.")
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    source_path = (ROOT / manifest.get("source_file", "M7_Modular_Replica_V2.blend")).resolve()
    if not source_path.is_relative_to(ROOT.resolve()) or source_path.suffix.lower() != ".blend":
        raise RuntimeError("Source library must be a .blend file inside the V2 directory.")
    rows = manifest["assets"]
    if not rows:
        raise RuntimeError("The V2 manifest contains no assets.")
    if "expected_asset_count" in manifest and len(rows) != manifest["expected_asset_count"]:
        raise RuntimeError("Asset count differs from manifest.expected_asset_count.")
    row_by_name = {row["name"]: row for row in rows}
    if len(row_by_name) != len(rows):
        raise RuntimeError("Manifest asset names must be unique.")
    defaults = list(manifest["default_parts"])
    if not defaults or len(defaults) != len(set(defaults)):
        raise RuntimeError("default_parts must be nonempty and unique.")
    preset_definitions = [dict(row) for row in manifest.get("presets", [])]
    if not preset_definitions:
        equipped = manifest.get("equipped_parts")
        if not equipped:
            raise RuntimeError("Declare presets or equipped_parts in the V2 manifest.")
        preset_definitions = [
            {"name": "SM_M7_Default_Assembled", "parts": defaults, "scene": "M7_Default_Assembly"},
            {"name": "SM_M7_Equipped_Assembled", "parts": equipped, "scene": "M7_Equipped_Assembly"},
        ]
    if len({row["name"] for row in preset_definitions}) != len(preset_definitions):
        raise RuntimeError("Preset names must be unique.")
    scene_names = manifest.get("scenes", {})
    source_name = scene_names.get("edit", "M7_Library_Edit")
    default_scene_name = scene_names.get("default", "M7_Default_Assembly")
    catalog_name = scene_names.get("catalog", "M7_Attachment_Catalog")
    for preset in preset_definitions:
        preset.setdefault("scene", default_scene_name if preset["parts"] == defaults else "M7_Equipped_Assembly")
        preset.update(resolve_preset(manifest, row_by_name, preset))
    if len({row["scene"] for row in preset_definitions}) != len(preset_definitions):
        raise RuntimeError("Each preset must declare its own unique scene name.")
    source = bpy.data.scenes.get(source_name)
    assembly = bpy.data.scenes.get(default_scene_name)
    catalog = bpy.data.scenes.get(catalog_name)
    if source is None or assembly is None or catalog is None:
        raise RuntimeError("Build the editable library, default assembly and catalog first.")
    collections = {}
    for row in rows:
        collection = bpy.data.collections.get(row["collection"])
        if collection is None:
            raise RuntimeError(f"Missing source collection: {row['collection']}")
        geometry = [obj for obj in collection.all_objects if obj.type in {"MESH", "FONT"}]
        if not geometry:
            raise RuntimeError(f"Source asset has no geometry: {row['name']}")
        collections[row["name"]] = collection

    OUT.mkdir(parents=True, exist_ok=True)
    token = secrets.token_hex(5)
    temp = bpy.data.scenes.new("M7_EXPORT_TEMP_" + token)
    temp.unit_settings.system = "METRIC"
    temp.unit_settings.scale_length = 1
    temp_collection = bpy.data.collections.new("M7_EXPORT_DERIVED_" + token)
    temp.collection.children.link(temp_collection)
    derived_mesh_pointers = set()
    local_assets = {}
    local_roots = {}
    source_excludes = []
    source_layer = source.view_layers[0]
    export_ok = False

    def walk_layers(layer):
        for child in layer.children:
            yield child
            yield from walk_layers(child)

    def select_only(objects):
        # Never select original objects: every operation runs in temp's layer.
        if bpy.context.window.scene != temp:
            raise RuntimeError("Export selection attempted outside the temporary scene.")
        layer = bpy.context.view_layer
        for obj in layer.objects:
            if obj.select_get():
                obj.select_set(False)
        for obj in objects:
            if obj.name not in layer.objects:
                raise RuntimeError(f"Derived object missing from export layer: {obj.name}")
            obj.select_set(True)
        layer.objects.active = objects[0] if objects else None

    def remember_mesh(data):
        derived_mesh_pointers.add(data.as_pointer())
        return data

    def fill_missing_uv(obj):
        if obj.data.uv_layers:
            return
        select_only([obj])
        bpy.ops.object.mode_set(mode="EDIT")
        try:
            bpy.ops.mesh.select_all(action="SELECT")
            bpy.ops.uv.smart_project(angle_limit=1.151917,
                                     island_margin=.012, scale_to_bounds=True)
        finally:
            bpy.ops.object.mode_set(mode="OBJECT")

    def deduplicate_materials(obj):
        """Remove unused/repeated slots while preserving every face assignment."""
        data = obj.data
        old_slots = list(data.materials)
        new_slots, remap = [], {}
        for polygon in data.polygons:
            old_index = polygon.material_index
            if old_index in remap:
                continue
            if old_index >= len(old_slots) or old_slots[old_index] is None:
                raise RuntimeError(f"Missing source face material in {obj.name}")
            mat = old_slots[old_index]
            if mat not in new_slots:
                new_slots.append(mat)
            remap[old_index] = new_slots.index(mat)
        assignments = [remap[polygon.material_index] for polygon in data.polygons]
        data.materials.clear()
        for mat in new_slots:
            data.materials.append(mat)
        for polygon, index in zip(data.polygons, assignments):
            polygon.material_index = index
        data.update()

    def join_derived(objects, name):
        if not objects:
            raise RuntimeError(f"Cannot join empty derived asset {name}")
        select_only(objects)
        if len(objects) > 1:
            result = bpy.ops.object.join()
            if "FINISHED" not in result:
                raise RuntimeError(f"Blender could not join derived asset {name}")
        obj = bpy.context.view_layer.objects.active
        # Applied transforms are geometry-only changes to the derived copy.
        select_only([obj])
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        obj.name = name
        obj.data.name = name + "_Geometry_Baked"
        remember_mesh(obj.data)
        deduplicate_materials(obj)
        return obj

    def stats(obj, bounds_key="local_bounds_m"):
        data = obj.data
        data.calc_loop_triangles()
        if not data.vertices:
            raise RuntimeError(f"Empty evaluated geometry for {obj.name}")
        # The objects are identity transforms; use actual vertices, not cached
        # object bounds, so the manifest exactly describes exported coordinates.
        minimum = [min(vertex.co[k] for vertex in data.vertices) for k in range(3)]
        maximum = [max(vertex.co[k] for vertex in data.vertices) for k in range(3)]
        return {
            "triangles": len(data.loop_triangles),
            "vertices": len(data.vertices),
            "used_materials": [mat.name for mat in data.materials],
            bounds_key: {"min": minimum, "max": maximum},
            "geometry_dimensions_m": [maximum[k] - minimum[k] for k in range(3)],
            "uv_layers": [layer.name for layer in data.uv_layers],
        }

    def new_root(name):
        root = bpy.data.objects.new(name + "_Root", None)
        temp_collection.objects.link(root)
        root.matrix_world = Matrix.Identity(4)
        root.empty_display_size = .014
        return root

    def check_export(result, path):
        if "FINISHED" not in result or not path.is_file() or path.stat().st_size == 0:
            raise RuntimeError(f"Export did not produce a valid file: {path}")

    def export_glb(name, root, meshes, mounts_m=None, barrel_mesh=None, barrel_local_mounts_m=None):
        mounts = []
        by_key = {}
        worlds = [(mesh, mesh.matrix_world.copy()) for mesh in meshes]
        mounts_m = mounts_m or {}
        barrel_local_mounts_m = barrel_local_mounts_m or {}
        for key, position in mounts_m.items():
            mount = bpy.data.objects.new("Mount_" + key, None)
            temp_collection.objects.link(mount)
            if barrel_mesh is not None and key in barrel_local_mounts_m:
                mount.parent = barrel_mesh
                mount.location = Vector(barrel_local_mounts_m[key])
                mount["socket_owner"] = barrel_mesh.get("source_asset", barrel_mesh.name)
            else:
                mount.parent = root
                mount.location = Vector(position)
                mount["socket_owner"] = "Core" if key not in DEPENDENT_MOUNT_KEYS else "Barrel"
            mount.matrix_parent_inverse = Matrix.Identity(4)
            mount.empty_display_size = .012
            mount["mount_key"] = key
            mount["mount_position_m"] = list(position)
            mounts.append(mount)
            by_key[key] = mount
        try:
            # Preset meshes attach at their origin to the actual socket node.
            # Forward sockets live beneath the selected Barrel mesh node, so
            # moving its branch carries the foregrip and muzzle attachment.
            for mesh, world in worlds:
                key = mesh.get("mount_key", "Core")
                if key in by_key:
                    mesh.parent = by_key[key]
                    mesh.matrix_parent_inverse = Matrix.Identity(4)
                    # The matching socket already holds the component origin.
                    # Explicit local identity avoids stale parent matrices
                    # turning the authored world offset into a second offset.
                    mesh.matrix_basis = Matrix.Identity(4)
            bpy.context.view_layer.update()
            select_only(meshes + mounts + [root])
            path = OUT / (name + ".glb")
            result = bpy.ops.export_scene.gltf(
                filepath=str(path), export_format="GLB", use_selection=True,
                use_active_scene=True, export_apply=False, export_yup=True,
                export_cameras=False, export_lights=False, export_animations=False,
                export_materials="EXPORT", export_image_format="AUTO", export_extras=True,
            )
            check_export(result, path)
            return path.relative_to(ROOT).as_posix()
        finally:
            # Restore each derived world transform before deleting sockets;
            # FBX can then join the components without losing parent offsets.
            for mesh, world in worlds:
                mesh.parent = root
                mesh.matrix_parent_inverse = Matrix.Identity(4)
                mesh.matrix_basis = world
            bpy.context.view_layer.update()
            for mount in mounts:
                bpy.data.objects.remove(mount, do_unlink=True)

    def export_fbx(name, mesh):
        select_only([mesh])
        path = OUT / (name + ".fbx")
        result = bpy.ops.export_scene.fbx(
            filepath=str(path), use_selection=True, object_types={"MESH"},
            axis_forward="X", axis_up="Z", use_space_transform=True,
            apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
            mesh_smooth_type="FACE", use_mesh_modifiers=False,
            add_leaf_bones=False, bake_anim=False,
            path_mode="COPY", embed_textures=True,
        )
        check_export(result, path)
        return path.relative_to(ROOT).as_posix()

    def export_preset(preset):
        name, parts = preset["name"], list(preset["parts"])
        resolved = resolve_preset(manifest, row_by_name, preset)
        root = new_root(name)
        root["asset_library"] = manifest["library"]
        root["assembly_parts"] = json.dumps(parts)
        root["barrel_asset"] = resolved["barrel_asset"]
        meshes = []
        barrel_mesh = None
        for asset_name in parts:
            source_mesh = local_assets[asset_name]
            data = remember_mesh(source_mesh.data.copy())
            obj = bpy.data.objects.new(asset_name, data)
            temp_collection.objects.link(obj)
            obj.parent = root
            obj.matrix_world = Matrix.Translation(Vector(resolved["part_mounts_m"][asset_name]))
            obj["source_asset"] = asset_name
            obj["mount_key"] = row_by_name[asset_name]["mount_key"]
            if asset_name == resolved["barrel_asset"]:
                barrel_mesh = obj
            meshes.append(obj)
        glb_path = export_glb(name, root, meshes, resolved["mounts_m"],
                              barrel_mesh, resolved["barrel_local_mounts_m"])
        # Freeze all resolved component transforms before the FBX join.  These
        # are derived copies, never source asset definitions or their UV data.
        select_only(meshes)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        combined = join_derived(meshes, name)
        combined.parent = None
        combined.matrix_world = Matrix.Identity(4)
        result = {**preset, **resolved, "glb_mesh_parts": len(parts), "fbx_mesh_parts": 1,
                  "glb_mount_nodes": ["Mount_" + key for key in resolved["mounts_m"]],
                  **stats(combined, "assembly_bounds_m")}
        result["exports"] = {"glb": glb_path, "fbx": export_fbx(name, combined)}
        return result

    def ensure_preset_scene(preset):
        name = preset["scene"]
        scene = bpy.data.scenes.get(name)
        if scene is not None:
            if scene.get("asset_library") != manifest["library"]:
                raise RuntimeError("Refusing to change an unrelated existing preset scene: " + name)
            for obj in list(scene.objects):
                if obj.instance_type == "COLLECTION" and obj.instance_collection in collections.values():
                    if len(obj.users_scene) > 1:
                        raise RuntimeError("Preset instance is shared with another scene: " + obj.name)
                    bpy.data.objects.remove(obj, do_unlink=True)
        else:
            scene = bpy.data.scenes.new(name)
            scene.unit_settings.system = "METRIC"
            scene.unit_settings.scale_length = 1
            scene.world = assembly.world
            for key in ("engine", "resolution_x", "resolution_y", "resolution_percentage", "film_transparent"):
                setattr(scene.render, key, getattr(assembly.render, key))
            for key in ("view_transform", "look", "exposure", "gamma"):
                setattr(scene.view_settings, key, getattr(assembly.view_settings, key))
            if hasattr(scene, "cycles") and hasattr(assembly, "cycles"):
                scene.cycles.samples = assembly.cycles.samples
                scene.cycles.use_denoising = assembly.cycles.use_denoising
            for old in assembly.objects:
                if old.type not in {"CAMERA", "LIGHT"}:
                    continue
                obj = old.copy()
                obj.data = old.data.copy()
                obj.name = name + "_" + old.type + "_" + old.name
                scene.collection.objects.link(obj)
                obj.matrix_world = old.matrix_world.copy()
                if old == assembly.camera:
                    scene.camera = obj
                    minimum, maximum = preset["assembly_bounds_m"]["min"], preset["assembly_bounds_m"]["max"]
                    target = Vector(tuple((minimum[k] + maximum[k]) / 2 for k in range(3)))
                    obj.data.ortho_scale = max(obj.data.ortho_scale, preset["geometry_dimensions_m"][0] * 1.16)
                    obj.rotation_euler = (target - obj.location).to_track_quat("-Z", "Y").to_euler()
        for asset_name in preset["parts"]:
            obj = bpy.data.objects.new(asset_name + "_" + name + "_Instance", None)
            scene.collection.objects.link(obj)
            obj.instance_type = "COLLECTION"
            obj.instance_collection = collections[asset_name]
            obj.location = Vector(preset["part_mounts_m"][asset_name])
            obj["source_asset"] = asset_name
        scene["assembly_parts"] = json.dumps(preset["parts"])
        scene["asset_library"] = manifest["library"]
        scene["barrel_asset"] = preset["barrel_asset"]
        return scene

    try:
        # The Edit scene excludes alternate asset definitions by default.
        # Evaluate after temporarily enabling them; operator mutations begin
        # only after every source object has been copied into the temp scene.
        bpy.context.window.scene = source
        bpy.context.window.view_layer = source_layer
        for layer in walk_layers(source_layer.layer_collection):
            source_excludes.append((layer, layer.exclude))
            layer.exclude = False
        source_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        depsgraph.update()
        raw_assets = {}
        for row in rows:
            name = row["name"]
            offset = Matrix.Translation(-Vector(row["mount_position_m"]))
            copies = []
            for original in collections[name].all_objects:
                if original.type not in {"MESH", "FONT"}:
                    continue
                evaluated = original.evaluated_get(depsgraph)
                data = remember_mesh(bpy.data.meshes.new_from_object(
                    evaluated, preserve_all_data_layers=True, depsgraph=depsgraph))
                if not data.vertices:
                    raise RuntimeError(f"Empty evaluated source piece: {original.name}")
                copy = bpy.data.objects.new("M7_EXPORT_COPY_" + original.name, data)
                temp_collection.objects.link(copy)
                copy.matrix_world = offset @ original.matrix_world.copy()
                copies.append(copy)
            raw_assets[name] = copies
        bpy.context.window.scene = temp
        bpy.context.window.view_layer = temp.view_layers[0]
        # Apply all cloned world transforms once, including rotated text and
        # cylinders, so each joined asset object ends at local identity.
        all_copies = [obj for copies in raw_assets.values() for obj in copies]
        select_only(all_copies)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        for row in rows:
            name = row["name"]
            for obj in raw_assets[name]:
                fill_missing_uv(obj)
            mesh = join_derived(raw_assets[name], name)
            root = new_root(name)
            root["source_asset"] = name
            root["mount_key"] = row["mount_key"]
            root["mount_position_m"] = row["mount_position_m"]
            mesh.parent = root
            mesh.matrix_world = Matrix.Identity(4)
            mesh["source_asset"] = name
            mesh["mount_key"] = row["mount_key"]
            local_assets[name], local_roots[name] = mesh, root
            row.update(stats(mesh))
            sockets = {}
            if row["category"] == "Core":
                sockets = {key: [value[k] - row["mount_position_m"][k] for k in range(3)]
                           for key, value in fixed_mounts(manifest).items()}
            elif row["mount_key"] == "Barrel":
                sockets = dict(row["local_mounts_m"])
            row["glb_mount_nodes"] = ["Mount_" + key for key in sockets]
            row["glb_mount_positions_m"] = sockets
            row["exports"] = {"glb": export_glb(name, root, [mesh], sockets),
                              "fbx": export_fbx(name, mesh)}
        manifest["presets"] = [export_preset(preset) for preset in preset_definitions]
        preset_scenes = [ensure_preset_scene(preset) for preset in manifest["presets"]]
        manifest["export_origin"] = "Individual assets: authored local mount; presets: selected barrel resolves dependent attachment origins."
        manifest["export_uv_policy"] = "Preserved evaluated source UVs; unwrap only individual copies lacking UVs."
        manifest["fbx_axis_note"] = "+X forward / +Z up; use_space_transform=True; metres with unit scaling enabled."
        manifest["textures"] = "Packed PNG ORM and normal images embedded in GLB and FBX COPY exports."
        manifest["saved_scenes"] = list(dict.fromkeys([source.name, catalog.name] + [scene.name for scene in preset_scenes]))
        manifest["expected_asset_count"] = len(rows)
        export_ok = True
    finally:
        # Restore edit visibility before serializing the library.  Unrelated
        # layer exclusions keep their original values; asset rows show defaults.
        for layer, old_exclude in source_excludes:
            layer.exclude = old_exclude
        for row in rows:
            layer = source_layer.layer_collection.children.get(row["collection"])
            if layer is not None:
                layer.exclude = not row["default"]
        if bpy.context.mode != "OBJECT" and bpy.context.window.scene == temp:
            bpy.ops.object.mode_set(mode="OBJECT")
        bpy.context.window.scene = assembly
        for obj in list(temp.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.scenes.remove(temp)
        if temp_collection.users == 0:
            bpy.data.collections.remove(temp_collection)
        # Purge only meshes created by this exporter; never touch old orphan IDs.
        for data in list(bpy.data.meshes):
            if data.as_pointer() in derived_mesh_pointers and data.users == 0:
                bpy.data.meshes.remove(data)

    if not export_ok:
        raise RuntimeError("M7 export did not complete; editable definitions were preserved.")
    source_layer.update()
    library_ids = {source, catalog, *preset_scenes} | set(collections.values())
    bpy.data.libraries.write(str(source_path),
                             library_ids, fake_user=True, compress=True)
    # Commit manifest only after all exports and isolated source save succeed.
    pending = MANIFEST_PATH.with_name(MANIFEST_PATH.name + ".tmp")
    pending.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    pending.replace(MANIFEST_PATH)
    print(json.dumps({
        "exported_assets": len(rows), "presets": [preset["name"] for preset in manifest["presets"]],
        "source_file": str(source_path),
        "source_scenes": manifest["saved_scenes"],
        "source_geometry_modified": False,
    }, ensure_ascii=False))


main()
