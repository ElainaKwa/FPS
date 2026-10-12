"""Read-only background validation of M7 material-sample modular geometry, baked PBR maps and exports.

Run with Blender, for example::

    blender --background --factory-startup --python Source/verify_library.py

An optional ``-- --root PATH`` overrides the asset directory. The script opens
the saved library, temporarily expands its edit view, and imports each export
into its own empty scene. It never saves a Blender file. Validation.json is the
only delivery artifact written; decoded PNG cache files live outside the asset
directory and are explicitly removed after this process exits. The saved source
SHA256 must remain unchanged.
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
import uuid

import numpy as np
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector


TOLERANCE_M = 5e-5
DEPENDENT_MOUNT_KEYS = ("Foregrip", "Muzzle", "FrontIron")
DELIVERY_BASELINE_SUFFIXES = {".blend", ".json", ".md", ".txt", ".csv", ".sha256", ".toml"}
DELIVERY_CONFIG_SUFFIXES = {".json", ".toml", ".yaml", ".yml", ".ini", ".cfg", ".md"}


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


GEOMETRY_TYPES = {"MESH", "FONT", "CURVE", "SURFACE"}
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
SUFFIX_RE = re.compile(r"(?:\.\d{3})+$")


def parse_arguments():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--cleanup-image-cache", type=Path,
                        help="Remove one completed run's image cache; never opens or saves the source.")
    return parser.parse_args(args)


def validation_image_cache_base(asset_root):
    asset_root = Path(asset_root).resolve()
    project = next((parent for parent in asset_root.parents if any(parent.glob("*.uproject"))), None)
    base = ((project or asset_root.parent) / "Saved/M7MaterialStudy/ValidationImages").resolve()
    if base.is_relative_to(asset_root):
        raise ValueError("Validation image cache must remain outside the asset directory")
    return base


def validation_process_running(pid):
    if os.name == "nt":
        import ctypes
        from ctypes import wintypes
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        kernel.OpenProcess.restype = wintypes.HANDLE
        kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        kernel.GetExitCodeProcess.restype = wintypes.BOOL
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel.CloseHandle.restype = wintypes.BOOL
        handle = kernel.OpenProcess(0x1000, False, pid)
        if not handle:
            # Access denied is not proof that the process has finished.
            if ctypes.get_last_error() == 87:  # ERROR_INVALID_PARAMETER: PID no longer exists.
                return False
            raise RuntimeError("Cannot prove the cache owner process has exited")
        try:
            code = wintypes.DWORD()
            if not kernel.GetExitCodeProcess(handle, ctypes.byref(code)):
                raise RuntimeError("Cannot read the cache owner process state")
            return code.value == 259  # STILL_ACTIVE
        finally:
            kernel.CloseHandle(handle)
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    return True


def cleanup_validation_image_cache(directory, asset_root):
    base = validation_image_cache_base(asset_root)
    directory = Path(directory).resolve()
    name = re.fullmatch(r"run_(\d+)_[0-9a-f]{32}", directory.name)
    if directory.parent != base or name is None or directory.is_symlink():
        raise ValueError("Refuse to clean a directory outside the exact validation cache scope")
    if validation_process_running(int(name.group(1))):
        raise RuntimeError("Cache owner is still running; clean only after Blender exits")
    if not directory.exists():
        return 0
    files = list(directory.iterdir())
    if any(file.is_symlink() or not file.is_file() or file.resolve().parent != directory or
           re.fullmatch(r"[0-9a-f]{64}\.png", file.name) is None for file in files):
        raise ValueError("Unexpected cache content; refuse a recursive or broad deletion")
    for file in files:
        file.unlink()
    directory.rmdir()
    return len(files)


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
                            # Dependency-graph copies omit source asset metadata.
                            # Keep geometry/material-slot accounting evaluated,
                            # but inspect marks and texture nodes on the actual
                            # source ID when verifying editable definitions.
                            used_material_ids.add(material.original if source and material.is_evaluated else material)
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


def packed_texture_statistics(materials, role_by_filename=None):
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
        filename = Path(image.filepath).name
        role_hint = (role_by_filename or {}).get(filename)
        rows.append({"name": name, "size": list(image.size),
                     "file_format": image.file_format,
                     "colorspace": image.colorspace_settings.name,
                     "packed_files": len(payloads),
                     "packed_bytes": sum(len(data) for data in payloads),
                     "packed_png": bool(payloads) and all(
                         data.startswith(PNG_SIGNATURE) for data in payloads),
                     "role": role_hint or ("ORM" if "_ORM" in name else (
                         "Normal" if "_Normal" in name else (
                         "BaseColor" if "basecolor" in (name + image.filepath).lower() else "Other")))})
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
        parent_index = next((parent for parent, other in enumerate(nodes)
                             if index in other.get("children", [])), None)
        mounts.append({"name": name, "in_active_scene": matrix is not None,
                       "position_m": blender_position,
                       "socket_owner": node.get("extras", {}).get("socket_owner"),
                       "parent_name": nodes[parent_index].get("name") if parent_index is not None else None,
                       "gltf_position_m": list(position) if position is not None else None})
    components = []
    for index, node in enumerate(nodes):
        if "mesh" not in node or index not in world_matrices:
            continue
        position = world_matrices[index].to_translation()
        parent_index = next((parent for parent, other in enumerate(nodes)
                             if index in other.get("children", [])), None)
        components.append({"name": node.get("name", str(index)),
                           "source_asset": node.get("extras", {}).get("source_asset"),
                           "position_m": [position.x, -position.z, position.y],
                           "parent_name": nodes[parent_index].get("name") if parent_index is not None else None})
    return {"roots": roots, "root_origin_at_zero": bool(roots) and all(
                row["origin_at_zero"] for row in roots),
            "mesh_definitions": len(data.get("meshes", [])), "mount_nodes": mounts,
            "component_nodes": components}


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


TEXTURE_ROLES = ("BaseColor", "Normal", "ORM")


def sample_texture_sets(asset):
    """Declared baked material-slot map; constant materials are separate."""
    sets = asset.get("texture_sets", {})
    if not isinstance(sets, dict):
        raise ValueError("texture_sets must map material names to BaseColor/Normal/ORM paths.")
    result = {}
    for name, maps in sets.items():
        normalized = material_name(name)
        if normalized in result:
            raise ValueError("Ambiguous normalized baked material name: " + normalized)
        if not isinstance(maps, dict) or not all(isinstance(maps.get(role), str) and maps[role] for role in TEXTURE_ROLES):
            raise ValueError("Incomplete PBR map declaration: " + name)
        result[normalized] = {role: maps[role] for role in TEXTURE_ROLES}
    return result


def read_glb_payload(path):
    """Return JSON and embedded binary buffer, never resolve external URIs."""
    with path.open("rb") as handle:
        header = handle.read(12)
        if len(header) != 12:
            raise ValueError("Truncated GLB header")
        magic, version, declared = struct.unpack("<4sII", header)
        if magic != b"glTF" or version != 2 or declared != path.stat().st_size:
            raise ValueError("Invalid GLB header or length")
        document, binary = None, None
        while handle.tell() < declared:
            chunk_header = handle.read(8)
            if len(chunk_header) != 8:
                raise ValueError("Truncated GLB chunk")
            length, kind = struct.unpack("<I4s", chunk_header)
            payload = handle.read(length)
            if len(payload) != length:
                raise ValueError("Truncated GLB payload")
            if kind == b"JSON":
                document = json.loads(payload.rstrip(b" \x00"))
            elif kind == b"BIN\x00":
                binary = payload
        if document is None or binary is None:
            raise ValueError("GLB must contain JSON and an embedded binary buffer")
    return document, binary


def png_pixel_signature(path):
    """Decode every pixel using Blender, hash raw 8-bit-equivalent channels.

    Non-Color is used only on this temporary verification image so signatures
    compare encoded PNG channels without source material color-space changes.
    The image datablock is removed immediately; source images stay untouched.
    """
    if not path.read_bytes().startswith(PNG_SIGNATURE):
        raise ValueError("Expected a PNG image: " + str(path))
    image = bpy.data.images.load(str(path), check_existing=False)
    try:
        image.colorspace_settings.name = "Non-Color"
        width, height = image.size
        if width <= 0 or height <= 0:
            raise ValueError("PNG has invalid dimensions: " + str(path))
        pixels = np.empty(width * height * 4, dtype=np.float32)
        image.pixels.foreach_get(pixels)
        if not np.isfinite(pixels).all():
            raise ValueError("PNG has nonfinite decoded pixels: " + str(path))
        rgba = np.rint(np.clip(pixels.reshape((-1, 4)), 0, 1) * 255).astype(np.uint8)
        hashes = {name: hashlib.sha256(rgba[:, index].tobytes()).hexdigest()
                  for index, name in enumerate(("R", "G", "B", "A"))}
        hashes["RGB"] = hashlib.sha256(rgba[:, :3].tobytes()).hexdigest()
        return {"size": [width, height], "channels": hashes}
    finally:
        bpy.data.images.remove(image)


class Validator:
    def __init__(self, root):
        self.root = root
        self.report = {"schema_version": 1, "success": False,
                       "blender_version": bpy.app.version_string,
                       "tolerance_m": TOLERANCE_M,
                       "source_sha256_before": None, "source_sha256_after": None,
                       "source_unchanged": False, "library": {}, "assets": [],
                       "presets": {}, "baked_textures": {}, "validated_files": {}, "failures": []}
        self.pixel_signatures = {}
        self.embedded_signatures = {}
        self.image_cache_directory = None

    def decode_embedded_png(self, payload, key):
        if re.fullmatch(r"[0-9a-f]{64}", key) is None or hashlib.sha256(payload).hexdigest() != key:
            raise ValueError("Validation PNG cache requires the exact payload SHA256")
        if self.image_cache_directory is None:
            base = validation_image_cache_base(self.root)
            base.mkdir(mode=0o777, parents=True, exist_ok=True)
            directory = base / ("run_%s_%s" % (os.getpid(), uuid.uuid4().hex))
            # tempfile.mkdtemp uses 0o700, which produces an inaccessible
            # private ACL under this Windows restricted token. Inherit the
            # already writable Saved directory's ACL instead.
            directory.mkdir(mode=0o777)
            self.image_cache_directory = directory
            self.report["image_decode_cache"] = {
                "directory": str(directory), "owner_pid": os.getpid(),
                "cleanup_after_process_exit": True, "files": []}
        path = self.image_cache_directory / (key + ".png")
        if not path.exists():
            with path.open("xb") as handle:
                handle.write(payload)
            self.report["image_decode_cache"]["files"].append(path.name)
        elif sha256(path) != key:
            raise ValueError("Validation PNG cache payload changed")
        # The file is closed before Blender loads it. png_pixel_signature
        # still decodes every pixel and removes its temporary image ID.
        return png_pixel_signature(path)

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
        expected_names = ["Mount_" + key for key in mounts]
        rows = stats.get("scene_graph", {}).get("mount_nodes", [])
        self.check(sorted(row["name"] for row in rows) == sorted(expected_names),
                   target, "mount_names", "GLB must contain exactly the declared Mount_* node names.")
        comparisons = []
        for key in mounts:
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

    def check_glb_components(self, stats, resolved, assets_by_name, target):
        nodes = stats.get("scene_graph", {}).get("component_nodes", [])
        parts = resolved["part_mounts_m"]
        self.check(sorted(node.get("source_asset") or "" for node in nodes) == sorted(parts),
                   target, "component_nodes", "Each part must have exactly one identifiable GLB mesh node.")
        for name, expected in parts.items():
            matches = [node for node in nodes if node.get("source_asset") == name]
            if len(matches) != 1:
                continue
            node = matches[0]
            self.check(compare_vector(node["position_m"], expected)["pass"],
                       target, "component_origin", name + " must be at its resolved attachment origin.")
            key = assets_by_name[name]["mount_key"]
            if key != "Core":
                self.check(node["parent_name"] == "Mount_" + key, target,
                           "component_parent", name + " must be a child of its matching Mount_* socket.")
        for socket in stats.get("scene_graph", {}).get("mount_nodes", []):
            key = socket["name"].removeprefix("Mount_")
            if key in resolved["barrel_local_mounts_m"]:
                self.check(socket.get("socket_owner") == resolved["barrel_asset"], target,
                           "forward_socket_owner", key + " must belong to the selected Barrel combo.")

    def validate_preset(self, preset, source_stats, assets_by_name):
        name, parts = preset["name"], preset["parts"]
        row = self.report["presets"].setdefault(name, {})
        row["name"], row["parts"] = name, parts
        try:
            resolved = resolve_preset(self.manifest, assets_by_name, preset)
        except Exception as exc:
            self.check(False, name, "preset_schema", str(exc))
            return
        count = len(parts)
        self.check(preset.get("glb_mesh_parts") == count and preset.get("fbx_mesh_parts") == 1,
                   name, "preset_mesh_manifest", "Preset must declare one GLB mesh per part and one FBX mesh.")
        self.check(sorted(preset.get("glb_mount_nodes", [])) == sorted("Mount_" + key for key in resolved["mounts_m"]),
                   name, "preset_mount_manifest", "Preset must declare receiver and selected-Barrel sockets.")
        self.check(preset.get("barrel_asset") == resolved["barrel_asset"], name,
                   "preset_barrel", "Preset barrel identity differs from its parts list.")
        for part, placement in resolved["part_mounts_m"].items():
            self.check(compare_vector(preset.get("part_mounts_m", {}).get(part), placement)["pass"],
                       name, "preset_placement_manifest", part + " has an outdated resolved mount.")
        if not all(part in source_stats and source_stats[part]["bounds_m"] is not None for part in parts):
            self.check(False, name, "missing_source_component", "A source component is missing or empty.")
            return
        positioned_bounds = []
        for part in parts:
            author = assets_by_name[part]["mount_position_m"]
            placed = resolved["part_mounts_m"][part]
            positioned_bounds.append(translated_bounds(source_stats[part]["bounds_m"],
                                                       [placed[k] - author[k] for k in range(3)]))
        bounds = union_bounds(positioned_bounds)
        expected = {"bounds_m": bounds, "dimensions_m": dimensions(bounds),
                    "triangles": sum(source_stats[part]["triangles"] for part in parts),
                    "used_materials": sorted({mat for part in parts for mat in source_stats[part]["used_materials"]})}
        row["expected"], row["resolved"] = expected, resolved
        self.check(expected["triangles"] == preset.get("triangles"), name,
                   "preset_manifest_triangles", "Source triangle sum differs from the preset manifest.")
        declared_bounds = preset.get("assembly_bounds_m", {})
        self.check(all(compare_vector(bounds[key], declared_bounds.get(key))["pass"] for key in ("min", "max")),
                   name, "preset_manifest_bounds", "Resolved source world bounds differ from the preset manifest.")
        for kind in ("fbx", "glb"):
            target = name + ":" + kind
            try:
                path = export_path(self.root, preset, kind)
                self.record_file(path)
                imported = import_statistics(path, kind)
                row[kind] = imported
                self.compare_export(imported, expected, count if kind == "glb" else 1,
                                    target, root_zero=kind == "glb")
                if kind == "glb":
                    self.check_glb_mounts(imported, resolved["mounts_m"], target)
                    self.check_glb_components(imported, resolved, assets_by_name, target)
                    self.check_glb_pbr(export_path(self.root, preset, kind), [assets_by_name[name] for name in parts], target)
            except Exception as exc:
                row[kind] = {"error": str(exc)}
                self.check(False, target, "import_failed", str(exc))

    def check_instances(self, scene, expected_names, assets_by_collection, catalog=False, placements=None):
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
                asset = assets_by_collection[name]
                expected = (placements or {}).get(asset["name"], asset["mount_position_m"])
                row["mount_comparison"] = compare_vector(list(obj.location), expected)
                self.check(row["mount_comparison"]["pass"], scene.name,
                           "assembly_mount", "Instance must use its resolved preset mount: " + name)
            rows.append(row)
        return rows

    def record_file(self, path):
        path = path.resolve()
        if not path.is_relative_to(self.root):
            raise ValueError("Validated file is outside the material sample: " + str(path))
        relative = path.relative_to(self.root).as_posix()
        actual = sha256(path)
        previous = self.report["validated_files"].get(relative)
        self.check(previous is None or previous == actual, relative, "delivery_file_changed",
                   "A delivery file changed during validation; validate the final files again.")
        # Preserve the first observed digest, rather than silently rebinding a
        # file that changed after its geometry, pixels or metadata were read.
        if previous is None:
            self.report["validated_files"][relative] = actual

    def record_delivery_files(self):
        """Bind the same actual input set collected by package_sample.py."""
        files = {self.root / "README.md", self.root / "Scripts/material_settings.json",
                 self.root / "Previews/Compare.html",
                 self.root / self.manifest.get("source_file", "Source/M7_Material_Study.blend")}
        files.update((self.root / "Scripts").glob("*.py"))
        files.update((self.root / "Scripts").glob("*.json"))
        files.update((self.root / "Previews").glob("Preview_*.png"))
        files.update((self.root / "Previews").glob("Before_*.png"))
        files.update((self.root / "Textures/Baked").rglob("*.png"))
        files.update(path for path in self.root.glob("*.json")
                     if path.is_file() and path.name.casefold() != "validation.json")
        files.update(path for path in (self.root / "Config").rglob("*")
                     if path.is_file() and path.suffix.lower() in DELIVERY_CONFIG_SUFFIXES)
        for row in self.manifest["assets"] + self.manifest.get("presets", []):
            files.update(self.root / path for kind, path in row.get("exports", {}).items()
                         if kind in {"fbx", "glb"})
        baseline = set()
        entries = self.manifest.get("baseline_files", [])
        if isinstance(entries, str):
            entries = [entries]
        if not entries and (self.root / "Baseline").is_dir():
            entries = ["Baseline"]
        if not entries:
            entries = [path.relative_to(self.root).as_posix() for path in self.root.glob("*Baseline*.json")]
        for entry in entries:
            path = (self.root / entry).resolve()
            if not path.is_relative_to(self.root):
                raise ValueError("Baseline entry is outside the material sample: " + str(path))
            candidates = [path] if path.is_file() else list(path.rglob("*"))
            baseline.update(file for file in candidates if file.is_file() and
                            file.suffix.lower() in DELIVERY_BASELINE_SUFFIXES)
        self.check(bool(baseline), "baseline", "missing_baseline", "The packaged read-only baseline is missing.")
        files.update(baseline)
        for path in sorted({path.resolve() for path in files}):
            try:
                if "Reference" in path.relative_to(self.root).parts:
                    raise ValueError("Game reference media is excluded from the delivery hash set")
                self.record_file(path)
            except Exception as exc:
                self.check(False, str(path), "delivery_file_unreadable", str(exc))
        # Keep the original baseline fingerprint authoritative when its one
        # .blend snapshot is declared alongside supporting baseline metadata.
        snapshots = [path for path in baseline if path.suffix.lower() == ".blend"]
        expected = self.manifest.get("baseline", {}).get("sha256")
        if expected and len(snapshots) == 1:
            relative = snapshots[0].resolve().relative_to(self.root).as_posix()
            self.check(self.report["validated_files"].get(relative) == expected,
                       "baseline", "baseline_sha256", "Read-only geometry baseline differs from its declared original SHA256.")

    def baked_path(self, relative):
        path = (self.root / relative).resolve()
        baked = (self.root / "Textures/Baked").resolve()
        if not path.is_relative_to(baked) or path.suffix.lower() != ".png":
            raise ValueError("Baked texture must be PNG inside Textures/Baked: " + relative)
        return path

    def expected_signature(self, relative):
        path = self.baked_path(relative)
        key = str(path)
        if key not in self.pixel_signatures:
            self.pixel_signatures[key] = png_pixel_signature(path)
            self.record_file(path)
        return self.pixel_signatures[key]

    def check_texture_declarations(self, assets):
        owners = {}
        for asset in assets:
            name = asset["name"]
            sets = sample_texture_sets(asset)
            self.check(bool(sets), name, "baked_material_map", "Each asset must declare its independent baked texture atlas.")
            for material, maps in sets.items():
                for role, relative in maps.items():
                    path = self.baked_path(relative)
                    old_owner = owners.get(str(path))
                    self.check(old_owner is None or old_owner == name, name,
                               "cross_asset_texture", "Different assets cannot share one baked atlas path: " + relative)
                    owners[str(path)] = name
                    signature = self.expected_signature(relative)
                    self.report["baked_textures"][relative] = {"owner_asset": name, "role": role, **signature}

    def check_source_materials(self, asset, collection, materials):
        target = asset["name"] + ":source"
        sets = sample_texture_sets(asset)
        preserved = {material_name(name) for name in self.manifest.get("preserved_constant_materials", [])}
        preserved.update(material_name(name) for name in asset.get("preserved_constant_materials", []))
        expected_uv = asset.get("baked_uv_layer", self.manifest.get("baked_uv_layer", "M7_MaterialUV"))
        for obj in collection.all_objects:
            if obj.type != "MESH":
                continue
            layer = obj.data.uv_layers.get(expected_uv)
            self.check(layer is not None and layer.active_render, target, "baked_source_uv",
                       "Source MESH must retain the declared active render UV: " + obj.name)
        for mat in materials:
            name = material_name(mat.name)
            self.check(name in sets or name in preserved, target, "undeclared_material_slot",
                       "Every source material must declare baked maps or be an explicit preserved constant: " + name)
            self.check(mat.asset_data is not None, target, "material_asset_mark", "Source materials must retain Blender asset marks: " + name)
            if name in sets and mat.use_nodes:
                declared = {self.baked_path(relative) for relative in sets[name].values()}
                images_seen = set()
                for node in mat.node_tree.nodes:
                    if node.type != "TEX_IMAGE" or node.image is None:
                        continue
                    image_path = Path(bpy.path.abspath(node.image.filepath)).resolve()
                    if image_path not in declared:
                        continue
                    images_seen.add(image_path)
                    for link in node.inputs["Vector"].links:
                        uses_uv = ((link.from_node.type == "UVMAP" and link.from_node.uv_map == expected_uv) or
                                   (link.from_node.type == "TEX_COORD" and link.from_socket.name == "UV"))
                        self.check(uses_uv, target, "baked_texture_vector", name + " must sample the declared UV atlas.")
                self.check(images_seen == declared, target, "source_baked_bindings",
                           "Source shader must reference all declared BaseColor/Normal/ORM images: " + name)

    def check_glb_pbr(self, path, asset_rows, target):
        """Decode embedded maps and compare channels against each slot atlas."""
        document, binary = read_glb_payload(path)
        self.record_file(path)
        expected_sets = {}
        preserved = {material_name(name) for name in self.manifest.get("preserved_constant_materials", [])}
        for asset in asset_rows:
            expected_sets.update(sample_texture_sets(asset))
            preserved.update(material_name(name) for name in asset.get("preserved_constant_materials", []))
        primitives = [primitive for mesh in document.get("meshes", []) for primitive in mesh.get("primitives", [])]
        used_indices = {primitive["material"] for primitive in primitives if "material" in primitive}
        texture_rows = []

        def image_signature(info):
            texture = document["textures"][info["index"]]
            image_index = texture.get("source")
            if image_index is None:
                raise ValueError("Texture has no standard embedded image source")
            image = document["images"][image_index]
            if "bufferView" not in image or image.get("mimeType") != "image/png":
                raise ValueError("GLB PBR images must be embedded PNG bufferViews")
            view = document["bufferViews"][image["bufferView"]]
            if view.get("buffer", 0) != 0:
                raise ValueError("Image uses an external or unsupported buffer")
            offset, length = view.get("byteOffset", 0), view["byteLength"]
            payload = binary[offset:offset + length]
            if len(payload) != length or not payload.startswith(PNG_SIGNATURE):
                raise ValueError("Embedded PNG payload is truncated or invalid")
            key = hashlib.sha256(payload).hexdigest()
            if key not in self.embedded_signatures:
                self.embedded_signatures[key] = self.decode_embedded_png(payload, key)
            return self.embedded_signatures[key]

        for index in sorted(used_indices):
            material = document["materials"][index]
            name = material_name(material.get("name", ""))
            pbr = material.get("pbrMetallicRoughness", {})
            if name not in expected_sets:
                self.check(name in preserved, target, "undeclared_glb_material", "GLB material is neither baked nor preserved: " + name)
                values = list(pbr.get("baseColorFactor", [1, 1, 1, 1])) + [pbr.get("metallicFactor", 1), pbr.get("roughnessFactor", 1)]
                self.check(all(isinstance(value, (int, float)) and math.isfinite(value) for value in values),
                           target, "constant_pbr_parameters", "Preserved material PBR factors must be finite: " + name)
                texture_rows.append({"material": name, "preserved_constant": True})
                continue
            bindings = (("BaseColor", pbr.get("baseColorTexture"), "RGB"),
                        ("Normal", material.get("normalTexture"), "RGB"),
                        ("ORM", pbr.get("metallicRoughnessTexture"), "G"),
                        ("ORM", pbr.get("metallicRoughnessTexture"), "B"),
                        ("ORM", material.get("occlusionTexture"), "R"))
            row = {"material": name, "channels": []}
            uv_channels = set()
            for role, info, channel in bindings:
                if info is None:
                    self.check(False, target, "missing_pbr_channel", name + " lacks " + role + ":" + channel)
                    continue
                uv_index = info.get("extensions", {}).get("KHR_texture_transform", {}).get("texCoord", info.get("texCoord", 0))
                uv_channels.add(uv_index)
                material_primitives = [primitive for primitive in primitives if primitive.get("material") == index]
                self.check(all("TEXCOORD_" + str(uv_index) in primitive.get("attributes", {}) for primitive in material_primitives),
                           target, "texture_uv_channel", name + " has a texture bound to a missing UV channel")
                try:
                    actual = image_signature(info)
                    expected = self.expected_signature(expected_sets[name][role])
                    match = actual["size"] == expected["size"] and actual["channels"][channel] == expected["channels"][channel]
                    self.check(match, target, "pbr_pixel_mismatch", name + " embedded " + role + ":" + channel + " differs from its declared atlas")
                    row["channels"].append({"role": role, "channel": channel, "size": actual["size"], "pixels_match": match})
                except Exception as exc:
                    self.check(False, target, "pbr_image_decode", name + ": " + str(exc))
            self.check(len(uv_channels) == 1, target, "inconsistent_atlas_uv", name + " PBR maps must use one consistent UV channel.")
            texture_rows.append(row)
        self.report.setdefault("glb_pbr", {})[target] = texture_rows

    def run(self):
        if not bpy.app.background:
            raise RuntimeError("This verifier only runs in background Blender; live sessions are refused.")
        manifest_path = self.root / "AssetManifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        self.manifest = manifest
        self.record_file(manifest_path)
        self.record_delivery_files()
        if len(manifest["assets"]) != 17 or len(manifest.get("presets", [])) != 3:
            raise ValueError("Material sample requires 17 assets and three presets.")
        self.check_texture_declarations(manifest["assets"])
        material_report = (self.root / manifest.get("material_report", "MaterialReport.json")).resolve()
        if material_report.is_file():
            self.record_file(material_report)
        source_path = (self.root / manifest.get("source_file", "Source/M7_Material_Study.blend")).resolve()
        if not source_path.is_relative_to((self.root / "Source").resolve()):
            raise ValueError("Source file must be inside the sample Source directory")
        self.source_path = source_path
        self.report["source_file"] = str(source_path)
        self.report["source_sha256_before"] = sha256(source_path)
        bpy.ops.wm.open_mainfile(filepath=str(source_path), load_ui=False)
        assets = manifest["assets"]
        mounts = manifest.get("mounts", {})
        self.check("Barrel" in mounts, "library", "barrel_mount_manifest",
                   "Manifest must define the stable receiver Barrel socket.")
        preset_rows = manifest.get("presets", [])
        preset_names = [row["name"] for row in preset_rows]
        self.check(bool(preset_rows) and len(preset_names) == len(set(preset_names)),
                   "library", "preset_names", "Exported presets must be nonempty and have unique names.")
        presets = {row["name"]: row for row in preset_rows}
        self.check(bool(assets), "library", "asset_count", "Asset library must not be empty.")
        if "expected_asset_count" in manifest:
            self.check(len(assets) == manifest["expected_asset_count"], "library", "asset_count",
                       "Asset count differs from manifest.expected_asset_count.")
        self.check(sum(asset["category"] == "Core" for asset in assets) == 1,
                   "library", "core_count", "The library must define exactly one Core.")
        self.check(any(asset["mount_key"] == "Barrel" for asset in assets),
                   "library", "barrel_count", "The library must define a replaceable Barrel combo.")
        asset_names = [asset["name"] for asset in assets]
        self.check(len(asset_names) == len(set(asset_names)), "library", "duplicate_asset_name",
                   "Asset names must be unique.")
        defaults = manifest["default_parts"]
        self.check(bool(defaults) and len(defaults) == len(set(defaults)), "library", "default_count",
                   "Default parts must be nonempty and unique.")
        self.check(set(defaults) == {asset["name"] for asset in assets if asset["default"]},
                   "library", "default_manifest", "default_parts and per-asset default flags must agree.")
        self.check(any(preset["parts"] == defaults for preset in preset_rows), "library",
                   "default_preset_parts", "One exported preset must exactly match default_parts.")
        scene_names = manifest.get("scenes", {})
        edit_name = scene_names.get("edit", "M7_Library_Edit")
        catalog_name = scene_names.get("catalog", "M7_Attachment_Catalog")
        required_scene_names = list(dict.fromkeys([edit_name, catalog_name] +
                                   [preset.get("scene", "M7_Default_Assembly" if preset["parts"] == defaults else "M7_Equipped_Assembly")
                                    for preset in preset_rows]))
        scenes = {name: bpy.data.scenes.get(name) for name in required_scene_names}
        for name, scene in scenes.items():
            self.check(scene is not None, "library", "missing_scene", "Missing required scene: " + name)
        obsolete = {"scenes": [scene.name for scene in bpy.data.scenes
                                if "TIDELINE" in scene.name.upper() or "M7_BLACK" in scene.name.upper()],
                    "objects": [obj.name for obj in bpy.data.objects
                                if "TIDELINE" in obj.name.upper() or "M7_BLACK" in obj.name.upper()]}
        self.check(not any(obsolete.values()), "library", "obsolete_asset_data",
                   "Saved source contains an object or scene from TIDELINE or M7_BLACK.")
        self.report["library"]["obsolete_data"] = obsolete
        edit = scenes[edit_name]
        if edit is None:
            return
        expand_layer_collections(edit.view_layers[0].layer_collection)
        edit.view_layers[0].update()
        by_collection = {asset["collection"]: asset for asset in assets}
        by_name = {asset["name"]: asset for asset in assets}
        self.report["library"]["preset_instances"] = {}
        for preset in preset_rows:
            scene_name = preset.get("scene", "M7_Default_Assembly" if preset["parts"] == defaults else "M7_Equipped_Assembly")
            scene = scenes.get(scene_name)
            if scene is None:
                continue
            try:
                resolved = resolve_preset(manifest, by_name, preset)
                expected = [by_name[name]["collection"] for name in preset["parts"]]
                self.report["library"]["preset_instances"][preset["name"]] = self.check_instances(
                    scene, expected, by_collection, placements=resolved["part_mounts_m"])
            except Exception as exc:
                self.check(False, preset["name"], "preset_instance_schema", str(exc))
        if scenes.get(catalog_name) is not None:
            expected = [asset["collection"] for asset in assets if asset["category"] != "Core"]
            self.report["library"]["catalog_instances"] = self.check_instances(
                scenes[catalog_name], expected, by_collection, catalog=True)
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
                if asset["category"] == "Core":
                    forbidden_prefixes = ("CORE_DefaultHandguard", "CORE_Handguard", "CORE_BarrelVisible",
                                          "CORE_BarrelShoulder", "CORE_BarrelShortFront", "CORE_LowerRail")
                    duplicate_forward = [obj.name for obj in collection.all_objects
                                         if obj.type in GEOMETRY_TYPES and obj.name.startswith(forbidden_prefixes)]
                    self.check(not duplicate_forward, name, "barrel_geometry_in_core",
                               "Core still owns forward combo geometry: " + ", ".join(duplicate_forward[:8]))
                    obsolete_sockets = [obj.name for obj in collection.all_objects if obj.type == "EMPTY" and
                                        any("Mount_" + key in obj.name for key in DEPENDENT_MOUNT_KEYS)]
                    self.check(not obsolete_sockets, name, "forward_socket_in_core",
                               "Forward sockets must belong to Barrel combos, not Core: " + ", ".join(obsolete_sockets))
                    for obj in collection.all_objects:
                        if obj.type == "MESH" and obj.name.startswith(("CORE_TopRailBase", "CORE_RailTooth")):
                            rail_max_x = max((obj.matrix_world @ Vector(corner)).x for corner in obj.bound_box)
                            self.check(rail_max_x <= mounts["Barrel"][0] + .004, name,
                                       "unsplit_top_rail", "Receiver top rail extends into the removable forward rail: " + obj.name)
                stats, materials = mesh_statistics(edit, list(collection.all_objects), source=True)
                row["source"] = stats
                source_materials.update(materials)
                source_stats[name] = stats
                self.check_geometry(stats, name + ":source")
                self.check_source_materials(asset, collection, materials)
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
                        path = export_path(self.root, asset, kind)
                        self.record_file(path)
                        imported = import_statistics(path, kind)
                        row[kind] = imported
                        self.compare_export(imported, expected, 1, target, root_zero=kind == "glb")
                        if kind == "glb":
                            sockets = {}
                            if asset["category"] == "Core":
                                sockets = {key: [point[k] - asset["mount_position_m"][k] for k in range(3)]
                                           for key, point in fixed_mounts(manifest).items()}
                            elif asset["mount_key"] == "Barrel":
                                sockets = asset["local_mounts_m"]
                                self.check(all(key in sockets for key in DEPENDENT_MOUNT_KEYS),
                                           target, "barrel_socket_schema", "Barrel must define Foregrip, Muzzle and FrontIron sockets.")
                            self.check(sorted(asset.get("glb_mount_nodes", [])) == sorted("Mount_" + key for key in sockets),
                                       target, "asset_mount_manifest", "Declared GLB sockets differ from their owner asset.")
                            self.check_glb_mounts(imported, sockets, target)
                            self.check_glb_pbr(export_path(self.root, asset, kind), [asset], target)
                    except Exception as exc:
                        row[kind] = {"error": str(exc)}
                        self.check(False, target, "import_failed", str(exc))
            except Exception as exc:
                row["error"] = str(exc)
                self.check(False, name, "source_failed", str(exc))
        role_by_filename = {Path(relative).name: role for asset in assets
                            for maps in sample_texture_sets(asset).values() for role, relative in maps.items()}
        textures = packed_texture_statistics(source_materials, role_by_filename)
        self.report["library"]["packed_textures"] = textures
        for role in TEXTURE_ROLES:
            role_rows = [row for row in textures if row["role"] == role]
            self.check(bool(role_rows), "library", "missing_" + role.lower(),
                       "Source materials must reference packed PNG " + role + " maps.")
            for texture in role_rows:
                self.check(texture["packed_bytes"] > 0 and texture["packed_png"] and
                           all(value > 0 for value in texture["size"]), texture["name"],
                           "packed_texture", "Image must have nonempty packed PNG data and valid dimensions.")
        for preset in preset_rows:
            self.validate_preset(preset, source_stats, by_name)

    def finish(self):
        if hasattr(self, "manifest"):
            try:
                self.record_delivery_files()
            except Exception as exc:
                self.check(False, "delivery", "delivery_binding_failed", str(exc))
        if hasattr(self, "source_path") and self.source_path.is_file():
            self.report["source_sha256_after"] = sha256(self.source_path)
            self.report["source_unchanged"] = (
                self.report["source_sha256_before"] == self.report["source_sha256_after"])
            self.check(self.report["source_unchanged"], "library", "source_modified",
                       "Saved source .blend SHA256 changed during validation.")
            self.record_file(self.source_path)
        self.report["success"] = not self.report["failures"]
        self.report["summary"] = {"assets_checked": len(self.report["assets"]),
                                  "asset_exports_imported": sum(
                                      int(kind in row and "error" not in row[kind])
                                      for row in self.report["assets"] for kind in ("fbx", "glb")),
                                  "assembly_exports_imported": sum(
                                      int(kind in row and "error" not in row[kind])
                                      for row in self.report["presets"].values()
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
    if options.cleanup_image_cache is not None:
        if not bpy.app.background:
            raise RuntimeError("Only clean completed validation caches from background Blender")
        count = cleanup_validation_image_cache(options.cleanup_image_cache, options.root.resolve())
        print(json.dumps({"image_cache_cleaned": str(options.cleanup_image_cache.resolve()), "files_removed": count}))
        return
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
