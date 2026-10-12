"""Reference-based Resonance generation-one foregrip, for a visual M7 prop.

This generator only creates external appearance meshes. It has no mechanisms
or manufacturing data. Coordinates use metres, +X towards the muzzle, +Z up;
the source geometry is in common assembled space, while its root remains at
zero. The large hand window and narrow lower opening are actual mesh holes.

Reference priority: Game_ResonanceGen1_K416.jpg and
Game_ResonanceGen1_ASVal.jpg for the side silhouette; ResonanceGen1_Icon.png
for curved front grip, grip panels, clamp channel and fastener appearance.
"""

import math
import random


ASSET_NAME = "SM_M7_Foregrip_ResonanceGen1"
MOUNT = (.3075, 0.0, -.0435)


def _bezier(a, b, c, d, steps=20):
    """Exclude the starting point so adjacent curve segments have no duplicates."""
    return [tuple((1-t)**3*a[k] + 3*(1-t)**2*t*b[k]
                  + 3*(1-t)*t*t*c[k] + t**3*d[k] for k in (0, 1))
            for t in (i/steps for i in range(1, steps+1))]


def _outline(start, segments):
    points = [start]
    for segment in segments:
        if len(segment) == 1:
            points.append(segment[0])
        else:
            points.extend(_bezier(points[-1], *segment))
    if points[-1] == points[0]:
        points.pop()
    return points


def _cross(a, b):
    return a[0]*b[1] - a[1]*b[0]


def _radial_point(points, center, theta, outer):
    direction = (math.cos(theta), math.sin(theta))
    hits = []
    for p, q in zip(points, points[1:] + points[:1]):
        edge = (q[0]-p[0], q[1]-p[1])
        rel = (p[0]-center[0], p[1]-center[1])
        determinant = _cross(direction, edge)
        if abs(determinant) < 1e-12:
            continue
        distance = _cross(rel, edge) / determinant
        fraction = _cross(rel, direction) / determinant
        if distance >= -1e-10 and -1e-9 <= fraction <= 1+1e-9:
            hits.append(max(distance, 0.0))
    if not hits:
        raise ValueError("Reference profile is not closed about its interior.")
    distance = max(hits) if outer else min(hits)
    return (center[0]+distance*direction[0], center[1]+distance*direction[1])


def _inside(p, polygon):
    inside = False
    for a, b in zip(polygon, polygon[1:] + polygon[:1]):
        if ((a[1] > p[1]) != (b[1] > p[1])
                and p[0] < (b[0]-a[0])*(p[1]-a[1])/(b[1]-a[1])+a[0]):
            inside = not inside
    return inside


def _body_half_width(x):
    if x >= -.016:
        return .019
    if x <= -.026:
        return .0135
    return .0135 + (x+.026)/.010*.0055


