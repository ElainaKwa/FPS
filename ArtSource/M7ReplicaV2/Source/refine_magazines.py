"""Refine the two M7 straight magazines as external visual meshes.

The slanted top, authored attachment origin, insertion neck and floor outline
stay in place.  A continuous rounded shell replaces flat side overlays.  The
shell itself carries pressed fields, shallow grooves and rolled borders; no
functional magazine internals are modeled.  Importing this module does not
change Blender.  Call ``refine_magazines()`` explicitly in the V2 library.
"""

import math
from pathlib import Path

import bpy
import bmesh
from mathutils import Matrix, Vector


ROOT = Path(r"D:/Unreal Projects/MCP_Test/ArtSource/M7ReplicaV2")
VERSION = 1
MAGAZINES = (("SM_M7_Mag_20", "Mag_20", 0),
             ("SM_M7_Mag_Extended", "Mag_Extended", 110))
OLD_FLAT_DETAILS = (
    "UpperFront_", "MidFront_", "LowerFront_", "BroadFlatSide_",
    "SlantedUpperSeam_", "RearEdgeRib_", "FrontSideDivision_",
    "FirstHorizontalMouldSeam_", "SecondHorizontalMouldSeam_",
    "FrontLowerBorder_", "FloorPlateInset_", "ExtensionMouldSeam_",
    "ExtendedLowerPanel_", "RearThinTransverseRidge_",
)


def _smooth(a, b, value):
    if a == b:
        return float(value >= b)
    t = max(0.0, min(1.0, (value - a) / (b - a)))
    return t * t * (3.0 - 2.0 * t)


def _field(px, py, x0, x1, top, bottom):
    """Pressed field with a rounded transition and a tiny rolled border."""
    distance = min(px - x0, x1 - px, py - top, bottom - py)
    interior = _smooth(0, 3.6, distance)
    rolled_edge = .00055 * math.exp(-((distance - 1.3) / 1.8) ** 2)
    return rolled_edge - .00068 * interior


def _relief(px, py, extra, v, face_u):
    """Y displacement is part of the shell, rather than separate box plates."""
    edge_fade = _smooth(0, .065, face_u) * _smooth(1, .935, face_u)
    end_fade = _smooth(.035, .11, v) * _smooth(.975, .915, v)
    convex = .00080 * math.sin(math.pi * face_u) ** 2 * math.sin(math.pi * v) ** 2
    value = convex
    value += _field(px, py, 1070, 1179,
                    629 + (px - 1070) * .270, 785 + extra)
    value += _field(px, py, 1003, 1062,
                    606 + (px - 1003) * .28, 641 + (px - 1003) * .28)
    value += _field(px, py, 1003, 1062,
                    647 + (px - 1003) * .23, 684 + (px - 1003) * .23)
    value += _field(px, py, 1003, 1062,
                    705 + (px - 1003) * .13, 782 + extra)
    # Longitudinal pressed ribs separate the broad panel from the narrow front
    # fields, and reinforce the rear edge without sharp pasted-on bars.
    gate = _smooth(616, 637, py) * _smooth(791 + extra, 779 + extra, py)
    for axis_px, height, spread in ((1066, .00074, 2.8), (1190, .00057, 2.6)):
        value += gate * height * math.exp(-((px - axis_px) / spread) ** 2)
    front_gate = _smooth(1004, 1011, px) * _smooth(1066, 1060, px)
    for baseline, slope in ((681, .22), (709, .15)):
        offset = py - (baseline + (px - 1004) * slope)
        value -= front_gate * .00058 * math.exp(-(offset / 2.5) ** 2)
        value += front_gate * .00012 * math.exp(-((abs(offset) - 4.1) / 1.9) ** 2)
    if extra:
        extension_gate = _smooth(1007, 1015, px) * _smooth(1188, 1181, px)
        value -= extension_gate * .00046 * math.exp(-((py - 790) / 2.7) ** 2)
        value += _field(px, py, 1077, 1174, 809, 888)
    return edge_fade * end_fade * value


