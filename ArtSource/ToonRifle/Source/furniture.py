"""Editable stylised rifle furniture, in metres (+X muzzle, +Z up).

No real internal weapon mechanism is represented.  The stock opening is actual
empty space; magazine fluting, grip textures and pad ribs are separate meshes.
The module is executed by the asset's main Blender build script.
"""

import math


def build_furniture(api):
    box = api["box"]
    cyl = api["cyl"]
    tube = api["tube"]
    mesh = api["mesh"]
    text = api["text"]
    mats = api["mats"]
    created = {"stock": [], "grip": [], "magazine": []}

    def keep(obj):
        prefix = obj.name.split("_", 1)[0].lower()
        created[prefix].append(obj)
        return obj

    def b(name, loc, dims, mat, bevel=0.002, rotation=None):
        obj = box(name, loc, dims, mats[mat], bevel=bevel)
        if rotation is not None:
            obj.rotation_euler = rotation
        return keep(obj)

    def c(name, loc, radius, depth, mat, axis="Y", vertices=32):
        return keep(cyl(name, loc, radius, depth, mats[mat], axis=axis,
                        vertices=vertices))

    def t(name, loc, radius, inner, depth, mat, axis="Y", segments=48):
        return keep(tube(name, loc, radius, inner, depth, mats[mat],
                         axis=axis, segments=segments))

    def prism(name, points, y0, y1, mat, bevel=0.002):
        """Extrude an anticlockwise X/Z polygon between two explicit Y planes."""
        area = sum(points[i][0] * points[(i + 1) % len(points)][1]
                   - points[(i + 1) % len(points)][0] * points[i][1]
                   for i in range(len(points)))
        if area < 0:
            points = list(reversed(points))
        count = len(points)
        verts = [(x, y0, z) for x, z in points]
        verts += [(x, y1, z) for x, z in points]
        faces = [tuple(range(count)), tuple(reversed(range(count, count * 2)))]
        for idx in range(count):
            nxt = (idx + 1) % count
            faces.append((idx, count + idx, count + nxt, nxt))
        return keep(mesh(name, verts, faces, mats[mat], bevel=bevel))

    def screw(prefix, label, x, z, side_y, radius=0.0026):
        side = 1 if side_y > 0 else -1
        c(prefix + "_" + label + "_Washer", (x, side_y, z), radius * 1.32,
          0.0011, "dark")
        c(prefix + "_" + label + "_HexHead", (x, side_y + side * 0.0007, z),
          radius, 0.0013, "steel", vertices=6)
        c(prefix + "_" + label + "_HexSocket", (x, side_y + side * 0.0015, z),
          radius * 0.40, 0.00035, "dark", vertices=6)

    # A broad upper comb, side frame and lower rail surround a through-opening.
    # These pieces intentionally do not span the lower opening with a slab.
    prism("Stock_CheekSupport", [(-0.407, 0.017), (-0.397, 0.041),
          (-0.263, 0.042), (-0.245, 0.031), (-0.261, 0.012),
          (-0.393, 0.009)], -0.036, 0.036, "navy", 0.004)
    prism("Stock_RaisedCheekPad", [(-0.405, 0.038), (-0.391, 0.067),
          (-0.275, 0.067), (-0.256, 0.056), (-0.256, 0.043),
          (-0.270, 0.036), (-0.395, 0.032)], -0.040, 0.040, "rubber", 0.006)
    prism("Stock_CheekCrown", [(-0.396, 0.058), (-0.388, 0.069),
          (-0.284, 0.069), (-0.272, 0.060), (-0.287, 0.057)],
          -0.034, 0.034, "teal", 0.003)
    # Front collar is a real open tube around the root-provided buffer tube.
    t("Stock_OpenFrontCollar", (-0.253, 0.0, 0.025), 0.029, 0.0205,
      0.032, "navy", axis="X", segments=64)
    t("Stock_CollarTrim", (-0.236, 0.0, 0.025), 0.028, 0.021,
      0.003, "steel", axis="X", segments=64)
    prism("Stock_RearSpine", [(-0.425, -0.096), (-0.406, -0.092),
          (-0.394, 0.029), (-0.411, 0.048), (-0.428, 0.040)],
          -0.037, 0.037, "navy", 0.004)
    prism("Stock_LowerSkeletonRail", [(-0.415, -0.093), (-0.293, -0.078),
          (-0.283, -0.060), (-0.399, -0.073), (-0.417, -0.070)],
          -0.030, 0.030, "navy", 0.004)
    prism("Stock_ForwardSkeletonBrace", [(-0.298, -0.076),
          (-0.281, -0.074), (-0.245, 0.009), (-0.260, 0.027),
          (-0.275, 0.014), (-0.302, -0.058)], -0.031, 0.031, "navy", 0.004)
    prism("Stock_RearDiagonalBrace", [(-0.398, -0.075), (-0.382, -0.070),
          (-0.368, 0.010), (-0.383, 0.015)], -0.029, 0.029, "navy", 0.003)

    # Side appliques follow the structural members instead of filling the hole.
    for sign, suffix in [(-1, "L"), (1, "R")]:
        y0, y1 = sorted((sign * 0.0363, sign * 0.0383))
        prism("Stock_CheekInsert_" + suffix,
              [(-0.387, 0.020), (-0.380, 0.029), (-0.274, 0.029),
               (-0.266, 0.022), (-0.276, 0.016), (-0.378, 0.016)],
              y0, y1, "teal", 0.0015)
        y0, y1 = sorted((sign * 0.0375, sign * 0.0390))
        prism("Stock_RearSpineInset_" + suffix,
              [(-0.422, -0.078), (-0.413, -0.075), (-0.405, 0.018),
               (-0.414, 0.026)], y0, y1, "teal", 0.0015)
        y0, y1 = sorted((sign * 0.0315, sign * 0.0335))
        prism("Stock_BraceAccent_" + suffix,
              [(-0.287, -0.058), (-0.280, -0.056),
               (-0.259, -0.004), (-0.266, 0.004)],
              y0, y1, "teal", 0.0016)
        prism("Stock_BraceSafetyStripe_" + suffix,
              [(-0.285, -0.050), (-0.280, -0.048),
               (-0.276, -0.038), (-0.281, -0.040)],
              *sorted((sign * 0.0336, sign * 0.0347)), "orange", 0.0008)
        screw("Stock", "CombRear_" + suffix, -0.387, 0.024, sign * 0.039)
        screw("Stock", "CombFront_" + suffix, -0.274, 0.023, sign * 0.039)
        screw("Stock", "BracePivot_" + suffix, -0.277, -0.054,
              sign * 0.034, 0.0032)
        screw("Stock", "HeelUpper_" + suffix, -0.417, 0.031,
              sign * 0.039, 0.003)
        screw("Stock", "HeelLower_" + suffix, -0.415, -0.079,
              sign * 0.039, 0.003)
        for idx in range(5):
            b("Stock_CombFineGroove_" + suffix + "_%02d" % idx,
              (-0.365 + idx * 0.016, sign * 0.041, 0.049),
              (0.009, 0.0011, 0.003), "dark", 0.0007,
              (0, -0.30, 0))

    # Layered heel and thick rounded rubber pad, with individual tread bars.
    prism("Stock_ButtMountingPlate", [(-0.434, -0.097), (-0.414, -0.099),
          (-0.409, 0.040), (-0.421, 0.058), (-0.436, 0.054)],
          -0.040, 0.040, "steel", 0.0035)
    prism("Stock_ButtAccentLayer", [(-0.438, -0.100), (-0.430, -0.099),
          (-0.427, 0.052), (-0.433, 0.058), (-0.440, 0.049)],
          -0.042, 0.042, "teal", 0.002)
    prism("Stock_ThickRubberButtPad", [(-0.447, -0.091), (-0.439, -0.105),
          (-0.430, -0.101), (-0.430, 0.052), (-0.438, 0.060),
          (-0.446, 0.052)], -0.044, 0.044, "rubber", 0.006)
    for idx in range(11):
        z = -0.085 + idx * 0.012
        b("Stock_RubberButtTread_%02d" % idx, (-0.448, 0, z),
          (0.008, 0.078, 0.0067), "dark", 0.0027)
    b("Stock_HeelIdentificationTile", (-0.419, -0.0405, -0.035),
      (0.009, 0.0018, 0.031), "ivory", 0.0016)
    b("Stock_AdjustmentLeverPivot", (-0.267, 0, -0.043),
      (0.026, 0.053, 0.022), "dark", 0.004)
    prism("Stock_AdjustmentLever", [(-0.299, -0.054), (-0.268, -0.055),
          (-0.249, -0.039), (-0.247, -0.025), (-0.257, -0.025),
          (-0.274, -0.042), (-0.298, -0.043)], -0.024, 0.024,
          "orange", 0.003)
    for idx in range(4):
        b("Stock_LeverThumbRib_%02d" % idx, (-0.294 + idx * 0.006, 0, -0.054),
          (0.0025, 0.042, 0.003), "rubber", 0.0008)
    b("Stock_SlingSocketBracket", (-0.348, 0, -0.084),
      (0.025, 0.018, 0.012), "steel", 0.003)
    t("Stock_OpenSlingRing", (-0.348, 0, -0.102),
      0.011, 0.0065, 0.006, "steel", axis="Y", segments=48)
    c("Stock_SlingSocketPin", (-0.348, 0, -0.084),
      0.0037, 0.021, "dark", axis="Y")

    # The pistol grip leans toward the stock; top and bottom are capped meshes.
    grip_outline = [(-0.101, -0.053), (-0.044, -0.061),
                    (-0.050, -0.094), (-0.088, -0.198),
                    (-0.101, -0.215), (-0.139, -0.210),
                    (-0.150, -0.195), (-0.119, -0.111)]
    prism("Grip_MainPolymer", grip_outline, -0.028, 0.028, "navy", 0.006)
    prism("Grip_Backstrap", [(-0.100, -0.066), (-0.113, -0.112),
          (-0.142, -0.195), (-0.135, -0.209), (-0.145, -0.208),
          (-0.154, -0.193), (-0.125, -0.112), (-0.112, -0.062)],
          -0.0225, 0.0225, "rubber", 0.004)
    prism("Grip_FingerFrontstrap", [(-0.045, -0.072), (-0.050, -0.096),
          (-0.090, -0.201), (-0.099, -0.207), (-0.103, -0.199),
          (-0.063, -0.093), (-0.057, -0.072)],
          -0.023, 0.023, "rubber", 0.003)
    side_panel = [(-0.101, -0.092), (-0.067, -0.096), (-0.105, -0.194),
                  (-0.128, -0.190), (-0.131, -0.180)]
    for sign, suffix in [(-1, "L"), (1, "R")]:
        prism("Grip_InsetPalmPanel_" + suffix, side_panel,
              *sorted((sign * 0.0285, sign * 0.0304)), "teal", 0.002)
        screw("Grip", "UpperFastener_" + suffix, -0.085, -0.087,
              sign * 0.0305, 0.003)
        screw("Grip", "LowerFastener_" + suffix, -0.119, -0.193,
              sign * 0.0310, 0.0028)
        for idx in range(8):
            z = -0.108 - idx * 0.0104
            x = -0.087 - idx * 0.0039
            # Short angled rubber bars define ribbing without repeating a slab.
            b("Grip_PalmTextureRib_" + suffix + "_%02d" % idx,
              (x, sign * 0.0317, z), (0.027, 0.0020, 0.0034),
              "rubber", 0.0010, (0, 0.18, 0))
            for dot in range(2):
                c("Grip_PalmTextureStud_" + suffix + "_%02d_%d" % (idx, dot),
                  (x - 0.007 + dot * 0.014, sign * 0.0330, z),
                  0.00105, 0.0008, "navy", vertices=8)
        b("Grip_IndexRest_" + suffix, (-0.059, sign * 0.032, -0.087),
          (0.027, 0.0065, 0.011), "rubber", 0.003,
          (0, -0.18, 0))
        b("Grip_OrangeIndexMarker_" + suffix,
          (-0.057, sign * 0.0355, -0.085),
          (0.011, 0.0013, 0.003), "orange", 0.0008,
          (0, -0.18, 0))
    for idx in range(7):
        z = -0.103 - idx * 0.013
        x = -0.056 - idx * 0.0051
        b("Grip_FrontstrapTraction_%02d" % idx, (x, 0, z),
          (0.005, 0.042, 0.0055), "dark", 0.0018,
          (0, -0.33, 0))
    prism("Grip_BaseCap", [(-0.147, -0.194), (-0.139, -0.213),
          (-0.100, -0.219), (-0.088, -0.206), (-0.100, -0.202),
          (-0.134, -0.200)], -0.0295, 0.0295, "dark", 0.0035)
    prism("Grip_BaseCapIvoryInlay", [(-0.133, -0.211), (-0.105, -0.216),
          (-0.100, -0.211), (-0.131, -0.206)],
          -0.025, 0.025, "ivory", 0.0012)
    t("Grip_OpenLanyardEye", (-0.140, 0, -0.209),
      0.0066, 0.0033, 0.010, "steel", axis="Y", segments=32)

    # Curved magazine: each rounded rectangular cross section remains editable.
    # Slice orientation is horizontal for a calm, slightly exaggerated silhouette.
    def mag_center(value):
        return 0.120 + 0.076 * (value ** 1.65)

    def mag_z(value):
        return -0.067 - 0.223 * value

    def mag_half_x(value):
        return 0.0345 - 0.0035 * value

    def rounded_ring(cx, z, hx, hy, corner=0.0045):
        ring = []
        for ox, oy, start in [(hx - corner, hy - corner, 0),
                              (-hx + corner, hy - corner, 90),
                              (-hx + corner, -hy + corner, 180),
                              (hx - corner, -hy + corner, 270)]:
            for step in range(5):
                angle = math.radians(start + step * 90 / 4)
                ring.append((cx + ox + corner * math.cos(angle),
                             oy + corner * math.sin(angle), z))
        return ring

    def mag_loft(name, start, end, hy, mat, hx_extra=0.0, slices=15,
                 bevel=0.0012):
        verts, faces = [], []
        ring_size = 20
        for idx in range(slices):
            value = start + (end - start) * idx / (slices - 1)
            verts += rounded_ring(mag_center(value), mag_z(value),
                                  mag_half_x(value) + hx_extra, hy)
        faces.append(tuple(reversed(range(ring_size))))
        for idx in range(slices - 1):
            base, nxt = idx * ring_size, (idx + 1) * ring_size
            for edge in range(ring_size):
                edge_next = (edge + 1) % ring_size
                faces.append((base + edge, base + edge_next,
                              nxt + edge_next, nxt + edge))
        faces.append(tuple(range((slices - 1) * ring_size, slices * ring_size)))
        return keep(mesh(name, verts, faces, mats[mat], bevel=bevel))

    mag_loft("Magazine_CurvedShell", 0.0, 1.0, 0.0235, "navy", slices=19)
    mag_loft("Magazine_UpperRetentionCollar", 0.003, 0.090, 0.0255,
             "dark", hx_extra=0.0015, slices=4, bevel=0.0010)
    mag_loft("Magazine_TopWarmWhiteTrim", 0.093, 0.107, 0.0256,
             "ivory", hx_extra=0.0018, slices=2, bevel=0.0007)

    def curved_strip(name, start, end, xoffset, strip_width, side,
                     y_near, thickness, mat, bevel=0.0008, samples=17):
        left, right = [], []
        for idx in range(samples):
            value = start + (end - start) * idx / (samples - 1)
            center = mag_center(value) + xoffset
            z = mag_z(value)
            left.append((center - strip_width / 2, z))
            right.append((center + strip_width / 2, z))
        y0, y1 = sorted((side * y_near, side * (y_near + thickness)))
        return prism(name, left + list(reversed(right)), y0, y1,
                     mat, bevel=bevel)

    for sign, suffix in [(-1, "L"), (1, "R")]:
        curved_strip("Magazine_RecessedSideField_" + suffix,
                     0.15, 0.92, 0, 0.046, sign, 0.0234, 0.0013,
                     "dark", bevel=0.0023)
        # Four narrow outer raised rails make three deep-looking dark channels.
        for idx, offset in enumerate([-0.021, -0.007, 0.007, 0.021]):
            curved_strip("Magazine_CurvedFluteRidge_" + suffix + "_%d" % idx,
                         0.17, 0.91, offset, 0.0054, sign,
                         0.0246, 0.0019, "teal", bevel=0.0010)
        curved_strip("Magazine_FrontEdgeAccent_" + suffix,
                     0.16, 0.88, 0.0300, 0.0025, sign,
                     0.0219, 0.0018, "steel", bevel=0.0007)
        # Small orange witness apertures sit in their own dark beveled surrounds.
        for idx, value in enumerate([0.36, 0.49, 0.62, 0.75]):
            x, z = mag_center(value) - 0.0068, mag_z(value)
            slope = -(0.076 * 1.65 * value ** 0.65) / 0.223
            angle = -math.atan(slope)
            b("Magazine_WitnessWindowBezel_" + suffix + "_%d" % idx,
              (x, sign * 0.0270, z), (0.0100, 0.0024, 0.0130),
              "navy", 0.0017, (0, angle, 0))
            b("Magazine_OrangeWitnessWindow_" + suffix + "_%d" % idx,
              (x, sign * 0.0285, z), (0.0051, 0.0007, 0.0080),
              "orange", 0.0014, (0, angle, 0))
            b("Magazine_WindowUpperGlint_" + suffix + "_%d" % idx,
              (x + 0.0001, sign * 0.0290, z + 0.0016),
              (0.0034, 0.0003, 0.0011), "ivory", 0.0003,
              (0, angle, 0))
        screw("Magazine", "UpperPanelFixing_" + suffix,
              mag_center(0.135), mag_z(0.135), sign * 0.026, 0.0023)
        screw("Magazine", "LowerPanelFixing_" + suffix,
              mag_center(0.945), mag_z(0.945), sign * 0.025, 0.0023)

    # Raised ribs run across the front and rear curved spines.
    for idx in range(11):
        value = 0.19 + idx * 0.066
        x, z = mag_center(value), mag_z(value)
        tangent = -(0.076 * 1.65 * value ** 0.65) / 0.223
        rotation = (0, -math.atan(tangent), 0)
        for edge, sign in [("Front", 1), ("Rear", -1)]:
            b("Magazine_" + edge + "SpineTraction_%02d" % idx,
              (x + sign * (mag_half_x(value) + 0.0004), 0, z),
              (0.0030, 0.039, 0.0050), "rubber", 0.0014, rotation)

    # Separate layered floor plate includes an orange release tab and corner pads.
    mag_loft("Magazine_FloorPlateUpperSeal", 0.977, 1.023,
             0.0255, "dark", hx_extra=0.0025, slices=3, bevel=0.0015)
    mag_loft("Magazine_FloorPlateWarmWhiteRim", 1.019, 1.038,
             0.0267, "ivory", hx_extra=0.0036, slices=2, bevel=0.0010)
    mag_loft("Magazine_ThickRubberFloorPlate", 1.036, 1.073,
             0.0274, "rubber", hx_extra=0.0045, slices=3, bevel=0.0024)
    b("Magazine_FloorPlateReleaseTab", (mag_center(1.055) + 0.016,
      -0.0287, mag_z(1.052)), (0.018, 0.0035, 0.006),
      "orange", 0.0012, (0, 0.45, 0))
    for idx in range(4):
        b("Magazine_FloorPlateBottomTraction_%02d" % idx,
          (mag_center(1.071) - 0.022 + idx * 0.014, 0, mag_z(1.073) - 0.001),
          (0.0045, 0.044, 0.003), "dark", 0.0010)

    # Readable markings remain editable Text objects before the export pass.
    keep(text("Stock_Marking_CTS", "CTS / 07", (-0.368, -0.0396, 0.0187),
              0.0065, mats["ivory"], rotation=(math.pi / 2, 0, 0)))
    keep(text("Magazine_CapacityMarking", "30", (mag_center(0.14) + 0.012,
              -0.0268, mag_z(0.14) - 0.002), 0.0065, mats["ivory"],
              rotation=(math.pi / 2, 0, 0)))
    return created