def build_resonance_grip(api):
    """Create a separate one-generation foregrip asset; do not execute on import."""
    if not isinstance(api, dict):
        api = {name: getattr(api, name) for name in
               ("begin_asset", "box", "cyl", "tube", "mesh", "profile", "text")}
    api["begin_asset"](ASSET_NAME, "Foregrip", "Foregrip", is_default=False)
    made = []
    prefix = "GEO_M7_ResonanceGen1_"

    def world(p):
        return tuple(MOUNT[i]+p[i] for i in range(3))

    def keep(obj):
        made.append(obj)
        return obj

    def mesh(name, verts, faces, mat, bevel=.0006, smooth_faces=None):
        obj = keep(api["mesh"](prefix+name, [world(p) for p in verts], faces, mat, bevel))
        if smooth_faces and hasattr(obj, "data") and hasattr(obj.data, "polygons"):
            for i in smooth_faces:
                obj.data.polygons[i].use_smooth = True
        return obj

    def box(name, pos, dims, mat, bevel=.0006):
        return keep(api["box"](prefix+name, world(pos), dims, mat, bevel))

    def profile(name, points, width, mat, bevel=.0006, y=0.0):
        obj = keep(api["profile"](prefix+name,
                   [(MOUNT[0]+x, MOUNT[2]+z) for x, z in points], width, mat, bevel))
        obj.location.y += MOUNT[1]+y
        return obj

    def cylinder(name, pos, radius, depth, mat, vertices=32):
        return keep(api["cyl"](prefix+name, world(pos), radius, depth, mat, 'Y', vertices))

    def ring(name, outer, inner, center, width_fn, mat, bevel=.0006, sections=160):
        """Four loops form a manifold extruded ring; no fake black hole plane."""
        outside = [_radial_point(outer, center, math.tau*i/sections, True)
                   for i in range(sections)]
        inside = [_radial_point(inner, center, math.tau*i/sections, False)
                  for i in range(sections)]
        vertices = []
        for points, side in ((outside, -1), (outside, 1), (inside, -1), (inside, 1)):
            vertices.extend((x, side*width_fn(x), z) for x, z in points)
        faces, smooth = [], []
        for i in range(sections):
            j = (i+1) % sections
            a, b, c, d = i, sections+i, 2*sections+i, 3*sections+i
            aj, bj, cj, dj = j, sections+j, 2*sections+j, 3*sections+j
            faces.extend(((a, aj, bj, b), (c, d, dj, cj),
                          (a, c, cj, aj), (b, bj, dj, d)))
            smooth.extend((len(faces)-4, len(faces)-3))
        return mesh(name, vertices, faces, mat, bevel, smooth)

    # The front member is genuinely curved, with the concave upper finger
    # throat and rounded lower heel visible in both game side references.
    outer = _outline((.056, -.011), [
        ((.054, -.015), (.036, -.020), (.033, -.031)),
        ((.032, -.037), (.040, -.042), (.041, -.050)),
        ((.039, -.064), (.024, -.083), (.005, -.094)),
        ((.001, -.098), (-.008, -.097), (-.014, -.092)),
        ((-.029, -.082),), ((-.045, -.082),),
        ((-.053, -.081), (-.079, -.049), (-.085, -.039)),
        ((-.086, -.012),), ((.056, -.011),)])
    inner = _outline((.006, -.014), [
        ((.021, -.021), (.019, -.033), (.014, -.045)),
        ((.010, -.056), (.000, -.067), (-.015, -.071)),
        ((-.033, -.071),),
        ((-.037, -.071), (-.040, -.069), (-.044, -.064)),
        ((-.068, -.038),),
        ((-.072, -.034), (-.070, -.022), (-.070, -.014)),
        ((.006, -.014),)])
    ring("CurvedGripAndOpenRearFrame", outer, inner, (-.027, -.040),
         _body_half_width, "polymer", .0009, 192)

    # Long open channel: its bottom lies below the rail, and two thin jaws
    # run along its sides. No box fills the mounting rail's central volume.
    box("ClampChannelFloor", (0, 0, -.008), (.168, .0274, .004), "metal", .0008)
    jaw = [(.084, -.010), (.084, -.004), (.081, .002), (.020, .002),
           (.015, .004), (-.045, .004), (-.049, .002), (-.080, .002),
           (-.084, -.004), (-.084, -.010)]
    for sign, side in ((1, "Near"), (-1, "Far")):
        profile("ClampJaw_"+side, jaw, .0056, "metal", .0008, sign*.0170)
        box("ClampLip_"+side, (0, sign*.0146, -.0006),
            (.155, .0015, .0024), "edge", .00045)
        # A long fine line follows the moulded clamp, rather than invented ribs.
        box("ClampLowerSeam_"+side, (0, sign*.01993, -.0065),
            (.153, .00018, .00036), "dark", .00008)
    box("ShortFrontHandStop", (.076, 0, -.016), (.016, .031, .014),
        "polymer", .0020)

    # A second actual through opening forms the narrow lower strap slot.
    slot_outer = _outline((-.013, -.0805), [
        ((-.015, -.087), (-.020, -.091), (-.026, -.091)),
        ((-.046, -.090),),
        ((-.049, -.089), (-.053, -.084), (-.054, -.0805)),
        ((-.013, -.0805),)])
    slot_inner = _outline((-.019, -.0828), [
        ((-.023, -.0875),), ((-.044, -.0868),),
        ((-.047, -.0830),), ((-.019, -.0828),)])
    ring("NarrowOpenLowerSlot", slot_outer, slot_inner, (-.035, -.085),
         lambda x: .0118, "polymer", .00055, 80)

    # Textured panels follow the thick member, with a clean margin around the
    # finger-throat fastener, and stop short of the open hand window.
    panel = _outline((.035, -.018), [
        ((.030, -.024), (.027, -.029), (.028, -.034)),
        ((.031, -.040), (.028, -.047), (.029, -.053)),
        ((.025, -.065), (.017, -.078), (.005, -.086)),
        ((.002, -.089), (-.003, -.090), (-.005, -.087)),
        ((-.012, -.079),),
        ((.000, -.071), (.011, -.057), (.018, -.046)),
        ((.023, -.035), (.022, -.025), (.012, -.017)),
        ((.035, -.018),)])
    for sign, side in ((1, "Near"), (-1, "Far")):
        profile("GripInsetPanel_"+side, panel, .00065, "rubber", .00025, sign*.0191)
        # Deterministic, submillimetre grain visible in closeups: a single
        # mesh per side, not hundreds of separately owned objects.
        vertices, faces = [], []
        rng = random.Random(731)
        row = 0
        z = -.0885
        while z < -.0175:
            x = -.0125 + (row % 2)*.00082
            while x < .0355:
                cx, cz = x+rng.uniform(-.00015, .00015), z+rng.uniform(-.00014, .00014)
                radius = rng.uniform(.00035, .00052)
                if all(_inside((cx+dx, cz+dz), panel) for dx, dz in
                       ((radius*1.7, 0), (-radius*1.7, 0), (0, radius*1.7), (0, -radius*1.7))):
                    base = len(vertices)
                    for height, factor in ((0, 1), (.00018, .57)):
                        for j in range(6):
                            angle = math.tau*j/6+rng.uniform(-.06, .06)
                            vertices.append((cx+radius*factor*math.cos(angle),
                                             sign*(.01948+height),
                                             cz+radius*factor*math.sin(angle)))
                    faces.append(tuple(base+j for j in reversed(range(6))))
                    faces.append(tuple(base+6+j for j in range(6)))
                    for j in range(6):
                        k = (j+1) % 6
                        faces.append((base+j, base+k, base+6+k, base+6+j))
                x += .00164
            row += 1
            z += .00172
        mesh("FineGripGrain_"+side, vertices, faces, "rubber", 0)

        # Flat rear face band and its continuous split line match the slim
        # diagonal brace in the one-generation design.
        brace = [(-.079, -.015), (-.079, -.037), (-.0445, -.0765),
                 (-.0268, -.0765), (-.032, -.072), (-.0415, -.0702),
                 (-.0735, -.0352), (-.0735, -.015)]
        profile("RearBraceFace_"+side, brace, .0004, "metal", .00035, sign*.01365)

        def seam(name, path, half_width=.00019):
            points = []
            for i, p in enumerate(path):
                previous, following = path[max(0, i-1)], path[min(len(path)-1, i+1)]
                dx, dz = following[0]-previous[0], following[1]-previous[1]
                length = math.hypot(dx, dz)
                nx, nz = -dz/length*half_width, dx/length*half_width
                points.extend(((p[0]+nx, sign*(_body_half_width(p[0])+.00015), p[1]+nz),
                               (p[0]-nx, sign*(_body_half_width(p[0])+.00015), p[1]-nz)))
            mesh(name+"_"+side, points,
                 [(2*i, 2*i+1, 2*i+3, 2*i+2) for i in range(len(path)-1)], "dark", 0)

        front_seam = _outline((.049, -.0145), [
            ((.042, -.018), (.034, -.023), (.0308, -.032)),
            ((.030, -.038), (.037, -.043), (.038, -.050)),
            ((.036, -.063), (.022, -.081), (.003, -.092)),])
        seam("FrontMouldPartingLine", front_seam)
        seam("RearBracePartingLine", [(-.081, -.015), (-.081, -.0375),
                                      (-.046, -.079), (-.0265, -.079)])

        # Three recognisable side fasteners, including an actual recessed
        # hexagonal socket in each head, rather than a painted screw icon.
        for index, (x, z, radius) in enumerate(((.0362, -.0468, .0037),
                                               (-.0710, -.0432, .0030),
                                               (-.0528, -.0638, .0030)), 1):
            surface = _body_half_width(x)
            cylinder("Bolt%dWasher_%s" % (index, side),
                     (x, sign*(surface+.00050), z), radius*1.23, .0008, "edge", 40)
            n = 48
            verts = []
            for inward, y in ((False, surface+.00065), (False, surface+.0016),
                               (True, surface+.00065), (True, surface+.0016)):
                for j in range(n):
                    theta = math.tau*j/n
                    rr = radius
                    if inward:
                        local = ((theta + math.pi/6) % (math.pi/3))-math.pi/6
                        rr = radius*.49/math.cos(local)
                    verts.append((x+rr*math.cos(theta), sign*y, z+rr*math.sin(theta)))
            faces = []
            for j in range(n):
                k = (j+1) % n
                faces.extend(((j, k, n+k, n+j), (2*n+j, 3*n+j, 3*n+k, 2*n+k),
                              (n+j, n+k, 3*n+k, 3*n+j), (j, 2*n+j, 2*n+k, k)))
            mesh("Bolt%dSocketHead_%s" % (index, side), verts, faces,
                 "steel", .00010)
            cylinder("Bolt%dSocketInterior_%s" % (index, side),
                     (x, sign*(surface+.0007), z), radius*.45, .00016, "dark", 6)

    # Front wrapping pad carries the same subtle rubber finish as side panels.
    curve = _outline((.0400, -.053), [
        ((.037, -.066), (.022, -.083), (.006, -.093)),])
    verts, inside_verts = [], []
    for i, p in enumerate(curve):
        a, b = curve[max(0, i-1)], curve[min(len(curve)-1, i+1)]
        dx, dz = b[0]-a[0], b[1]-a[1]
        length = math.hypot(dx, dz)
        nx, nz = -dz/length*.00038, dx/length*.00038
        width = .0120-.0033*i/(len(curve)-1)
        verts.extend(((p[0]+nx, -width, p[1]+nz), (p[0]+nx, width, p[1]+nz)))
        inside_verts.extend(((p[0]+nx*.2, -width, p[1]+nz*.2),
                             (p[0]+nx*.2, width, p[1]+nz*.2)))
    offset = len(verts)
    verts += inside_verts
    wrap_faces = []
    for i in range(len(curve)-1):
        a, b, c, d = 2*i, 2*i+1, 2*i+3, 2*i+2
        wrap_faces.extend(((a, b, c, d), (offset+a, offset+d, offset+c, offset+b),
                           (a, d, offset+d, offset+a), (b, offset+b, offset+c, c)))
    wrap_faces.extend(((0, offset, offset+1, 1),
                       (offset-2, offset-1, 2*offset-1, 2*offset-2)))
    mesh("CurvedFrontRubberWrap", verts, wrap_faces, "rubber", 0)

    return {"name": ASSET_NAME, "objects": made, "mount": MOUNT,
            "visualnotes": [
                "Generation one: long rail clamp, curved solid front and diagonal open rear frame.",
                "Primary hand window and slim lower strap slot are actual mesh openings.",
                "Curved black grip pads, submillimetre grain, continuous split lines and three side bolts.",
                "Open clamp channel preserves rail clearance; common assembled coordinates, zero root."]}
