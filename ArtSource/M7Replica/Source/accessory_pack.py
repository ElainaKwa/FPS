"""External M7 accessory meshes matched to the supplied game references.

All coordinates are assembled world coordinates in metres: +X is the muzzle,
+Z is up.  ``begin_asset`` assigns each accessory to an independent root and
collection.  This module does not operate on Blender directly or alter the
base rifle.  Open mouths, sight apertures, brake ports and the K1 grip window
are actual mesh openings rather than dark overlays.
"""

import math


ASSET_SPECS = (
    ("SM_M7_Optic_LPVO", "Optic", "Optic", False),
    ("SM_M7_Optic_Combat", "Optic", "Optic", False),
    ("SM_M7_Foregrip_K1", "Foregrip", "Foregrip", False),
    ("SM_M7_Foregrip_Vertical", "Foregrip", "Foregrip", False),
    ("SM_M7_Muzzle_Standard", "Muzzle", "Muzzle", True),
    ("SM_M7_Muzzle_Brake", "Muzzle", "Muzzle", False),
    ("SM_M7_Muzzle_Suppressor", "Muzzle", "Muzzle", False),
    ("SM_M7_Sights_Iron", "Sights", "Iron", True),
)


def build_accessory_pack(api):
    """Build eight separately owned external assets, return their objects."""
    if not isinstance(api, dict):
        api = {key: getattr(api, key) for key in
               ("box", "cyl", "tube", "mesh", "profile", "text", "begin_asset")}
    made = []
    prefix = ""

    def begin(index):
        nonlocal prefix
        name, category, mount, default = ASSET_SPECS[index]
        api["begin_asset"](name, category, mount, is_default=default)
        prefix = "GEO_" + name[3:] + "_"

    def keep(obj):
        made.append(obj)
        return obj

    def B(name, loc, dims, mat="metal", bevel=.00045):
        return keep(api["box"](prefix + name, loc, dims, mat, bevel))

    def C(name, loc, radius, depth, mat="metal", axis="X", vertices=48):
        return keep(api["cyl"](prefix + name, loc, radius, depth, mat, axis, vertices))

    def T(name, loc, radius, inner, depth, mat="metal", axis="X", segments=64):
        return keep(api["tube"](prefix + name, loc, radius, inner, depth,
                               mat, axis, segments))

    def M(name, verts, faces, mat="metal", bevel=.0003, smooth=False):
        obj = keep(api["mesh"](prefix + name, verts, faces, mat, bevel))
        if smooth and hasattr(obj, "data") and hasattr(obj.data, "polygons"):
            for polygon in obj.data.polygons:
                polygon.use_smooth = len(polygon.vertices) == 4
        return obj

    def P(name, points, width, mat="metal", bevel=.00045):
        return keep(api["profile"](prefix + name, points, width, mat, bevel))

    def label(name, body, loc, size=.0014, mat="label", rotation=(math.pi / 2, 0, 0)):
        return keep(api["text"](prefix + name, body, loc, size, mat, rotation))

    def screw(name, loc, radius=.0018, axis="Y"):
        C(name + "_Head", loc, radius, .0011, "steel", axis, 12)
        sign = 1 if loc["XYZ".index(axis)] >= 0 else -1
        out = list(loc)
        out["XYZ".index(axis)] += sign * .00063
        C(name + "_HexSocket", tuple(out), radius * .46, .00022, "dark", axis, 6)

    def axial_ribs(name, center, radius, height, width, length, count,
                   axis="X", mat="polymer", phase=0):
        """Fine tangential teeth in one mesh, with neither large blocks nor caps."""
        verts, faces = [], []
        angular = width / radius
        for i in range(count):
            mid = math.tau * i / count + phase
            base = len(verts)
            for a in (-length / 2, length / 2):
                for r, t in ((radius, mid - angular / 2),
                             (radius, mid + angular / 2),
                             (radius + height, mid + angular / 2),
                             (radius + height, mid - angular / 2)):
                    u, v = r * math.cos(t), r * math.sin(t)
                    point = (a, u, v) if axis == "X" else (
                        (u, a, v) if axis == "Y" else (u, v, a))
                    verts.append(tuple(center[k] + point[k] for k in range(3)))
            for face in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                         (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)):
                faces.append(tuple(base + j for j in face))
        return M(name, verts, faces, mat, .00009)

    def shell(name, sections, center_z, wall=.0015, mat="metal", segments=80):
        """Longitudinal annular loft with open front and rear mouths."""
        verts = []
        for inward in (False, True):
            for x, radius in sections:
                r = radius - wall if inward else radius
                for i in range(segments):
                    a = math.tau * i / segments
                    verts.append((x, r * math.cos(a), center_z + r * math.sin(a)))
        n = len(sections)
        offset = n * segments
        faces = []
        for k in range(n - 1):
            for i in range(segments):
                j = (i + 1) % segments
                a, b = k * segments + i, k * segments + j
                c, d = (k + 1) * segments + j, (k + 1) * segments + i
                faces.extend(((a, b, c, d), (offset + a, offset + d,
                                            offset + c, offset + b)))
        for i in range(segments):
            j = (i + 1) % segments
            end = (n - 1) * segments
            faces.extend(((i, offset + i, offset + j, j),
                          (end + i, end + j, offset + end + j, offset + end + i)))
        return M(name, verts, faces, mat, .00022, True)

    def contour_ring(name, outer, inner, width, mat="metal", y=0, bevel=.0005):
        """A real through-hole ring in the XZ plane; outlines have equal counts."""
        assert len(outer) == len(inner)
        n = len(outer)
        verts = []
        for yy in (y - width / 2, y + width / 2):
            verts.extend((x, yy, z) for x, z in outer)
            verts.extend((x, yy, z) for x, z in inner)
        faces = []
        for i in range(n):
            j = (i + 1) % n
            faces.extend(((i, j, n + j, n + i),
                          (2 * n + i, 3 * n + i, 3 * n + j, 2 * n + j),
                          (i, 2 * n + i, 2 * n + j, j),
                          (n + i, n + j, 3 * n + j, 3 * n + i)))
        return M(name, verts, faces, mat, bevel)

    def cross_ring(name, x0, x1, z0, outer, inner, mat="metal", bevel=.0004):
        """Open hood along X, outer/inner are matched YZ outlines."""
        n = len(outer)
        assert n == len(inner)
        verts = []
        for x in (x0, x1):
            verts.extend((x, y, z0 + z) for y, z in outer)
            verts.extend((x, y, z0 + z) for y, z in inner)
        faces = []
        for i in range(n):
            j = (i + 1) % n
            faces.extend(((i, j, n + j, n + i),
                          (2 * n + i, 3 * n + i, 3 * n + j, 2 * n + j),
                          (i, 2 * n + i, 2 * n + j, j),
                          (n + i, n + j, 3 * n + j, 3 * n + i)))
        return M(name, verts, faces, mat, bevel)

    def rect_outline(w, h, corner):
        return [(-w + corner, -h), (w - corner, -h), (w, -h + corner),
                (w, h - corner), (w - corner, h), (-w + corner, h),
                (-w, h - corner), (-w, -h + corner)]

    def rail_clamp(name, x, length, top=.0615, width=.030):
        # Actual rail jaws form an open inverted U below the foot.
        B(name + "_Foot", (x, 0, top - .002), (length, width, .004), "metal", .00065)
        for side in (-1, 1):
            B(name + "_Jaw_%s" % side, (x, side * (width / 2 - .0015), top - .005),
              (length - .006, .004, .006), "metal", .00055)
        for i, xx in enumerate((x - length * .31, x + length * .31)):
            C(name + "_CrossBolt_%s" % i, (xx, 0, top - .003), .0016,
              width + .003, "steel", "Y", 32)
            screw(name + "_Clamp_%s" % i, (xx, -width / 2 - .002, top - .003), .0025)

    # LPVO: restrained 250 mm long tubular optic, two low scope rings and a
    # continuous cantilever foot, matched to Official_Garena_M7.jpg.
    begin(0)
    z = .100
    shell("Housing", [(-.140, .0215), (-.136, .0220), (-.113, .0220),
                      (-.108, .0218), (-.092, .0218), (-.086, .0180),
                      (-.074, .0154), (.060, .0154), (.071, .0170),
                      (.078, .0195), (.083, .0204), (.105, .0204),
                      (.110, .0200)], z, .00165)
    T("Ocular_RubberRim", (-.1377, 0, z), .0223, .0196, .0043, "rubber")
    T("Ocular_InternalBaffle", (-.131, 0, z), .0197, .0188, .009, "dark")
    C("Ocular_Glass", (-.133, 0, z), .0187, .00065, "lens", vertices=80)
    T("Objective_FrontLip", (.1075, 0, z), .0206, .0186, .0043, "polymer")
    T("Objective_RecessedBaffle", (.1015, 0, z), .0187, .0177, .008, "dark")
    C("Objective_Glass", (.1048, 0, z), .01765, .00065, "lens", vertices=80)
    T("Objective_GlassSeal", (.1054, 0, z), .01835, .0177, .001, "rubber")
    T("Focus_Band", (-.123, 0, z), .0221, .0216, .020, "polymer")
    axial_ribs("Focus_FineGrip", (-.123, 0, z), .0220, .0003,
               .0010, .018, 44, mat="polymer")
    for i, x in enumerate((-.1348, -.1115)):
        T("Focus_Seam_%s" % i, (x, 0, z), .02225, .02165, .0007, "dark")
    T("Zoom_Collar", (-.099, 0, z), .0224, .0215, .021, "polymer")
    axial_ribs("Zoom_Knurl", (-.099, 0, z), .02235, .00045,
               .0011, .017, 40, mat="rubber")
    T("Zoom_GraduationBand", (-.0867, 0, z), .0189, .0183, .0053, "metal")
    axial_ribs("Zoom_IndexTicks", (-.0867, 0, z), .0189, .00012,
               .0003, .0026, 32, mat="label")
    label("Zoom_Digits", "1  2  4  6", (-.108, -.0227, .1008), .00145)
    B("Zoom_ThrowTab", (-.096, .0088, .1243), (.006, .0042, .0115), "metal", .00085)
    C("Zoom_ThrowTabGrip", (-.096, .0088, .1297), .0024, .0068, "rubber", "X", 32)
    B("Erector_Block", (-.006, 0, .1025), (.031, .030, .020), "metal", .0022)
    C("Elevation_Seat", (-.006, 0, .1137), .0120, .0046, "metal", "Z")
    C("Elevation_Drum", (-.006, 0, .1203), .0115, .010, "metal", "Z")
    T("Elevation_ScaleBand", (-.006, 0, .1247), .01185, .0111, .0028, "edge", "Z")
    C("Elevation_Cap", (-.006, 0, .1290), .0121, .0055, "polymer", "Z")
    axial_ribs("Elevation_Knurl", (-.006, 0, .1290), .0121, .00038,
               .00065, .0042, 40, "Z")
    C("Elevation_TopInset", (-.006, 0, .13195), .0097, .0004, "metal", "Z")
    label("Elevation_Etching", "UP  0.1", (-.012, -.003, .13225), .00155,
          rotation=(0, 0, 0))
    for i in range(16):
        a = math.tau * i / 16
        tick = B("Elevation_Tick_%02d" % i,
                 (-.006 + .0084 * math.cos(a), .0084 * math.sin(a), .1323),
                 (.0011, .00018, .00012), "label", .000025)
        if hasattr(tick, "rotation_euler"):
            tick.rotation_euler[2] = a
    for side in (-1, 1):
        C("Windage_Seat_%s" % side, (-.006, side * .0165, .1030), .0113, .005, "metal", "Y")
        C("Windage_Cap_%s" % side, (-.006, side * .0205, .1030), .0108, .0055, "polymer", "Y")
        axial_ribs("Windage_Knurl_%s" % side, (-.006, side * .0205, .1030),
                   .0108, .00038, .00065, .004, 40, "Y")
        C("Windage_Inset_%s" % side, (-.006, side * .0235, .1030), .0087, .00055, "metal", "Y")
        C("Windage_Index_%s" % side, (-.006, side * .0239, .1030), .0022, .00025, "dark", "Y", 32)
    label("Windage_MicroMark", "L  0.1", (-.012, -.0241, .1060), .00125)
    label("Optic_Mark", "LPVO  1-6x", (.080, -.0207, .0995), .0015)
    rail_clamp("Mount", -.006, .114, .0620, .030)
    P("Mount_CantileverWeb", [(-.063, .062), (.051, .062), (.051, .068),
                              (.042, .083), (.034, .083), (.029, .074),
                              (-.027, .074), (-.031, .082), (-.040, .082),
                              (-.046, .067), (-.063, .067)], .021, "metal", .0008)
    for i, x in enumerate((-.036, .038)):
        T("Mount_Ring_%s" % i, (x, 0, z), .0184, .0154, .0125, "metal")
        for side in (-1, 1):
            B("Mount_RingEar_%s_%s" % (i, side), (x, side * .0181, .100),
              (.0125, .0058, .0061), "metal", .00045)
            B("Mount_RingSeam_%s_%s" % (i, side), (x, side * .0211, .1000),
              (.0120, .00022, .00036), "dark", .00004)
            for j, dx in enumerate((-.0032, .0032)):
                screw("Mount_RingBolt_%s_%s_%s" % (i, side, j),
                      (x + dx, side * .0215, .1022), .00135)

    # Compact combat optic with circular front/rear windows and the small top
    # auxiliary hood visible in the game's loadout reference.
    begin(1)
    z = .093
    shell("TubularHousing", [(-.020, .0190), (-.017, .0205), (-.010, .0205),
                             (-.007, .0190), (.057, .0190), (.062, .0210),
                             (.072, .0210), (.075, .0198)], z, .0016)
    for name, x, radius in (("Ocular", -.0176, .0208), ("Objective", .0716, .0212)):
        T(name + "_Rim", (x, 0, z), radius, radius - .0023, .0044, "polymer")
        T(name + "_Baffle", (x - (.003 if x > 0 else -.003), 0, z),
          radius - .0024, radius - .0034, .004, "dark")
        C(name + "_Glass", (x - (.0015 if x > 0 else -.0015), 0, z),
          radius - .0035, .0006, "lens", vertices=64)
    T("FrontSeal", (.0628, 0, z), .02115, .0206, .0012, "rubber")
    T("RearFocusBand", (-.0115, 0, z), .0208, .0203, .009, "polymer")
    axial_ribs("RearFocusGrip", (-.0115, 0, z), .0208, .0003, .0009, .0073, 40)
    B("CentralAdjustmentSeat", (.022, 0, .094), (.025, .0385, .022), "metal", .0025)
    C("Windage_Base", (.026, -.022, .094), .0108, .006, "metal", "Y")
    C("Windage_Dial", (.026, -.0260, .094), .0098, .0045, "polymer", "Y")
    axial_ribs("Windage_DialGrip", (.026, -.0260, .094), .0098, .0004,
               .00065, .0036, 36, "Y")
    C("Windage_CoinFace", (.026, -.0285, .094), .0071, .0007, "edge", "Y")
    B("Windage_CoinSlot", (.026, -.0290, .094), (.008, .00035, .0009), "dark", .0001)
    C("TopDial_Base", (.020, 0, .114), .0078, .0035, "metal", "Z")
    C("TopDial_Cap", (.020, 0, .1173), .0075, .003, "polymer", "Z")
    axial_ribs("TopDial_Grip", (.020, 0, .1173), .0075, .0003, .0006, .0022, 32, "Z")
    B("AuxHood_Seat", (.050, 0, .1125), (.026, .023, .004), "metal", .0005)
    cross_ring("AuxHood_OpenShroud", .039, .058, .126,
               rect_outline(.0115, .013, .003), rect_outline(.0077, .0087, .002),
               "polymer", .00065)
    cross_ring("AuxHood_FrontLip", .0575, .059, .126,
               rect_outline(.0116, .0131, .003), rect_outline(.0076, .0086, .002),
               "edge", .00025)
    # Thin optical pane rather than filling the top shroud with solid material.
    vv = [(x, y, .126 + zz) for x in (.0565, .0570)
          for y, zz in rect_outline(.0075, .0085, .002)]
    ff = [tuple(reversed(range(8))), tuple(range(8, 16))]
    ff.extend((i, (i + 1) % 8, (i + 1) % 8 + 8, i + 8) for i in range(8))
    M("AuxHood_Glass", vv, ff, "glass", 0)
    rail_clamp("Mount", .026, .067, .0615, .028)
    P("Mount_Saddle", [(-.005, .0615), (.057, .0615), (.057, .0690),
                       (.048, .0770), (.006, .0770), (-.005, .0700)], .024, "metal", .0007)
    for i, x in enumerate((.003, .055)):
        T("Mount_Ring_%s" % i, (x, 0, z), .0219, .0189, .008, "metal")
        for side in (-1, 1):
            screw("Mount_RingPin_%s_%s" % (i, side), (x, side * .0223, .090), .0015)
    label("Body_MicroMark", "COMBAT", (.043, -.0201, .091), .00145)
    label("Body_Serial", "TR15", (-.006, -.0201, .089), .00135, "edge")

    # K1 Elite Bevel: a slim oblique front grip and a genuine triangular window.
    begin(2)
    outer = [(.292, -.052), (.385, -.052), (.392, -.059), (.390, -.077),
             (.371, -.119), (.361, -.132), (.345, -.131), (.324, -.110),
             (.310, -.083), (.292, -.078)]
    inner = [(.307, -.066), (.372, -.066), (.375, -.071), (.371, -.079),
             (.353, -.110), (.347, -.116), (.341, -.113), (.329, -.094),
             (.315, -.075), (.307, -.073)]
    contour_ring("OpenGripFrame", outer, inner, .021, "polymer", bevel=.0011)
    for side in (-1, 1):
        contour_ring("BevelSidePlate_%s" % side, outer, inner, .0011,
                     "metal", side * .0107, .0005)
    B("RailShoe", (.338, 0, -.0500), (.089, .022, .007), "metal", .00075)
    for side in (-1, 1):
        B("RailJaw_%s" % side, (.338, side * .0117, -.047),
          (.077, .0033, .007), "metal", .0006)
        for i, (x, zz) in enumerate(((.301, -.060), (.381, -.064),
                                    (.360, -.124), (.347, -.125), (.317, -.080))):
            screw("SideFastener_%s_%s" % (side, i), (x, side * .0120, zz), .00175)
    # Fine teeth follow the front slope; maximum width remains under 102 mm.
    for i in range(10):
        t = (i + .5) / 10
        x, zz = .388 - .027 * t, -.077 - .050 * t
        rib = B("FrontGripRib_%02d" % i, (x + .0010, 0, zz),
                (.0038, .0235, .00165), "rubber", .0003)
        if hasattr(rib, "rotation_euler"):
            rib.rotation_euler[1] = -.49
    T("RearSlingEye_Rim", (.300, 0, -.0848), .005, .0027, .024, "metal", "Y", 40)
    # The eye itself is an annulus, leaving its center genuinely open.
    for side in (-1, 1):
        T("RearSlingEye_Collar_%s" % side, (.300, side * .0124, -.0848),
          .0048, .0027, .0010, "dark", "Y", 40)
    label("SmallModelMark", "K1", (.327, -.0115, -.065), .0018, "edge")

    # Compact vertical foregrip, 100 mm from its rail interface to the cap.
    begin(3)
    B("RailFoot", (.325, 0, -.0495), (.040, .027, .009), "metal", .00085)
    for side in (-1, 1):
        B("RailClamp_%s" % side, (.325, side * .0138, -.0468),
          (.032, .004, .008), "metal", .0006)
        screw("RailClampBolt_%s" % side, (.325, side * .0161, -.0490), .0021)
    sections = [(-.052, .0100), (-.057, .0120), (-.066, .0145),
                (-.075, .0152), (-.139, .0152), (-.147, .0156), (-.155, .0135)]
    n = 64
    verts = []
    for zz, radius in sections:
        for i in range(n):
            a = math.tau * i / n
            verts.append((.325 + radius * math.cos(a), radius * .86 * math.sin(a), zz))
    faces = [tuple(reversed(range(n))), tuple(range((len(sections) - 1) * n, len(sections) * n))]
    for k in range(len(sections) - 1):
        for i in range(n):
            j = (i + 1) % n
            faces.append((k * n + i, k * n + j, (k + 1) * n + j, (k + 1) * n + i))
    M("TaperedGripBody", verts, faces, "polymer", .0006, True)
    # Elliptical anti-slip bands follow the housing, represented as subtle ribs.
    for k in range(11):
        zz = -.079 - k * .0056
        vv = []
        for zlev, radius in ((zz - .00055, .01518), (zz + .00055, .01518),
                             (zz - .00055, .01555), (zz + .00055, .01555)):
            for i in range(n):
                a = math.tau * i / n
                vv.append((.325 + radius * math.cos(a), radius * .86 * math.sin(a), zlev))
        ff = []
        for i in range(n):
            j = (i + 1) % n
            ff.extend(((i, j, n + j, n + i), (2 * n + i, 3 * n + i, 3 * n + j, 2 * n + j),
                       (i, 2 * n + i, 2 * n + j, j), (n + i, n + j, 3 * n + j, 3 * n + i)))
        M("AntiSlipBand_%02d" % k, vv, ff, "rubber", .00012, True)
    C("BottomCap", (.325, 0, -.1548), .0115, .0015, "rubber", "Z")
    screw("BottomSocket", (.325, 0, -.1557), .0026, "Z")
    label("ModelMark", "VFG", (.320, -.0137, -.072), .0014, "edge")

    # Default bare muzzle cap, sized directly from the official side image.
    begin(4)
    shell("KnurledThreadCap", [(.592, .0106), (.593, .0114), (.609, .0114),
                              (.612, .0109)], 0, .0030, "metal", 64)
    T("RearSeal", (.5930, 0, 0), .0115, .0085, .0015, "dark")
    T("MouthLip", (.6113, 0, 0), .0111, .0078, .0014, "edge")
    # Alternating narrow chevrons give a real diamond knurl without changing
    # the cap into a blocky muzzle device.
    knurl_v, knurl_f = [], []
    for row in range(5):
        xx = .595 + row * .0027
        for i in range(40):
            a = math.tau * (i + .5 * (row % 2)) / 40
            base = len(knurl_v)
            for dx, da, r in ((-.0011, 0, .01148), (0, -.038, .01148),
                              (.0011, 0, .01148), (0, .038, .01148),
                              (0, 0, .0117)):
                knurl_v.append((xx + dx, r * math.cos(a + da), r * math.sin(a + da)))
            knurl_f.extend((base + u, base + v, base + 4)
                          for u, v in ((0, 1), (1, 2), (2, 3), (3, 0)))
            knurl_f.append((base + 3, base + 2, base + 1, base))
    M("DiamondKnurl", knurl_v, knurl_f, "metal", .000015)

    # Compact slotted muzzle brake.  The radial lattice includes the interior
    # bore, port edge walls and annular ends, so each vent opens all the way in.
    begin(5)
    T("RearSocket", (.604, 0, 0), .0125, .0073, .009, "metal")
    T("RearCollar", (.6088, 0, 0), .0139, .0114, .0035, "edge", segments=32)
    xs = [.6105, .6140, .6235, .6270, .6372, .6428, .6460]
    n = 64
    ro, ri = .0140, .0075
    vv = []
    for radius in (ro, ri):
        for xx in xs:
            for i in range(n):
                a = math.tau * i / n
                vv.append((xx, radius * math.cos(a), radius * math.sin(a)))
    offset = len(xs) * n
    # Four symmetric directions, with two side ports in each direction.
    port_angles = {i % n for c in (0, 16, 32, 48) for i in range(c - 3, c + 3)}
    solid = [[not (k in (1, 3) and i in port_angles) for i in range(n)]
             for k in range(len(xs) - 1)]
    ff = []
    for k in range(len(xs) - 1):
        for i in range(n):
            if not solid[k][i]:
                continue
            j = (i + 1) % n
            a, b, c, d = k * n + i, k * n + j, (k + 1) * n + j, (k + 1) * n + i
            ff.extend(((a, b, c, d), (offset + a, offset + d, offset + c, offset + b)))
            if k == 0 or not solid[k - 1][i]:
                ff.append((a, offset + a, offset + b, b))
            if k == len(xs) - 2 or not solid[k + 1][i]:
                ff.append((d, c, offset + c, offset + d))
            if not solid[k][(i - 1) % n]:
                ff.append((a, d, offset + d, offset + a))
            if not solid[k][j]:
                ff.append((b, offset + b, offset + c, c))
    M("PortedBrakeBody", vv, ff, "metal", .00022, True)
    T("ExitLip", (.6454, 0, 0), .01415, .00735, .00125, "edge")
    axial_ribs("RearWrenchKnurl", (.6046, 0, 0), .0125, .00025,
               .0011, .0050, 24, mat="metal")

    # M7 Practical Suppressor: slim continuous barrel, faceted rear adapter and
    # a dark recessed open muzzle with a narrow protective face rim.
    begin(6)
    T("Adapter_InnerSocket", (.608, 0, 0), .0170, .0073, .0160, "dark")
    T("Adapter_FacetedCollar", (.6110, 0, 0), .0235, .0157, .0200,
      "metal", segments=12)
    T("Adapter_BackBevel", (.6018, 0, 0), .0217, .0125, .0035,
      "metal", segments=12)
    shell("ContinuousHousing", [(.6170, .0216), (.6210, .0223), (.6270, .0230),
                                (.7350, .0230), (.7430, .0225), (.7480, .0205)],
          0, .0030, "metal", 96)
    T("RearBody_Seal", (.6260, 0, 0), .02315, .0228, .0012, "dark")
    T("FrontBody_Seam", (.7398, 0, 0), .02295, .0224, .0010, "dark")
    T("Muzzle_ProtectiveLip", (.7468, 0, 0), .0210, .0095, .0024, "metal")
    T("Muzzle_InternalBaffle", (.7400, 0, 0), .0127, .0064, .0105, "dark")
    T("Muzzle_InnerEdge", (.7452, 0, 0), .0094, .0064, .0015, "edge")
    # The open bore remains visible; no solid black plug is inserted.
    for i in range(12):
        a = math.tau * (i + .5) / 12
        # Thin raised edges reinforce the actual facets of the adapter.
        verts = []
        for xx in (.6042, .6178):
            for rr, aa in ((.02335, a - .012), (.02335, a + .012),
                           (.02360, a + .012), (.02360, a - .012)):
                verts.append((xx, rr * math.cos(aa), rr * math.sin(aa)))
        faces = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
                 (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        M("Adapter_FacetEdge_%02d" % i, verts, faces, "edge", .00004)
    label("Body_ModelMark", "M7  PRACTICAL", (.657, -.02315, .0012), .00135, "edge")
    label("Body_SerialMark", "7.62  /  00817", (.665, -.02315, -.0020), .0010, "edge")
    # Six small face recesses, restrained like the official front rim.
    for i in range(6):
        a = math.tau * i / 6
        C("Muzzle_FaceRecess_%02d" % i, (.74805, .0176 * math.cos(a), .0176 * math.sin(a)),
          .0014, .00025, "dark", "X", 24)

    # Default upright iron sights.  Position and height are taken from the
    # 2000x1000 official side render; p(px,py)=((1250-px)*.0005,(380-py)*.0005).
    begin(7)
    def p(px, py):
        return ((1250 - px) * .0005, (380 - py) * .0005)

    def pix_profile(name, coords, width, mat="metal", bevel=.00045):
        return P(name, [p(px, py) for px, py in coords], width, mat, bevel)

    pix_profile("Front_RailShoe", [(171, 284), (245, 284), (245, 274),
                                 (232, 262), (181, 262), (171, 276)], .022, bevel=.0006)
    pix_profile("Front_HingeArm", [(191, 267), (228, 267), (221, 250),
                                 (217, 230), (201, 230), (201, 249)], .012, bevel=.00065)
    x, zz = p(210, 207)
    T("Front_ProtectiveRing", (x, 0, zz), .0141, .01045, .0068, "metal", "X", 80)
    T("Front_RingFrontEdge", (x + .0034, 0, zz), .01415, .0105, .0009, "edge", "X", 80)
    # Slender post rising inside the protective ring with open air on both sides.
    B("Front_PostBlade", (x + .001, 0, zz - .0068), (.0024, .0018, .0085), "steel", .0002)
    C("Front_PostAdjuster", (x + .001, 0, zz - .0110), .0031, .0042, "metal", "Z", 24)
    hx, hz = p(201, 263)
    C("Front_HingeAxle", (hx, 0, hz), .0030, .026, "metal", "Y", 40)
    for side in (-1, 1):
        screw("Front_HingePin_%s" % side, (hx, side * .0136, hz), .0023)
        screw("Front_RailBolt_%s" % side, (p(226, 0)[0], side * .0118, p(0, 277)[1]), .0018)

    pix_profile("Rear_RailShoe", [(1357, 284), (1441, 284), (1441, 274),
                                (1419, 257), (1374, 257), (1357, 272)], .024, bevel=.00065)
    pix_profile("Rear_HingeArm", [(1390, 265), (1420, 265), (1420, 244),
                                (1416, 228), (1400, 228), (1390, 246)], .012, bevel=.0007)
    # Upright thin leaf with a round peep hole, ray-mapped outer rectangle.
    # These meshes leave the peep aperture completely open along the gun axis.
    x, aperture_z = p(1405, 205)
    n = 64
    verts = []
    low, high, halfwidth, aperture = -.0108, .0110, .0075, .0035
    for xx in (x - .0030, x + .0030):
        for inner in (False, True):
            for i in range(n):
                a = math.tau * i / n
                yy, zz = math.cos(a), math.sin(a)
                if inner:
                    r = aperture
                else:
                    candidates = [halfwidth / max(abs(yy), 1e-8)]
                    candidates.append((high if zz > 0 else -low) / max(abs(zz), 1e-8))
                    r = min(candidates)
                verts.append((xx, r * yy, aperture_z + r * zz))
    faces = []
    for i in range(n):
        j = (i + 1) % n
        faces.extend(((i, j, n + j, n + i),
                      (2 * n + i, 3 * n + i, 3 * n + j, 2 * n + j),
                      (i, 2 * n + i, 2 * n + j, j),
                      (n + i, n + j, 3 * n + j, 3 * n + i)))
    M("Rear_UprightApertureLeaf", verts, faces, "metal", .0005)
    T("Rear_PeepLip", (x + .0032, 0, aperture_z), .0048, .0035, .0008, "edge", "X", 64)
    hx, hz = p(1416, 259)
    C("Rear_HingeAxle", (hx, 0, hz), .004, .027, "metal", "Y", 40)
    for side in (-1, 1):
        screw("Rear_HingePin_%s" % side, (hx, side * .0144, hz), .0030)
        C("Rear_WindageWheel_%s" % side, (p(1397, 0)[0], side * .0105, .090),
          .0043, .0025, "metal", "Y", 40)
        axial_ribs("Rear_WindageKnurl_%s" % side,
                   (p(1397, 0)[0], side * .0105, .090), .0043, .00022,
                   .0006, .0020, 24, "Y", "metal")
        screw("Rear_RailBolt_%s" % side,
              (p(1378, 0)[0], side * .0128, p(0, 277)[1]), .002)
    return made