def _shell_geometry(extra, across=96, vertical=92, corner_steps=6, edge_steps=8):
    """Closed swept shell, with true rounded X/Y corners and the traced top.

    No side-to-side sweep is introduced in XZ: these remain straight box
    magazines.  Top endpoints are exactly p(999,576) and p(1200,637).
    """
    vertices = []
    ring_count = None
    for ring in range(vertical + 1):
        v = ring / vertical
        front = 999 - 4 * _smooth(.04, .27, v) + 11 * _smooth(.969, 1, v)
        rear = 1200 + _smooth(.3, .62, v) + 6 * _smooth(.82, .925, v) - 11 * _smooth(.96, 1, v)
        maximum_x = (1250 - front) * .0005
        minimum_x = (1250 - rear) * .0005
        half_y = .0208 + .00026 * math.sin(math.pi * v) ** 2
        corner = .00285
        section = []

        def add(x, y, side=0, face_u=0):
            px = 1250 - x / .0005
            u = (px - front) / (rear - front)
            top = 576 + 61 * u
            py = top + (830 + extra - top) * v
            if side:
                y += side * _relief(px, py, extra, v, face_u)
            section.append((x, y, (380 - py) * .0005))

        # Positive Y broad side, muzzle edge to rear edge.
        for i in range(across + 1):
            u = i / across
            x = maximum_x - corner - (maximum_x - minimum_x - 2 * corner) * u
            add(x, half_y, 1, u)
        for i in range(1, corner_steps + 1):
            a = math.pi / 2 + math.pi / 2 * i / corner_steps
            add(minimum_x + corner + corner * math.cos(a),
                half_y - corner + corner * math.sin(a))
        for i in range(1, edge_steps + 1):
            add(minimum_x, half_y - corner - (2 * half_y - 2 * corner) * i / edge_steps)
        for i in range(1, corner_steps + 1):
            a = math.pi + math.pi / 2 * i / corner_steps
            add(minimum_x + corner + corner * math.cos(a),
                -half_y + corner + corner * math.sin(a))
        for i in range(1, across + 1):
            u = i / across
            x = minimum_x + corner + (maximum_x - minimum_x - 2 * corner) * u
            add(x, -half_y, -1, 1 - u)
        for i in range(1, corner_steps + 1):
            a = math.pi * 1.5 + math.pi / 2 * i / corner_steps
            add(maximum_x - corner + corner * math.cos(a),
                -half_y + corner + corner * math.sin(a))
        for i in range(1, edge_steps + 1):
            add(maximum_x, -half_y + corner + (2 * half_y - 2 * corner) * i / edge_steps)
        for i in range(1, corner_steps + 1):
            a = math.pi / 2 * i / corner_steps
            add(maximum_x - corner + corner * math.cos(a),
                half_y - corner + corner * math.sin(a))
        # The last arc endpoint is the first broad-face point.
        section.pop()
        if ring_count is None:
            ring_count = len(section)
        assert len(section) == ring_count
        vertices.extend(section)
    faces = [tuple(reversed(range(ring_count))),
             tuple(range(vertical * ring_count, (vertical + 1) * ring_count))]
    for ring in range(vertical):
        for i in range(ring_count):
            j = (i + 1) % ring_count
            faces.append((ring * ring_count + i, ring * ring_count + j,
                          (ring + 1) * ring_count + j, (ring + 1) * ring_count + i))
    return vertices, faces


