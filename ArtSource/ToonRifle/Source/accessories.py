"""Editable external accessories for the original stylized VELA rifle.

Coordinates use metres, +X toward the muzzle and +Z upward.  The sight hood
is a true open annulus, with an independent thin transparent optical pane.
No real weapon internals are represented.
"""

import math


def build_accessories(api):
    box = api["box"]
    cyl = api["cyl"]
    tube = api["tube"]
    mesh = api["mesh"]
    m = api["mats"]
    made = []

    def add(obj):
        made.append(obj)
        return obj

    def B(name, loc, dims, material, bevel=.001):
        return add(box("ACC_" + name, loc, dims, m[material], bevel=bevel))

    def C(name, loc, radius, depth, material, axis="X", vertices=40):
        return add(cyl("ACC_" + name, loc, radius, depth, m[material],
                       axis=axis, vertices=vertices))

    def T(name, loc, radius, inner, depth, material, axis="X", segments=48):
        return add(tube("ACC_" + name, loc, radius, inner, depth, m[material],
                        axis=axis, segments=segments))

    def annulus(name, x0, x1, center_z, outer, inner, material, bevel=.001):
        """Extruded chamfered rectangle ring, open along the X view axis."""
        def outline(width, height, corner):
            return [(-width + corner, -height), (width - corner, -height),
                    (width, -height + corner), (width, height - corner),
                    (width - corner, height), (-width + corner, height),
                    (-width, height - corner), (-width, -height + corner)]
        out = outline(*outer)
        ins = outline(*inner)
        verts = []
        for x in (x0, x1):
            verts += [(x, y, center_z + z) for y, z in out]
            verts += [(x, y, center_z + z) for y, z in ins]
        faces = []
        for i in range(8):
            j = (i + 1) % 8
            faces += [(i, j, 8 + j, 8 + i),
                      (16 + i, 24 + i, 24 + j, 16 + j),
                      (i, 16 + i, 16 + j, j),
                      (8 + i, 8 + j, 24 + j, 24 + i)]
        return add(mesh("ACC_" + name, verts, faces, m[material], bevel=bevel))

    def screw(name, x, y, z, axis="Y", radius=.0032):
        C(name + "_Head", (x, y, z), radius, .0018, "steel", axis)
        if axis == "Y":
            B(name + "_Slot", (x, y - .0011 if y < 0 else y + .0011, z),
              (radius * 1.22, .0005, .00065), "dark", .00012)
        elif axis == "X":
            B(name + "_Slot", (x + .0011, y, z),
              (.0005, radius * 1.22, .00065), "dark", .00012)
        else:
            B(name + "_Slot", (x, y, z + .0011),
              (radius * 1.22, .00065, .0005), "dark", .00012)

    # Two-level rail mount, metal clamp jaws and a quick-release lever.
    B("Optic_RailSaddle", (0, 0, .110), (.117, .047, .016), "steel", .002)
    B("Optic_LowerDeck", (0, 0, .121), (.108, .057, .011), "navy", .002)
    B("Optic_UpperDeck", (.003, 0, .132), (.097, .051, .012), "teal", .002)
    for side in (-1, 1):
        B("Optic_ClampJaw_%s" % side, (0, side * .029, .111),
          (.082, .008, .015), "navy", .0018)
        for x in (-.028, .028):
            screw("Optic_ClampBolt_%s_%s" % (side, x), x,
                  side * .034, .111, radius=.004)
    B("Optic_ReleasePivotPlate", (-.028, -.039, .119),
      (.018, .014, .016), "steel", .002)
    C("Optic_ReleasePivot", (-.028, -.047, .119), .005, .003, "orange", "Y")
    lever = B("Optic_ReleaseLever", (.002, -.042, .116),
              (.064, .009, .008), "ivory", .002)
    lever.rotation_euler[2] = math.radians(-5)
    B("Optic_ReleaseGrip", (.031, -.045, .116),
      (.015, .012, .010), "rubber", .002)
    for i in range(3):
        B("Optic_ReleaseGripRib_%02d" % i, (.026 + i * .004, -.0515, .116),
          (.001, .001, .008), "dark", .0002)

    # Main housing and a thick rounded polygon hood with a genuine opening.
    B("Optic_ElectronicsBody", (-.005, 0, .143),
      (.106, .062, .020), "navy", .003)
    B("Optic_RearLedge", (-.050, 0, .148), (.022, .062, .010), "ivory", .002)
    B("Optic_FrontLedge", (.053, 0, .143), (.023, .062, .014), "navy", .002)
    annulus("Optic_OpenHood", -.020, .029, .174,
            (.036, .043, .012), (.0275, .0335, .009), "navy", .0017)
    annulus("Optic_ForwardRim", .0285, .032, .174,
            (.0364, .0434, .012), (.0273, .0333, .009), "ivory", .0008)
    annulus("Optic_RearRim", -.023, -.0195, .174,
            (.036, .043, .012), (.0282, .0342, .009), "teal", .0008)
    annulus("Optic_InnerLensGasket", .0268, .0285, .174,
            (.0286, .0346, .009), (.0268, .0328, .0085), "rubber", .00035)

    # Independent glass lens: the surrounding housing is never filled solid.
    lens_outline = [(-.0185, -.032), (.0185, -.032), (.0265, -.024),
                    (.0265, .024), (.0185, .032), (-.0185, .032),
                    (-.0265, .024), (-.0265, -.024)]
    lens_verts = [(x, y, .174 + z) for x in (.026, .0267)
                  for y, z in lens_outline]
    lens_faces = [tuple(reversed(range(8))), tuple(range(8, 16))]
    lens_faces += [(i, (i + 1) % 8, (i + 1) % 8 + 8, i + 8) for i in range(8)]
    add(mesh("ACC_Optic_CyanLens", lens_verts, lens_faces, m["glass"], bevel=0))
    # A small emissive painted aiming cue remains visible without blocking view.
    C("Optic_AimDot", (.0254, 0, .174), .0007, .00018, "emission", "X", 24)
    for side in (-1, 1):
        B("Optic_HoodSideInlay_%s" % side, (.003, side * .0368, .185),
          (.028, .0012, .010), "teal", .001)
        for x in (-.010, .023):
            screw("Optic_HoodScrew_%s_%s" % (side, x), x,
                  side * .0373, .157, radius=.0023)

    # Side windage dial, raised hash marks and contrasting index pointer.
    C("Optic_DialSeat", (.029, -.038, .151), .013, .010, "rubber", "Y")
    C("Optic_WindageDial", (.029, -.0465, .151), .0115, .012, "ivory", "Y")
    C("Optic_DialCenter", (.029, -.053, .151), .0068, .002, "navy", "Y")
    B("Optic_DialCoinSlot", (.029, -.0542, .151), (.008, .0006, .0014), "steel", .0002)
    for i in range(20):
        angle = i * math.tau / 20
        mark = B("Optic_DialHash_%02d" % i,
                 (.029 + .0095 * math.sin(angle), -.053,
                  .151 + .0095 * math.cos(angle)),
                 (.00075, .0007, .0020 if i % 5 == 0 else .0012),
                 "orange" if i == 0 else "navy", .00012)
        mark.rotation_euler[1] = angle
    B("Optic_DialIndex", (.029, -.039, .166), (.0025, .002, .004), "orange", .0004)
    for i, z in enumerate((.146, .162)):
        B("Optic_ControlPad_%02d" % i, (-.039, -.033, z),
          (.015, .004, .010), "rubber", .0015)
        B("Optic_ControlSymbolHorizontal_%02d" % i, (-.039, -.0353, z),
          (.006, .0005, .00085), "ivory", .00015)
        if i == 1:
            B("Optic_ControlSymbolVertical", (-.039, -.0353, z),
              (.00085, .0005, .006), "ivory", .00015)

    # Original three-bar VELA emblem, built in geometry rather than a decal.
    for i in range(3):
        emblem = B("Optic_OriginalEmblem_%02d" % i, (-.021 + i * .007, .032, .143),
                   (.003, .0009, .009 - i * .002), "ivory", .0003)
        emblem.rotation_euler[1] = math.radians(-25)
    C("Optic_StatusLED", (.041, -.033, .143), .0017, .001, "emission", "Y", 24)

    # Rear iron sight is folded flat. Its aperture ring and hinge are separate
    # editable pieces, suitable for later animation of a stow/deploy action.
    B("RearIron_RailShoe", (-.118, 0, .108), (.054, .047, .014), "navy", .002)
    B("RearIron_HingeYoke", (-.139, 0, .122), (.014, .039, .017), "ivory", .002)
    C("RearIron_HingePin", (-.139, 0, .124), .005, .047, "steel", "Y")
    for side in (-1, 1):
        C("RearIron_HingeCap_%s" % side, (-.139, side * .025, .124),
          .0065, .004, "teal", "Y")
    B("RearIron_FoldedArm", (-.121, 0, .130), (.037, .012, .010), "navy", .0015)
    T("RearIron_PeepRing", (-.102, 0, .135), .0125, .0066, .007,
      "ivory", "Z", 40)
    T("RearIron_PeepInner", (-.102, 0, .139), .008, .0066, .0015,
      "dark", "Z", 40)
    B("RearIron_FoldLatch", (-.109, -.022, .116), (.017, .007, .009), "rubber", .001)
    B("RearIron_LatchMark", (-.109, -.026, .116), (.008, .0006, .002), "orange", .0002)
    for x in (-.131, -.109):
        screw("RearIron_Mount_%s" % x, x, .025, .108, radius=.0027)

    # Matching folded front sight: low guard wings and a central blade/post.
    B("FrontIron_RailShoe", (.441, 0, .108), (.064, .046, .014), "navy", .002)
    B("FrontIron_HingeBlock", (.419, 0, .120), (.018, .037, .014), "ivory", .002)
    C("FrontIron_HingePin", (.419, 0, .123), .005, .046, "steel", "Y")
    for side in (-1, 1):
        C("FrontIron_HingeCap_%s" % side, (.419, side * .024, .123),
          .0065, .003, "teal", "Y")
        B("FrontIron_FoldedGuard_%s" % side, (.440, side * .016, .130),
          (.043, .007, .017), "ivory", .002)
        B("FrontIron_GuardInlay_%s" % side, (.444, side * .020, .130),
          (.018, .001, .005), "navy", .0005)
    B("FrontIron_FoldedBlade", (.444, 0, .129), (.044, .008, .010), "navy", .0015)
    B("FrontIron_PostTip", (.464, 0, .130), (.006, .009, .012), "teal", .0012)
    B("FrontIron_PostMark", (.4675, 0, .130), (.001, .004, .007), "orange", .0005)
    screw("FrontIron_Mount", .442, -.025, .108, radius=.003)

    # Compact side accessory: a sealed fictional illuminator/rangefinder.
    B("SideModule_RailClamp", (.370, -.052, .053),
      (.064, .013, .025), "steel", .002)
    B("SideModule_IsolationPad", (.371, -.063, .053),
      (.071, .011, .029), "rubber", .002)
    B("SideModule_Housing", (.376, -.080, .054),
      (.087, .039, .037), "navy", .004)
    B("SideModule_TopShell", (.376, -.080, .074),
      (.069, .034, .007), "ivory", .0025)
    B("SideModule_LeadingShoulder", (.416, -.080, .054),
      (.012, .037, .033), "teal", .0025)
    T("SideModule_OpticBezel", (.424, -.081, .057),
      .0095, .0064, .007, "ivory", "X")
    T("SideModule_OpticGasket", (.428, -.081, .057),
      .0068, .0055, .0018, "rubber", "X")
    C("SideModule_CyanLens", (.4282, -.081, .057), .0053, .0008, "glass", "X")
    C("SideModule_Sensor", (.4232, -.082, .041), .0028, .001, "emission", "X", 24)
    B("SideModule_ButtonSurround", (.366, -.101, .057),
      (.030, .005, .015), "teal", .002)
    B("SideModule_Button", (.366, -.104, .057),
      (.022, .003, .010), "rubber", .0018)
    B("SideModule_ButtonMark", (.366, -.106, .057),
      (.007, .0007, .0015), "ivory", .0002)
    for i in range(4):
        B("SideModule_CoolingRib_%02d" % i, (.387 + i * .0046, -.100, .049),
          (.0018, .0015, .013), "steel", .00035)
    for i in range(3):
        B("SideModule_RearAccent_%02d" % i, (.339 + i * .0033, -.100, .056),
          (.0012, .001, .010 - i * .002), "orange", .00025)
    for x in (.343, .412):
        screw("SideModule_CaseScrew_%s" % x, x, -.101, .066,
              radius=.0023)
    B("SideModule_TopSwitch", (.355, -.081, .079),
      (.015, .013, .004), "rubber", .001)
    B("SideModule_SwitchIndex", (.355, -.081, .0815),
      (.004, .006, .001), "orange", .0003)
    B("SideModule_EndCap", (.331, -.080, .054),
      (.005, .031, .030), "rubber", .002)
    return made
