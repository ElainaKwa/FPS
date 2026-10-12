"""Read-only background validation of the M7 asset library and its exports.

Run with Blender, for example::

    blender --background --factory-startup --python Source/verify_library.py

An optional ``-- --root PATH`` overrides the asset directory. The script opens
the saved library, temporarily expands its edit view, and imports each export
into its own empty scene. It never saves a Blender file. Validation.json is the
only written artifact; the saved source SHA256 must remain unchanged.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import struct
import sys
import traceback
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector


TOLERANCE_M = 5e-5
EXPECTED_ASSET_COUNT = 15
EXPECTED_DEFAULT_COUNT = 6
EXPECTED_SUPPRESSED_COUNT = 7
MOUNT_KEYS = ("Stock", "RearGrip", "Magazine", "Optic", "Foregrip", "Muzzle", "Iron")
EXPECTED_SCENES = (
    "M7_Library_Edit",
    "M7_Default_Assembly",
    "M7_Attachment_Catalog",
    "M7_Suppressed_Assembly",
)
GEOMETRY_TYPES = {"MESH", "FONT", "CURVE", "SURFACE"}
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SUFFIX_RE = re.compile(r"(?:\.\d{3})+$")


def parse_arguments():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    return parser.parse_args(args)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def material_name(name):
    """Discard only suffixes Blender adds while importing duplicate datablocks."""
    return SUFFIX_RE.sub("", name)


def compare_vector(actual, expected, tolerance=TOLERANCE_M):
    if actual is None or expected is None or len(actual) != 3 or len(expected) != 3:
        return {"pass": False, "absolute_error_m": None}
    errors = [abs(float(a) - float(b)) for a, b in zip(actual, expected)]
    return {"pass": all(math.isfinite(e) and e <= tolerance for e in errors),
            "absolute_error_m": errors}


def translated_bounds(bounds, translation):
    return {key: [value[i] + translation[i] for i in range(3)]
            for key, value in bounds.items() if key in ("min", "max")}


def union_bounds(bounds):
    return {"min": [min(item["min"][axis] for item in bounds) for axis in range(3)],
            "max": [max(item["max"][axis] for item in bounds) for axis in range(3)]}


def dimensions(bounds):
    return [bounds["max"][axis] - bounds["min"][axis] for axis in range(3)]


def expand_layer_collections(layer_collection):
    layer_collection.exclude = False
    for child in layer_collection.children:
        expand_layer_collections(child)


def mesh_statistics(scene, objects, source=False):
    """Use evaluated vertices, not bounding-box corners or unevaluated topology."""
    lower = [math.inf] * 3
    upper = [-math.inf] * 3
    totals = {"geometry_objects": 0, "mesh_objects": 0, "vertices": 0,
              "triangles": 0, "nonfinite_vertices": 0, "uv_layers": 0,
              "uv_coordinates": 0, "nonfinite_uv_coordinates": 0,
              "missing_uv_objects": 0}
    missing_uv_examples = []
    used_materials = set()
    used_material_ids = set()
    with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
        depsgraph = bpy.context.evaluated_depsgraph_get()
        for original in objects:
            if original.type not in GEOMETRY_TYPES:
                continue
            evaluated = original.evaluated_get(depsgraph)
            evaluated_mesh = None
            try:
                evaluated_mesh = evaluated.to_mesh(
                    preserve_all_data_layers=True, depsgraph=depsgraph)
                if evaluated_mesh is None:
                    raise RuntimeError("Cannot evaluate geometry: " + original.name)
                totals["geometry_objects"] += 1
                totals["mesh_objects"] += int(original.type == "MESH")
                totals["vertices"] += len(evaluated_mesh.vertices)
                evaluated_mesh.calc_loop_triangles()
                totals["triangles"] += len(evaluated_mesh.loop_triangles)
                world = evaluated.matrix_world
                for vertex in evaluated_mesh.vertices:
                    point = world @ vertex.co
                    if not all(math.isfinite(value) for value in point):
                        totals["nonfinite_vertices"] += 1
                        continue
                    for axis, value in enumerate(point):
                        lower[axis] = min(lower[axis], value)
                        upper[axis] = max(upper[axis], value)
                for triangle in evaluated_mesh.loop_triangles:
                    if triangle.material_index < len(evaluated_mesh.materials):
                        material = evaluated_mesh.materials[triangle.material_index]
                        if material is not None:
                            used_materials.add(material_name(material.name))
                            used_material_ids.add(material)
                layers = evaluated_mesh.uv_layers
                totals["uv_layers"] += len(layers)
                # Source FONT objects are flat-material lettering and may have
                # no UVs; their converted exported faces share the joined UV map.
                uv_required = not source or original.type == "MESH"
                if uv_required and evaluated_mesh.polygons and not layers:
                    totals["missing_uv_objects"] += 1
                    if len(missing_uv_examples) < 6:
                        missing_uv_examples.append(original.name)
                for layer in layers:
                    for coordinate in layer.data:
                        totals["uv_coordinates"] += 1
                        if not all(math.isfinite(value) for value in coordinate.uv):
                            totals["nonfinite_uv_coordinates"] += 1
            finally:
                if evaluated_mesh is not None:
                    evaluated.to_mesh_clear()
    valid_bounds = all(math.isfinite(v) for v in lower + upper)
    bounds = {"min": lower, "max": upper} if valid_bounds else None
    result = dict(totals, bounds_m=bounds,
                  dimensions_m=dimensions(bounds) if bounds is not None else None,
                  used_materials=sorted(used_materials),
                  uv_finite=totals["nonfinite_uv_coordinates"] == 0,
                  uv_present=totals["missing_uv_objects"] == 0,
                  missing_uv_examples=missing_uv_examples)
    return result, used_material_ids


def packed_texture_statistics(materials):
    images = set()
    visited_trees = set()

    def visit(tree):
        if tree is None or tree.as_pointer() in visited_trees:
            return
        visited_trees.add(tree.as_pointer())
        for node in tree.nodes:
            if node.type == "TEX_IMAGE" and node.image is not None:
                images.add(node.image)
            if node.type == "GROUP":
                visit(node.node_tree)

    for material in materials:
        visit(material.node_tree if material.use_nodes else None)
    rows = []
    for image in sorted(images, key=lambda item: item.name):
        files = list(image.packed_files)
        packed = [entry.packed_file for entry in files]
        if not packed and image.packed_file is not None:
            packed = [image.packed_file]
        payloads = [bytes(item.data) for item in packed]
        name = material_name(image.name)
        rows.append({"name": name, "size": list(image.size),
                     "file_format": image.file_format,
                     "colorspace": image.colorspace_settings.name,
                     "packed_files": len(payloads),
                     "packed_bytes": sum(len(data) for data in payloads),
                     "packed_png": bool(payloads) and all(
                         data.startswith(PNG_SIGNATURE) for data in payloads),
                     "role": "ORM" if "_ORM" in name else (
                         "Normal" if "_Normal" in name else "Other")})
    return rows


def glb_root_statistics(path):
    """Read origins and sockets from GLB, including parent transforms and Y-up."""
    with path.open("rb") as handle:
        header = handle.read(12)
        if len(header) != 12:
            raise ValueError("GLB header is truncated")
        magic, version, declared_length = struct.unpack("<4sII", header)
        if magic != b"glTF" or version != 2 or declared_length != path.stat().st_size:
            raise ValueError("Invalid GLB 2.0 header or declared length")
        data = None
        while handle.tell() < declared_length:
            chunk_header = handle.read(8)
            if len(chunk_header) != 8:
                raise ValueError("GLB chunk header is truncated")
            size, kind = struct.unpack("<I4s", chunk_header)
            chunk = handle.read(size)
            if len(chunk) != size:
                raise ValueError("GLB chunk is truncated")
            if kind == b"JSON":
                data = json.loads(chunk.rstrip(b" \x00").decode("utf-8"))
        if data is None:
            raise ValueError("GLB has no JSON scene chunk")
    nodes = data.get("nodes", [])
    scene_index = data.get("scene", 0)
    scenes = data.get("scenes", [])
    if not scenes or scene_index >= len(scenes):
        raise ValueError("GLB has no valid active scene")
    roots = []
    root_indices = scenes[scene_index].get("nodes", [])
    for index in root_indices:
        node = nodes[index]
        translation = node.get("translation", [0.0, 0.0, 0.0])
        if "matrix" in node:
            translation = node["matrix"][12:15]
        roots.append({"name": node.get("name", str(index)),
                      "translation_m": translation,
                      "origin_at_zero": compare_vector(translation, [0, 0, 0])["pass"]})

    def local_matrix(node):
        if "matrix" in node:
            raw = node["matrix"]
            if len(raw) != 16:
                raise ValueError("GLB node matrix must have sixteen components")
            # glTF matrices are column-major; mathutils.Matrix uses rows.
            return Matrix([[raw[column * 4 + row] for column in range(4)] for row in range(4)])
        rotation = node.get("rotation", [0, 0, 0, 1])
        scale = node.get("scale", [1, 1, 1])
        return (Matrix.Translation(Vector(node.get("translation", [0, 0, 0]))) @
                Quaternion((rotation[3], rotation[0], rotation[1], rotation[2])).to_matrix().to_4x4() @
                Matrix.Diagonal(Vector(tuple(scale) + (1,))))

    world_matrices = {}

    def visit(index, parent_matrix, ancestors):
        if index in ancestors:
            raise ValueError("GLB scene hierarchy contains a cycle")
        if index in world_matrices:
            raise ValueError("GLB active scene references one node from multiple parents")
        node = nodes[index]
        world_matrices[index] = parent_matrix @ local_matrix(node)
        for child in node.get("children", []):
            visit(child, world_matrices[index], ancestors | {index})

    for index in root_indices:
        visit(index, Matrix.Identity(4), set())
    mounts = []
    for index, node in enumerate(nodes):
        name = node.get("name", "")
        if not name.startswith("Mount_"):
            continue
        matrix = world_matrices.get(index)
        position = matrix.to_translation() if matrix is not None else None
        # Blender's export_yup converts (X,Y,Z) to glTF (X,Z,-Y).
        blender_position = [position.x, -position.z, position.y] if position is not None else None
        mounts.append({"name": name, "in_active_scene": matrix is not None,
                       "position_m": blender_position,
                       "gltf_position_m": list(position) if position is not None else None})
    return {"roots": roots, "root_origin_at_zero": bool(roots) and all(
                row["origin_at_zero"] for row in roots),
            "mesh_definitions": len(data.get("meshes", [])), "mount_nodes": mounts}


def export_path(root, asset, kind):
    provided = asset.get("exports", {}).get(kind) or asset.get("export" + kind.upper())
    path = Path(provided) if isinstance(provided, str) and provided else (
        Path("Exports") / (asset["name"] + "." + kind))
    if not path.is_absolute():
        path = root / path
    path = path.resolve()
    if not path.is_relative_to(root):
        raise ValueError("Export path is outside the library directory: " + str(path))
    return path


def snapshot_datablocks():
    names = ("objects", "meshes", "curves", "materials", "images", "actions",
             "cameras", "lights", "collections", "scenes")
    return {name: {item.as_pointer() for item in getattr(bpy.data, name)} for name in names}


def discard_import(snapshot):
    # Remove only datablocks created by this individual import. Source IDs stay
    # alive even when their users are temporarily zero during scene switching.
    order = ("scenes", "objects", "collections", "meshes", "curves", "materials",
             "images", "actions", "cameras", "lights")
    for name in order:
        container = getattr(bpy.data, name)
        for item in list(container):
            if item.as_pointer() not in snapshot[name]:
                container.remove(item, do_unlink=True)


def import_statistics(path, kind):
    if not path.is_file():
        raise FileNotFoundError(str(path))
    snapshot = snapshot_datablocks()
    original_scene = bpy.context.window.scene if bpy.context.window else None
    scene = bpy.data.scenes.new("M7_Validation_Import_" + kind)
    try:
        if bpy.context.window:
            bpy.context.window.scene = scene
        with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
            if kind == "fbx":
                bpy.ops.import_scene.fbx(filepath=str(path), use_anim=False)
            else:
                bpy.ops.import_scene.gltf(filepath=str(path))
        scene.view_layers[0].update()
        stats, _ = mesh_statistics(scene, list(scene.objects))
        stats["file"] = str(path)
        stats["file_bytes"] = path.stat().st_size
        if kind == "glb":
            stats["scene_graph"] = glb_root_statistics(path)
        return stats
    finally:
        if bpy.context.window and original_scene is not None:
            bpy.context.window.scene = original_scene
        discard_import(snapshot)


class Validator:
    def __init__(self, root):
        self.root = root
        self.report = {"schema_version": 1, "success": False,
                       "blender_version": bpy.app.version_string,
                       "tolerance_m": TOLERANCE_M,
                       "source_sha256_before": None, "source_sha256_after": None,
                       "source_unchanged": False, "library": {}, "assets": [],
                       "default_assembled": {}, "suppressed_assembled": {}, "failures": []}

    def check(self, condition, target, code, message):
        if not condition:
            self.report["failures"].append(
                {"target": target, "code": code, "message": message})
        return bool(condition)

    def check_geometry(self, stats, target):
        self.check(stats["triangles"] > 0 and stats["bounds_m"] is not None,
                   target, "empty_geometry", "Geometry must contain evaluated triangles.")
        self.check(stats["nonfinite_vertices"] == 0, target, "nonfinite_vertices",
                   "Evaluated world coordinates must be finite.")
        self.check(stats["uv_finite"], target, "nonfinite_uv",
                   "All available evaluated UV coordinates must be finite.")
        self.check(stats["uv_present"], target, "missing_uv",
                   "Mesh geometry must contain a UV map; source FONT lettering is exempt.")

    def compare_export(self, actual, expected, expected_meshes, target, root_zero=False):
        self.check_geometry(actual, target)
        comparisons = {key: compare_vector(actual["bounds_m"][key], expected["bounds_m"][key])
                       for key in ("min", "max")} if (
                           actual["bounds_m"] is not None and expected["bounds_m"] is not None) else {}
        comparisons["dimensions"] = compare_vector(actual["dimensions_m"], expected["dimensions_m"])
        actual["comparison"] = comparisons
        self.check(bool(comparisons) and all(row["pass"] for row in comparisons.values()),
                   target, "bounds_mismatch", "XYZ dimensions and min/max must match within 5e-5 m.")
        self.check(actual["triangles"] == expected["triangles"], target, "triangle_mismatch",
                   "Triangle count differs: expected %s, imported %s." % (
                       expected["triangles"], actual["triangles"]))
        self.check(actual["used_materials"] == expected["used_materials"],
                   target, "material_mismatch", "Used material names differ after removing Blender numeric suffixes.")
        if expected_meshes is not None:
            self.check(actual["mesh_objects"] == expected_meshes,
                       target, "mesh_component_count", "Expected %s mesh components, imported %s." % (
                           expected_meshes, actual["mesh_objects"]))
        if root_zero:
            self.check(actual.get("scene_graph", {}).get("root_origin_at_zero", False),
                       target, "mount_origin", "GLB root translation must be zero for the local mount origin.")

    def check_glb_mounts(self, stats, mounts, target, offset=(0, 0, 0)):
        expected_names = ["Mount_" + key for key in MOUNT_KEYS]
        rows = stats.get("scene_graph", {}).get("mount_nodes", [])
        self.check(sorted(row["name"] for row in rows) == sorted(expected_names),
                   target, "mount_names", "GLB must contain exactly the seven declared Mount_* node names.")
        comparisons = []
        for key in MOUNT_KEYS:
            name = "Mount_" + key
            matches = [row for row in rows if row["name"] == name]
            expected = [mounts[key][axis] - offset[axis] for axis in range(3)] if key in mounts else None
            if len(matches) != 1:
                comparisons.append({"name": name, "pass": False, "expected_position_m": expected})
                continue
            actual = matches[0]
            result = compare_vector(actual["position_m"], expected)
            result.update({"name": name, "expected_position_m": expected,
                           "actual_position_m": actual["position_m"],
                           "in_active_scene": actual["in_active_scene"]})
            comparisons.append(result)
            self.check(actual["in_active_scene"] and result["pass"], target,
                       "mount_position", name + " must exist in the active GLB scene at its declared XYZ position within 5e-5 m.")
        stats["mount_comparison"] = comparisons

    def validate_preset(self, preset, source_stats, expected_count, report_key, mounts):
        name = preset["name"]
        parts = preset["parts"]
        row = self.report[report_key]
        row["name"] = name
        row["parts"] = parts
        self.check(len(parts) == expected_count and len(parts) == len(set(parts)),
                   name, "preset_component_count", "Preset must have %s unique components." % expected_count)
        self.check(preset.get("glb_mesh_parts") == expected_count and preset.get("fbx_mesh_parts") == 1,
                   name, "preset_mesh_manifest", "Preset mesh counts must declare %s GLB parts and one FBX mesh." % expected_count)
        self.check(sorted(preset.get("glb_mount_nodes", [])) == sorted("Mount_" + key for key in MOUNT_KEYS),
                   name, "preset_mount_manifest", "Preset must declare all seven Mount_* sockets.")
        if not parts or not all(part in source_stats and source_stats[part]["bounds_m"] is not None for part in parts):
            self.check(False, name, "missing_source_component",
                       "Cannot validate preset because a source component is missing or empty.")
            return
        part_stats = [source_stats[part] for part in parts]
        bounds = union_bounds([stats["bounds_m"] for stats in part_stats])
        expected = {"bounds_m": bounds, "dimensions_m": dimensions(bounds),
                    "triangles": sum(stats["triangles"] for stats in part_stats),
                    "used_materials": sorted({material for stats in part_stats for material in stats["used_materials"]})}
        row["expected"] = expected
        self.check(expected["triangles"] == preset.get("triangles"), name,
                   "preset_manifest_triangles", "Sum of evaluated source triangles differs from preset manifest.")
        manifest_bounds = preset.get("assembly_bounds_m", {})
        self.check(all(compare_vector(bounds[key], manifest_bounds.get(key))["pass"] for key in ("min", "max")),
                   name, "preset_manifest_bounds", "Union of source world bounds differs from preset manifest.")
        for kind in ("fbx", "glb"):
            target = name + ":" + kind
            try:
                imported = import_statistics(export_path(self.root, preset, kind), kind)
                row[kind] = imported
                self.compare_export(imported, expected, expected_count if kind == "glb" else 1,
                                    target, root_zero=kind == "glb")
                if kind == "glb":
                    self.check_glb_mounts(imported, mounts, target)
            except Exception as exc:
                row[kind] = {"error": str(exc)}
                self.check(False, target, "import_failed", str(exc))

    def check_instances(self, scene, expected_names, assets_by_collection, catalog=False):
        instances = [obj for obj in scene.objects if obj.instance_type == "COLLECTION"
                     and obj.instance_collection is not None]
        names = [obj.instance_collection.name for obj in instances]
        self.check(sorted(names) == sorted(expected_names), scene.name,
                   "collection_instances", "Collection instances must match the manifest components exactly.")
        rows = []
        for obj in instances:
            name = obj.instance_collection.name
            row = {"collection": name, "position_m": list(obj.location)}
            if not catalog and name in assets_by_collection:
                expected = assets_by_collection[name]["mount_position_m"]
                row["mount_comparison"] = compare_vector(list(obj.location), expected)
                self.check(row["mount_comparison"]["pass"], scene.name,
                           "assembly_mount", "Default instance must use its declared mount: " + name)
            rows.append(row)
        return rows

    def run(self):
        if not bpy.app.background:
            raise RuntimeError("This verifier only runs in background Blender; live sessions are refused.")
        manifest_path = self.root / "AssetManifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        source_path = (self.root / manifest.get("source_file", "M7_Modular_Replica.blend")).resolve()
        if not source_path.is_relative_to(self.root):
            raise ValueError("Source file is outside the library directory")
        self.source_path = source_path
        self.report["source_file"] = str(source_path)
        self.report["source_sha256_before"] = sha256(source_path)
        bpy.ops.wm.open_mainfile(filepath=str(source_path), load_ui=False)
        assets = manifest["assets"]
        mounts = manifest.get("mounts", {})
        self.check(set(mounts) - {"Core"} == set(MOUNT_KEYS), "library", "mount_manifest",
                   "Manifest must define the seven attachment mount positions.")
        preset_rows = manifest.get("presets", [])
        preset_names = [row["name"] for row in preset_rows]
        self.check(len(preset_names) == len(set(preset_names)), "library", "duplicate_preset_name",
                   "Preset names must be unique.")
        presets = {row["name"]: row for row in preset_rows}
        for name in ("SM_M7_Default_Assembled", "SM_M7_Suppressed_Assembled"):
            self.check(name in presets, "library", "missing_preset", "Missing required exported preset: " + name)
        self.check(len(assets) == EXPECTED_ASSET_COUNT, "library", "asset_count",
                   "Expected one core and fourteen attachments (15 assets).")
        self.check(sum(asset["category"] == "Core" for asset in assets) == 1,
                   "library", "core_count", "The library must define exactly one core.")
        asset_names = [asset["name"] for asset in assets]
        self.check(len(asset_names) == len(set(asset_names)), "library", "duplicate_asset_name",
                   "Asset names must be unique.")
        defaults = manifest["default_parts"]
        self.check(len(defaults) == EXPECTED_DEFAULT_COUNT, "library", "default_count",
                   "Default assembly must contain six separately replaceable mesh components.")
        self.check(set(defaults) == {asset["name"] for asset in assets if asset["default"]},
                   "library", "default_manifest", "default_parts and per-asset default flags must agree.")
        if "SM_M7_Default_Assembled" in presets:
            self.check(sorted(presets["SM_M7_Default_Assembled"]["parts"]) == sorted(defaults),
                       "library", "default_preset_parts", "Default preset parts must match default_parts.")
        scenes = {name: bpy.data.scenes.get(name) for name in EXPECTED_SCENES}
        for name, scene in scenes.items():
            self.check(scene is not None, "library", "missing_scene", "Missing required scene: " + name)
        obsolete = {"scenes": [scene.name for scene in bpy.data.scenes
                                if "TIDELINE" in scene.name.upper() or "M7_BLACK" in scene.name.upper()],
                    "objects": [obj.name for obj in bpy.data.objects
                                if "TIDELINE" in obj.name.upper() or "M7_BLACK" in obj.name.upper()]}
        self.check(not any(obsolete.values()), "library", "obsolete_asset_data",
                   "Saved source contains an object or scene from TIDELINE or M7_BLACK.")
        self.report["library"]["obsolete_data"] = obsolete
        edit = scenes["M7_Library_Edit"]
        if edit is None:
            return
        expand_layer_collections(edit.view_layers[0].layer_collection)
        edit.view_layers[0].update()
        by_collection = {asset["collection"]: asset for asset in assets}
        by_name = {asset["name"]: asset for asset in assets}
        if scenes["M7_Default_Assembly"] is not None:
            expected = [by_name[name]["collection"] for name in defaults if name in by_name]
            self.report["library"]["default_instances"] = self.check_instances(
                scenes["M7_Default_Assembly"], expected, by_collection)
        if scenes["M7_Attachment_Catalog"] is not None:
            expected = [asset["collection"] for asset in assets if asset["category"] != "Core"]
            self.report["library"]["catalog_instances"] = self.check_instances(
                scenes["M7_Attachment_Catalog"], expected, by_collection, catalog=True)
        if scenes["M7_Suppressed_Assembly"] is not None and "SM_M7_Suppressed_Assembled" in presets:
            parts = presets["SM_M7_Suppressed_Assembled"]["parts"]
            expected = [by_name[name]["collection"] for name in parts if name in by_name]
            self.report["library"]["suppressed_instances"] = self.check_instances(
                scenes["M7_Suppressed_Assembly"], expected, by_collection)
        source_stats = {}
        source_materials = set()
        for asset in assets:
            name = asset["name"]
            row = {"name": name, "category": asset["category"], "source": None}
            self.report["assets"].append(row)
            try:
                collection = bpy.data.collections.get(asset["collection"])
                if collection is None:
                    raise ValueError("Missing source collection: " + asset["collection"])
                row["asset_marked"] = collection.asset_data is not None
                row["editable_objects"] = len(collection.objects)
                row["collection_offset_m"] = list(collection.instance_offset)
                self.check(row["asset_marked"], name, "asset_mark", "Collection must be marked as a Blender asset.")
                self.check(row["editable_objects"] == asset["editable_objects"], name,
                           "editable_object_count", "Editable object count differs from the manifest.")
                self.check(compare_vector(row["collection_offset_m"], asset["mount_position_m"])["pass"],
                           name, "collection_mount_offset", "Collection instance_offset must equal its declared mount.")
                stats, materials = mesh_statistics(edit, list(collection.all_objects), source=True)
                row["source"] = stats
                source_materials.update(materials)
                source_stats[name] = stats
                self.check_geometry(stats, name + ":source")
                if "triangles" in asset:
                    self.check(stats["triangles"] == asset["triangles"], name,
                               "manifest_triangles", "Evaluated triangle count differs from manifest.triangles.")
                declared_materials = asset.get("used_materials", asset.get("materials"))
                if isinstance(declared_materials, list) and all(
                        isinstance(value, str) for value in declared_materials):
                    self.check(stats["used_materials"] == sorted({material_name(value) for value in declared_materials}),
                               name, "manifest_materials", "Used materials differ from manifest used_materials.")
                if stats["bounds_m"] is None:
                    continue
                local_bounds = translated_bounds(stats["bounds_m"], [-value for value in asset["mount_position_m"]])
                expected = dict(stats, bounds_m=local_bounds, dimensions_m=dimensions(local_bounds))
                row["expected_export_bounds_m"] = local_bounds
                for kind in ("fbx", "glb"):
                    target = name + ":" + kind
                    try:
                        imported = import_statistics(export_path(self.root, asset, kind), kind)
                        row[kind] = imported
                        self.compare_export(imported, expected, 1 if kind == "glb" else None,
                                            target, root_zero=kind == "glb")
                        if kind == "glb" and asset["category"] == "Core":
                            self.check(sorted(asset.get("glb_mount_nodes", [])) == sorted("Mount_" + key for key in MOUNT_KEYS),
                                       target, "core_mount_manifest", "Core must declare all seven Mount_* sockets.")
                            self.check_glb_mounts(imported, mounts, target, offset=asset["mount_position_m"])
                    except Exception as exc:
                        row[kind] = {"error": str(exc)}
                        self.check(False, target, "import_failed", str(exc))
            except Exception as exc:
                row["error"] = str(exc)
                self.check(False, name, "source_failed", str(exc))
        textures = packed_texture_statistics(source_materials)
        self.report["library"]["packed_textures"] = textures
        for role in ("ORM", "Normal"):
            role_rows = [row for row in textures if row["role"] == role]
            self.check(bool(role_rows), "library", "missing_" + role.lower(),
                       "Source materials must reference packed PNG " + role + " maps.")
            for texture in role_rows:
                self.check(texture["packed_bytes"] > 0 and texture["packed_png"] and
                           all(value > 0 for value in texture["size"]), texture["name"],
                           "packed_texture", "Image must have nonempty packed PNG data and valid dimensions.")
        for name, count, report_key in (
                ("SM_M7_Default_Assembled", EXPECTED_DEFAULT_COUNT, "default_assembled"),
                ("SM_M7_Suppressed_Assembled", EXPECTED_SUPPRESSED_COUNT, "suppressed_assembled")):
            if name in presets:
                self.validate_preset(presets[name], source_stats, count, report_key, mounts)

    def finish(self):
        if hasattr(self, "source_path") and self.source_path.is_file():
            self.report["source_sha256_after"] = sha256(self.source_path)
            self.report["source_unchanged"] = (
                self.report["source_sha256_before"] == self.report["source_sha256_after"])
            self.check(self.report["source_unchanged"], "library", "source_modified",
                       "Saved source .blend SHA256 changed during validation.")
        self.report["success"] = not self.report["failures"]
        self.report["summary"] = {"assets_checked": len(self.report["assets"]),
                                  "asset_exports_imported": sum(
                                      int(kind in row and "error" not in row[kind])
                                      for row in self.report["assets"] for kind in ("fbx", "glb")),
                                  "assembly_exports_imported": sum(
                                      int(kind in self.report[preset] and
                                          "error" not in self.report[preset][kind])
                                      for preset in ("default_assembled", "suppressed_assembled")
                                      for kind in ("fbx", "glb")),
                                  "failure_count": len(self.report["failures"]),
                                  "source_unchanged": self.report["source_unchanged"]}
        report_path = self.root / "Validation.json"
        report_path.write_text(json.dumps(self.report, ensure_ascii=False, indent=2, allow_nan=False), encoding="utf-8")
        print(json.dumps({"success": self.report["success"], "report": str(report_path),
                          **self.report["summary"]}, ensure_ascii=False))
        for failure in self.report["failures"][:12]:
            print("FAIL {target} [{code}]: {message}".format(**failure))
        remaining = len(self.report["failures"]) - 12
        if remaining > 0:
            print("%s additional failures are recorded in Validation.json." % remaining)
        return 0 if self.report["success"] else 1


def main():
    options = parse_arguments()
    validator = Validator(options.root.resolve())
    try:
        validator.run()
    except Exception as exc:
        validator.check(False, "library", "validation_exception", str(exc))
        validator.report["exception"] = traceback.format_exc()
    exit_code = validator.finish()
    # Blender's Python-exception exit status varies by CLI configuration. This
    # verifier is a dedicated background process, so flush the compact summary
    # and terminate that process with the explicit required status. Never use
    # os._exit in a live session, even if the background preflight was refused.
    if bpy.app.background:
        sys.stdout.flush()
        sys.stderr.flush()
        os._exit(exit_code)
    raise SystemExit(exit_code)


if __name__ == "__main__":
    main()