def _floor_roll_geometry(extra, radius=.00046, cross_segments=8):
    """Continuous rounded rectangular roll around the floorplate shoulder."""
    center_x = (1250 - 1102) * .0005
    z = (380 - (807 + extra)) * .0005
    half_x, half_y, corner = .0520, .0237, .0045
    path = []
    for cx, cy, first in ((half_x - corner, half_y - corner, 0),
                          (-half_x + corner, half_y - corner, math.pi / 2),
                          (-half_x + corner, -half_y + corner, math.pi),
                          (half_x - corner, -half_y + corner, math.pi * 1.5)):
        for i in range(9):
            a = first + math.pi / 2 * i / 8
            path.append((center_x + cx + corner * math.cos(a), cy + corner * math.sin(a)))
    vertices = []
    for i, point in enumerate(path):
        before, after = path[(i - 1) % len(path)], path[(i + 1) % len(path)]
        dx, dy = after[0] - before[0], after[1] - before[1]
        length = math.hypot(dx, dy)
        nx, ny = dy / length, -dx / length
        for k in range(cross_segments):
            a = math.tau * k / cross_segments
            rr = radius * math.cos(a)
            vertices.append((point[0] + nx * rr, point[1] + ny * rr,
                             z + radius * math.sin(a)))
    faces = []
    for i in range(len(path)):
        j = (i + 1) % len(path)
        for k in range(cross_segments):
            h = (k + 1) % cross_segments
            faces.append((i * cross_segments + k, i * cross_segments + h,
                          j * cross_segments + h, j * cross_segments + k))
    return vertices, faces


