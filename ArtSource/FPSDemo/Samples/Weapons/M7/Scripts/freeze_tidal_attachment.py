"""Freeze/check the existing Tidal accessory without saving or editing Blender data.

Capture: blender --background --factory-startup --python THIS_SCRIPT -- --freeze
Check:   blender --background --factory-startup --python THIS_SCRIPT -- --check
The common Source .blend remains the only editable authority. Whole-source SHA
is provenance at capture time; subsequent receiver edits are explicitly allowed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import sys
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REGISTRY_NAME = "AttachmentRegistry.json"
ASSET_NAME = "SM_M7_Barrel_Tidal"
CONTRACT_KEYS = ("name", "collection", "category", "mount_key", "mount_position_m",
                 "local_mounts_m", "exports", "texture_sets", "baked_uv_layer",
                 "atlas_resolution", "preserved_constant_materials")


def sha256(path):
    result = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def inside(root, relative):
    root = Path(root).resolve()
    target = (root / relative).resolve()
    if not target.is_relative_to(root):
        raise ValueError("Frozen attachment path outside sample: " + str(relative))
    return target


def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False,
                      allow_nan=False, separators=(",", ":")).encode("utf-8")


def digest_json(value):
    return hashlib.sha256(canonical(value)).hexdigest()


def contract(row):
    return {key: row.get(key) for key in CONTRACT_KEYS}


def load_registry(root=ROOT):
    registry = json.loads(inside(root, REGISTRY_NAME).read_text(encoding="utf-8"))
    if registry.get("schema_version") != 1:
        raise RuntimeError("Unsupported frozen attachment registry schema.")
    entry = registry.get("assets", {}).get(ASSET_NAME)
    if not entry or entry.get("status") != "frozen":
        raise RuntimeError("The required Tidal frozen attachment record is missing.")
    return registry


def check_frozen_files(root=ROOT, manifest=None):
    """Pure filesystem/manifest guard, also usable by the packaging script."""
    root = Path(root).resolve()
    if manifest is None:
        manifest = json.loads((root / "AssetManifest.json").read_text(encoding="utf-8"))
    registry = load_registry(root)
    entry = registry["assets"][ASSET_NAME]
    rows = [row for row in manifest["assets"] if row["name"] == ASSET_NAME]
    if len(rows) != 1 or contract(rows[0]) != entry["manifest_contract"]:
        raise RuntimeError("Frozen Tidal identity, mounts, UV or texture/export bindings changed.")
    if manifest.get("source_file") != entry["source_file"]:
        raise RuntimeError("Frozen attachment must use the common authoritative Source file.")
    if set(entry["files"]) != set(asset_files(rows[0])):
        raise RuntimeError("Frozen Tidal file set is incomplete or changed.")
    for relative, expected in entry["files"].items():
        path = inside(root, relative)
        if not path.is_file() or sha256(path) != expected["sha256"] or path.stat().st_size != expected["bytes"]:
            raise RuntimeError("Frozen Tidal delivery file changed: " + relative)
    return registry


def asset_files(row):
    paths = set(row["exports"].values())
    for channels in row["texture_sets"].values():
        if set(channels) != {"BaseColor", "Normal", "ORM"}:
            raise RuntimeError("Frozen atlas requires BaseColor, Normal and ORM.")
        paths.update(channels.values())
    if len(paths) != 5 or set(row["exports"]) != {"fbx", "glb"}:
        raise RuntimeError("Expected two standalone exports and three Tidal atlas images.")
    return sorted(paths)


def plain(value):
    if value is None or isinstance(value, (bool, str, int)):
        return value
    if isinstance(value, float):
        if not math.isfinite(value):
            raise RuntimeError("Nonfinite frozen data.")
        return value
    try:
        return [plain(item) for item in value]
    except TypeError:
        if hasattr(value, "name_full"):
            return {"id_name": value.name_full, "id_type": type(value).__name__}
        if type(value).__name__ == "CurveProfile":
            return {"type": "CurveProfile",
                    "points": [{key: plain(getattr(point, key))
                                for key in ("location", "handle_type_1", "handle_type_2") if hasattr(point, key)}
                               for point in value.points]}
        return {"rna_type": type(value).__name__}


def matrix_value(matrix):
    return [[float(value) for value in row] for row in matrix]


def image_descriptor(image, root):
    import bpy
    result = {"name": image.name, "source": image.source, "size": list(image.size),
              "colorspace": image.colorspace_settings.name, "alpha_mode": image.alpha_mode}
    path = Path(bpy.path.abspath(image.filepath)).resolve() if image.filepath else None
    result["path"] = path.relative_to(root).as_posix() if path and path.is_relative_to(root) else image.filepath
    packed = getattr(image, "packed_file", None)
    if packed:
        result["packed_sha256"] = hashlib.sha256(bytes(packed.data)).hexdigest()
    elif path and path.is_file():
        result["file_sha256"] = sha256(path)
    return result


def graph_descriptor(tree, root, stack=()):
    if tree is None:
        return None
    if tree.name in stack:
        raise RuntimeError("Recursive frozen material node group: " + tree.name)
    stack = (*stack, tree.name)
    attributes = ("operation", "blend_type", "data_type", "uv_map", "projection",
                  "projection_blend", "interpolation", "extension", "space", "distribution",
                  "normalize", "clamp", "use_clamp", "noise_dimensions", "noise_type",
                  "normalize", "distance", "feature", "wave_type", "bands_direction",
                  "rings_direction", "wave_profile", "gradient_type", "vector_type",
                  "invert", "target", "is_active_output")
    nodes = []
    for node in sorted(tree.nodes, key=lambda node: node.name):
        row = {"name": node.name, "type": node.bl_idname, "mute": node.mute}
        row["parameters"] = {key: plain(getattr(node, key)) for key in attributes if hasattr(node, key)}
        row["inputs"] = [{"identifier": socket.identifier,
                          "value": plain(socket.default_value) if hasattr(socket, "default_value") else None}
                         for socket in node.inputs]
        row["outputs"] = [{"identifier": socket.identifier,
                           "value": plain(socket.default_value) if hasattr(socket, "default_value") else None}
                          for socket in node.outputs]
        if getattr(node, "image", None):
            row["image"] = image_descriptor(node.image, root)
        if getattr(node, "node_tree", None):
            row["group"] = graph_descriptor(node.node_tree, root, stack)
        if hasattr(node, "color_ramp"):
            ramp = node.color_ramp
            row["ramp"] = {"mode": ramp.color_mode, "interpolation": ramp.interpolation,
                           "hue_interpolation": ramp.hue_interpolation,
                           "elements": [[element.position, list(element.color)] for element in ramp.elements]}
        nodes.append(row)
    links = sorted([link.from_node.name, link.from_socket.identifier,
                    link.to_node.name, link.to_socket.identifier] for link in tree.links)
    interface = [{"name": item.name, "type": item.item_type,
                  "direction": getattr(item, "in_out", None),
                  "value": plain(item.default_value) if hasattr(item, "default_value") else None}
                 for item in tree.interface.items_tree]
    return {"name": tree.name, "nodes": nodes, "links": links, "interface": interface}


def material_descriptor(material, root):
    if getattr(material, "is_evaluated", False):
        material = material.original
    attributes = ("diffuse_color", "metallic", "roughness", "use_nodes", "use_backface_culling",
                  "surface_render_method", "use_transparency_overlap")
    return {"name": material.name,
            "properties": {key: plain(getattr(material, key)) for key in attributes if hasattr(material, key)},
            "graph": graph_descriptor(material.node_tree, root)}


def mesh_digest(mesh, include_uv=True):
    import numpy as np
    result = hashlib.sha256()
    fields = {}
    def field(label, collection, attribute, width, dtype):
        values = np.empty(len(collection) * width, dtype=dtype)
        collection.foreach_get(attribute, values)
        result.update(canonical([label, len(collection), width, np.dtype(dtype).str]))
        result.update(values.tobytes())
        fields[label] = hashlib.sha256(values.tobytes()).hexdigest()
    mesh.calc_loop_triangles()
    field("positions", mesh.vertices, "co", 3, "<f4")
    field("triangles", mesh.loop_triangles, "vertices", 3, "<i4")
    field("triangle_material", mesh.loop_triangles, "material_index", 1, "<i4")
    field("loop_vertices", mesh.loops, "vertex_index", 1, "<i4")
    field("corner_normals", mesh.corner_normals, "vector", 3, "<f4")
    for uv in mesh.uv_layers:
        result.update(canonical(["uv", uv.name, len(uv.data), uv.active_render, uv == mesh.uv_layers.active]))
        if include_uv:
            field("uv_coordinates:" + uv.name, uv.data, "uv", 2, "<f4")
    result.update(canonical([material.name if material else None for material in mesh.materials]))
    return {"sha256": result.hexdigest(), "vertices": len(mesh.vertices),
            "triangles": len(mesh.loop_triangles), "uv_layers": [layer.name for layer in mesh.uv_layers],
            "fields": fields}


def modifier_descriptor(modifier):
    """Exact source parameters protect the modifier-generated UV interpolation."""
    result = {"name": modifier.name, "type": modifier.type}
    ignored = {"rna_type", "show_expanded", "is_active", "execution_time", "is_override_data"}
    for prop in modifier.bl_rna.properties:
        if prop.identifier in ignored or prop.type == "COLLECTION":
            continue
        result[prop.identifier] = plain(getattr(modifier, prop.identifier))
    return result


def source_snapshot(root, row, edit, depsgraph=None):
    """Evaluate only the frozen collection in the common edit scene. No mutations."""
    import bpy
    root = Path(root).resolve()
    collection = bpy.data.collections.get(row["collection"])
    if collection is None:
        raise RuntimeError("Missing frozen attachment collection.")
    objects = sorted(list(collection.all_objects), key=lambda obj: obj.name)
    geometry, raw_geometry, transforms, materials = {}, {}, {}, {}
    with bpy.context.temp_override(scene=edit, view_layer=edit.view_layers[0]):
        depsgraph = depsgraph or bpy.context.evaluated_depsgraph_get()
        for obj in objects:
            transforms[obj.name] = {"type": obj.type, "parent": obj.parent.name if obj.parent else None,
                                    "matrix_world": matrix_value(obj.matrix_world)}
            if obj.type not in {"MESH", "FONT"}:
                continue
            evaluated = obj.evaluated_get(depsgraph)
            mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
            try:
                # Blender Bevel interpolated UVs jitter by float32 ULPs between
                # identical evaluations. Freeze raw UV bytes AND all modifier
                # parameters exactly; evaluated topology/positions/normals are
                # still hashed exactly. Existing export UV tests stay strict.
                geometry[obj.name] = mesh_digest(mesh, include_uv=False)
                if obj.type == "MESH":
                    raw_geometry[obj.name] = {"mesh": mesh_digest(obj.data),
                                              "modifiers": [modifier_descriptor(mod) for mod in obj.modifiers]}
                for material in mesh.materials:
                    if material:
                        material = material.original if material.is_evaluated else material
                        materials[material.name] = material
            finally:
                evaluated.to_mesh_clear()
    material_rows = {name: material_descriptor(material, root) for name, material in sorted(materials.items())}
    sockets = {obj.name: {"mount_key": obj.get("mount_key"), "matrix_world": matrix_value(obj.matrix_world)}
               for obj in objects if obj.type == "EMPTY" and "Mount_" in obj.name}
    if len(sockets) != 3:
        raise RuntimeError("Frozen barrel must retain exactly three actual source socket empties.")
    state = {"collection": collection.name, "instance_offset": list(collection.instance_offset),
             "objects": transforms, "geometry": geometry, "raw_geometry": raw_geometry,
             "materials": material_rows, "sockets": sockets}
    return {"sha256": digest_json(state), "geometry_sha256": digest_json(geometry),
            "materials_sha256": digest_json(material_rows), "transforms_sha256": digest_json(transforms),
            "raw_geometry_uv_modifiers_sha256": digest_json(raw_geometry),
            "vertices": sum(item["vertices"] for item in geometry.values()),
            "triangles": sum(item["triangles"] for item in geometry.values()),
            "geometry_objects": len(geometry), "objects": len(objects),
            "instance_offset": state["instance_offset"], "sockets": sockets,
            "object_geometry": geometry,
            "material_hashes": {name: digest_json(value) for name, value in material_rows.items()}}


def guard_source(root, manifest, edit, depsgraph=None):
    registry = check_frozen_files(root, manifest)
    entry = registry["assets"][ASSET_NAME]
    row = next(row for row in manifest["assets"] if row["name"] == ASSET_NAME)
    current = source_snapshot(root, row, edit, depsgraph)
    expected = entry["source_snapshot"]
    if current["sha256"] != expected["sha256"]:
        changed = [key for key in ("geometry_sha256", "raw_geometry_uv_modifiers_sha256",
                                   "materials_sha256", "transforms_sha256")
                   if current[key] != expected[key]]
        objects = [name for name, value in current["object_geometry"].items()
                   if expected["object_geometry"].get(name) != value]
        raise RuntimeError("Frozen Tidal source changed: " + ", ".join(changed or ["collection/socket state"]) +
                           "; objects=" + ",".join(objects))
    return {ASSET_NAME: {"source_sha256": current["sha256"], "files_unchanged": True}}


def expand(layer):
    layer.exclude = False
    for child in layer.children:
        expand(child)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--freeze", action="store_true")
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--check-files", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:])
    root = args.root.resolve()
    manifest = json.loads((root / "AssetManifest.json").read_text(encoding="utf-8"))
    if args.check_files:
        check_frozen_files(root, manifest)
        print("M7_TIDAL_FROZEN_FILES_OK", flush=True)
        return
    import bpy
    if not bpy.app.background:
        raise RuntimeError("Capture/check requires independent background Blender; live sessions refused.")
    source = inside(root, manifest["source_file"])
    before = sha256(source)
    registry_path = root / REGISTRY_NAME
    if args.freeze and not registry_path.exists():
        validation = json.loads((root / "Validation.json").read_text(encoding="utf-8"))
        if not validation.get("success") or validation.get("source_sha256_after") != before:
            raise RuntimeError("Freeze only the successfully validated current saved Source.")
        row = next(row for row in manifest["assets"] if row["name"] == ASSET_NAME)
        for relative in asset_files(row):
            if validation.get("validated_files", {}).get(relative) != sha256(inside(root, relative)):
                raise RuntimeError("Frozen file differs from the successful validation: " + relative)
    bpy.ops.wm.open_mainfile(filepath=str(source), load_ui=False)
    edit = bpy.data.scenes[manifest.get("scenes", {}).get("edit", "M7_Library_Edit")]
    bpy.context.window.scene = edit
    bpy.context.window.view_layer = edit.view_layers[0]
    expand(edit.view_layers[0].layer_collection)
    edit.view_layers[0].update()
    if args.freeze and not registry_path.exists():
        state = source_snapshot(root, row, edit)
        if state["triangles"] != row["triangles"]:
            raise RuntimeError("Frozen evaluated triangles differ from validated manifest.")
        files = {relative: {"sha256": sha256(inside(root, relative)),
                            "bytes": inside(root, relative).stat().st_size}
                 for relative in asset_files(row)}
        registry = {"schema_version": 1, "assets": {ASSET_NAME: {
            "status": "frozen", "frozen_utc": datetime.now(timezone.utc).isoformat(),
            "source_file": manifest["source_file"], "source_sha256_at_freeze": before,
            "validation_sha256_at_freeze": sha256(root / "Validation.json"),
            "manifest_contract": contract(row), "source_snapshot": state, "files": files,
            "fingerprint_policy": "Exact evaluated positions/topology/normals; exact source mesh UVs and modifier parameters. Evaluated Bevel UV interpolation may jitter by float32 ULPs.",
            "reuse": {"standalone": row["exports"], "editable_collection": row["collection"],
                     "authoritative_source": manifest["source_file"],
                     "install_at": "Core/Mount_Barrel",
                     "dependent_sockets": row["local_mounts_m"],
                     "policy": "Replace the installed BarrelCombo; retain one common editable Source."}}}}
        if sha256(source) != before:
            raise RuntimeError("Source changed during freeze capture.")
        temporary = registry_path.with_suffix(".json.tmp")
        temporary.write_text(json.dumps(registry, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, registry_path)
    result = guard_source(root, manifest, edit)
    if sha256(source) != before:
        raise RuntimeError("Read-only attachment capture/check changed Source.")
    print("M7_TIDAL_FROZEN_OK " + json.dumps(result, sort_keys=True), flush=True)


if __name__ == "__main__":
    main()