def refine_magazines():
    """Return {changed_objs, new_objs, removed_names}; source scene is restored."""
    if bpy.context.window is None or bpy.context.mode != "OBJECT":
        raise RuntimeError("Refine magazines in Blender Object Mode with a window context.")
    edit = bpy.data.scenes.get("M7_Library_Edit")
    if edit is None:
        raise RuntimeError("The V2 M7_Library_Edit scene is missing.")
    collections = {}
    for name, prefix, extra in MAGAZINES:
        collection = bpy.data.collections.get(name)
        if collection is None:
            raise RuntimeError("Missing magazine collection: " + name)
        if not collection.objects.get(prefix + "_StraightBoxBody"):
            raise RuntimeError("Missing magazine shell: " + prefix)
        collections[name] = collection
    polymer = bpy.data.materials.get("M7R_02_PolymerBlack")
    if polymer is None:
        raise RuntimeError("The original black polymer material is missing.")
    changed_objs, new_objs, removed_names, uv_objects = [], [], [], []
    processed_collections = []
    previous_scene = bpy.context.window.scene
    previous_layer = bpy.context.window.view_layer
    edit_layer = edit.view_layers[0]
    exclusions = []
    bpy.context.window.scene = edit
    bpy.context.window.view_layer = edit_layer
    previous_selection = [obj.name for obj in edit_layer.objects if obj.select_get()]
    previous_active = edit_layer.objects.active.name if edit_layer.objects.active else None

    def make_data(name, vertices, faces):
        data = bpy.data.meshes.new(name)
        data.from_pydata(vertices, [], faces)
        data.update()
        bm = bmesh.new()
        bm.from_mesh(data)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(data)
        bm.free()
        data.materials.append(polymer)
        for polygon in data.polygons:
            polygon.use_smooth = len(polygon.vertices) == 4
        return data

    def restore_finish(obj, width, segments=4):
        for modifier in list(obj.modifiers):
            if modifier.type in {"BEVEL", "WEIGHTED_NORMAL"}:
                obj.modifiers.remove(modifier)
        bevel = obj.modifiers.new("Visual moulded edge roll", "BEVEL")
        bevel.width = width
        bevel.segments = segments
        bevel.limit_method = "ANGLE"
        bevel.angle_limit = .65
        bevel.harden_normals = True
        normals = obj.modifiers.new("Visual moulded surface normals", "WEIGHTED_NORMAL")
        normals.keep_sharp = True
        normals.weight = 35

    def select_only(objects):
        for obj in edit_layer.objects:
            if obj.select_get():
                obj.select_set(False)
        for obj in objects:
            obj.select_set(True)
        edit_layer.objects.active = objects[0] if objects else None

    try:
        for name, collection in collections.items():
            layer = edit_layer.layer_collection.children.get(collection.name)
            if layer is not None:
                exclusions.append((layer, layer.exclude))
                layer.exclude = False
        edit_layer.update()
        for name, prefix, extra in MAGAZINES:
            collection = collections[name]
            if collection.get("visual_magazine_refinement_version", 0) >= VERSION:
                continue
            body = collection.objects[prefix + "_StraightBoxBody"]
            vertices, faces = _shell_geometry(extra)
            # Authored coordinates remain world assembly coordinates.  If the
            # object's local transform changed, compensate in the new data.
            inverse = body.matrix_world.inverted()
            vertices = [tuple(inverse @ Vector(point)) for point in vertices]
            old_data = body.data
            body.data = make_data(prefix + "_RoundedPressedShell", vertices, faces)
            if old_data.users == 0:
                bpy.data.meshes.remove(old_data)
            restore_finish(body, .00050, 3)
            body["visual_magazine_refinement"] = VERSION
            changed_objs.append(body)
            uv_objects.append(body)
            # Only superseded flat overlays from the original generator are
            # removed.  Neck, collar, floorplate, lock catch and user additions
            # keep their original geometry and authored attachment transforms.
            for obj in list(collection.objects):
                if any(obj.name.startswith(prefix + "_" + stem) for stem in OLD_FLAT_DETAILS):
                    removed_names.append(obj.name)
                    data = obj.data if obj.type == "MESH" else None
                    bpy.data.objects.remove(obj, do_unlink=True)
                    if data is not None and data.users == 0:
                        bpy.data.meshes.remove(data)
            for suffix, width in (("UpperSlantedCollar", .00078),
                                  ("FloorPlate", .00105),
                                  ("FloorPlateBottomLip", .00072)):
                obj = collection.objects.get(prefix + "_" + suffix)
                if obj is not None:
                    restore_finish(obj, width, 5)
                    obj["visual_magazine_refinement"] = VERSION
                    changed_objs.append(obj)
            vertices, faces = _floor_roll_geometry(extra)
            data = make_data(prefix + "_ContinuousFloorRoll", vertices, faces)
            roll_name = "RFN_" + prefix + "_ContinuousFloorRoll"
            roll = collection.objects.get(roll_name)
            if roll is None:
                roll = bpy.data.objects.new(roll_name, data)
                collection.objects.link(roll)
                new_objs.append(roll)
            else:
                old_roll_data = roll.data
                roll.data = data
                if old_roll_data.users == 0:
                    bpy.data.meshes.remove(old_roll_data)
                changed_objs.append(roll)
            roll.parent = body.parent
            roll.matrix_world = Matrix.Identity(4)
            roll["visual_magazine_refinement"] = VERSION
            uv_objects.append(roll)
            processed_collections.append(collection)
        # New mesh data alone needs UVs.  Existing floorplate/collar/neck UVs
        # remain intact even though their evaluated bevel edges are improved.
        if uv_objects:
            select_only(uv_objects)
            bpy.ops.object.mode_set(mode="EDIT")
            try:
                bpy.ops.mesh.select_all(action="SELECT")
                bpy.ops.uv.smart_project(angle_limit=1.151917,
                                         island_margin=.012, scale_to_bounds=True)
            finally:
                bpy.ops.object.mode_set(mode="OBJECT")
        edit_layer.update()
        for collection in processed_collections:
            collection["visual_magazine_refinement_version"] = VERSION
    finally:
        if bpy.context.mode != "OBJECT" and bpy.context.window.scene == edit:
            bpy.ops.object.mode_set(mode="OBJECT")
        for obj in edit_layer.objects:
            if obj.select_get():
                obj.select_set(False)
        for obj_name in previous_selection:
            obj = edit_layer.objects.get(obj_name)
            if obj is not None:
                obj.select_set(True)
        edit_layer.objects.active = edit_layer.objects.get(previous_active) if previous_active else None
        for layer, old_exclude in exclusions:
            layer.exclude = old_exclude
        bpy.context.window.scene = previous_scene
        bpy.context.window.view_layer = previous_layer
    return {"changed_objs": changed_objs, "new_objs": new_objs,
            "removed_names": removed_names}


if __name__ == "__main__":
    result = refine_magazines()
    print({"changed_objs": [obj.name for obj in result["changed_objs"]],
           "new_objs": [obj.name for obj in result["new_objs"]],
           "removed_objects": len(result["removed_names"])})
