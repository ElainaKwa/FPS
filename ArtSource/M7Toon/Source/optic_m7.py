"""Restrained LPVO and folded backup sights for the black M7 visual prop.

The scope is a 300 mm long optic with a 30 mm objective radius.  All geometry
is built through the supplied API; this module never changes the live scene.
"""
import math


def build_optic(api):
    if isinstance(api, dict):
        from types import SimpleNamespace
        api = SimpleNamespace(**api)
    objects = []
    scope_z = .127

    def keep(obj):
        objects.append(obj)
        return obj

    def box(name, loc, dims, mat='metal', bevel=.0007):
        return keep(api.box(name, loc, dims, mat, bevel))

    def cyl(name, loc, radius, depth, mat='metal', axis='X', vertices=48):
        return keep(api.cyl(name, loc, radius, depth, mat, axis, vertices))

    def tube(name, loc, radius, inner, depth, mat='metal', axis='X', segments=64):
        return keep(api.tube(name, loc, radius, inner, depth, mat, axis, segments))

    def mesh(name, verts, faces, mat='metal', bevel=.00035):
        obj = keep(api.mesh(name, verts, faces, mat, bevel))
        return obj

    def profile(name, points, width, mat='metal', bevel=.00065):
        return keep(api.profile(name, points, width, mat, bevel))

    def text(name, body, loc, size=.0020, mat='label', rotation=(math.pi/2, 0, 0)):
        return keep(api.text(name, body, loc, size, mat, rotation))

    def smooth_sides(obj):
        if hasattr(obj, 'data') and hasattr(obj.data, 'polygons'):
            for polygon in obj.data.polygons:
                polygon.use_smooth = len(polygon.vertices) == 4
        return obj

    def shell(name, sections, wall=.0020, mat='metal', segments=80):
        """Closed annular loft: separate inner wall and actual open lens mouth."""
        verts = []
        for inward in (False, True):
            for x, radius in sections:
                r = radius - wall if inward else radius
                for i in range(segments):
                    a = math.tau * i / segments
                    verts.append((x, r*math.cos(a), scope_z+r*math.sin(a)))
        n = len(sections)
        offset = n*segments
        faces = []
        for k in range(n-1):
            for i in range(segments):
                j = (i+1) % segments
                faces.append((k*segments+i, k*segments+j, (k+1)*segments+j, (k+1)*segments+i))
                faces.append((offset+k*segments+i, offset+(k+1)*segments+i,
                              offset+(k+1)*segments+j, offset+k*segments+j))
        for i in range(segments):
            j = (i+1) % segments
            faces.append((i, offset+i, offset+j, j))
            a = (n-1)*segments
            faces.append((a+i, a+j, offset+a+j, offset+a+i))
        return smooth_sides(mesh(name, verts, faces, mat, .0004))

    def radial_ribs(name, center, radius, height, width, length,
                    count, axis='X', mat='polymer', bevel=.00018):
        """One mesh of fine raised axial grip ribs, rather than large blocks."""
        verts, faces = [], []
        angular = width / radius
        for i in range(count):
            mid = math.tau*i/count
            start = len(verts)
            for a in (-length/2, length/2):
                for r, t in ((radius, mid-angular/2), (radius, mid+angular/2),
                             (radius+height, mid+angular/2), (radius+height, mid-angular/2)):
                    u, v = r*math.cos(t), r*math.sin(t)
                    p = (a,u,v) if axis=='X' else ((u,a,v) if axis=='Y' else (u,v,a))
                    verts.append(tuple(center[k]+p[k] for k in range(3)))
            for face in ((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)):
                faces.append(tuple(start+j for j in face))
        return mesh(name, verts, faces, mat, bevel)

    def ticks(name, x, radius, count, length=.0025, mat='label'):
        # Fine axial scale marks around the magnification collar.
        return radial_ribs(name, (x,0,scope_z), radius, .00014,
                           .00036, length, count, 'X', mat, .000035)

    # Characteristic narrow middle tube, large forward bell, shorter ocular.
    shell('Optic_LPVO_ContinuousHousing', [
        (-.155,.0245),(-.150,.0250),(-.112,.0250),(-.106,.0242),
        (-.100,.0230),(-.083,.0228),(-.076,.0190),(-.068,.0182),
        (.068,.0182),(.080,.0190),(.091,.0215),(.106,.0260),
        (.121,.0298),(.137,.0300),(.145,.0297)], .0020)
    tube('Optic_Ocular_RubberRim',(-.1524,0,scope_z),.0251,.0227,.0048,'rubber')
    tube('Optic_Ocular_InternalBaffle',(-.146,0,scope_z),.0229,.0211,.0060,'dark')
    tube('Optic_Ocular_InnerEdge',(-.1502,0,scope_z),.0225,.0218,.0012,'edge')
    cyl('Optic_Ocular_TransparentGlass',(-.1494,0,scope_z),.0217,.0008,'lens')
    # No bright cyan disc: only the supplied low-saturation optical material.
    tube('Optic_Objective_Rim',(.1413,0,scope_z),.03015,.0278,.0068,'polymer')
    tube('Optic_Objective_InnerLip',(.1407,0,scope_z),.0279,.02715,.0015,'edge')
    tube('Optic_Objective_DarkBaffle',(.1346,0,scope_z),.0278,.0262,.006,'dark')
    cyl('Optic_Objective_TransparentGlass',(.1370,0,scope_z),.0269,.0010,'lens',vertices=80)
    tube('Optic_Objective_ShallowGlassSeal',(.1377,0,scope_z),.0274,.0268,.0013,'rubber')
    # Eyepiece focus band with fine grooves and narrow sealing rings.
    tube('Optic_Eyepiece_FocusBand',(-.132,0,scope_z),.02515,.0247,.0280,'polymer')
    radial_ribs('Optic_Eyepiece_FineGrip',(-.132,0,scope_z),.0251,.00065,.00125,.0245,40,'X','polymer')
    for i,x in enumerate((-.1466,-.1168,-.1080)):
        tube('Optic_Eyepiece_FocusSeal_%02d'%i,(x,0,scope_z),.0253,.02465,.0012,'dark')
    tube('Optic_Magnification_RotatingCollar',(-.0918,0,scope_z),.0239,.0227,.0200,'polymer')
    radial_ribs('Optic_Magnification_SoftGrip',(-.0918,0,scope_z),.0239,.00055,.00115,.0160,36,'X','rubber')
    tube('Optic_Magnification_GraduationBand',(-.0795,0,scope_z),.0229,.02245,.0050,'metal')
    ticks('Optic_Magnification_FineTicks',-.0795,.02302,36,.0022)
    ticks('Optic_Magnification_MajorTicks',-.0789,.02302,9,.0038)
    text('Optic_Magnification_SideDigits','1   2   4   6',(-.1030,-.0247,.128),.0018)
    # Small throw lever, sized as an actual collar tab, with a rounded end.
    throw = box('Optic_Magnification_ThrowLever',(-.0910,.0115,.1530),(.009,.0055,.0145),'metal',.0011)
    if hasattr(throw,'rotation_euler'):
        throw.rotation_euler[0] = -.25
    cyl('Optic_ThrowLever_End',(-.091,.0132,.1593),.0033,.009,'rubber',axis='X',vertices=32)
    # Central adjustment block grows only slightly from the narrow optical tube.
    box('Optic_ErectorHousing',(.006,0,.1335),(.033,.0360,.0230),'metal',.0030)
    cyl('Optic_Elevation_Base',(.006,0,.1485),.0165,.0050,'metal','Z')
    cyl('Optic_Elevation_Drum',(.006,0,.1605),.0134,.0190,'metal','Z')
    tube('Optic_Elevation_ZeroRing',(.006,0,.1699),.0145,.0131,.0040,'edge','Z')
    cyl('Optic_Elevation_Cap',(.006,0,.1760),.0148,.0080,'polymer','Z')
    radial_ribs('Optic_Elevation_CapKnurl',(.006,0,.1760),.0148,.00055,.00065,.0060,48,'Z','polymer')
    cyl('Optic_Elevation_TopInset',(.006,0,.18025),.0118,.00055,'metal','Z')
    text('Optic_Elevation_TopEtching','UP  0.1',(-.0018,-.0048,.1807),.0020,'label',(0,0,0))
    # Tiny top ticks form a scale instead of a large white emblem.
    for i in range(12):
        a = math.tau*i/12
        mark = box('Optic_Elevation_TopTick_%02d'%i,
                   (.006+.0102*math.cos(a),.0102*math.sin(a),.1807),
                   (.00165,.00024,.00014),'label',.000025)
        if hasattr(mark,'rotation_euler'):
            mark.rotation_euler[2] = a
    # Side windage and illumination controls sit close to the body.
    for side in (-1,1):
        letter = 'L' if side<0 else 'R'
        cyl('Optic_Windage_%s_Base'%letter,(.006,side*.0201,.1344),.0142,.0060,'metal','Y')
        cyl('Optic_Windage_%s_Cap'%letter,(.006,side*.0240,.1344),.0131,.0055,'polymer','Y')
        radial_ribs('Optic_Windage_%s_FineKnurl'%letter,(.006,side*.0240,.1344),.0131,.00045,.00060,.0042,40,'Y','polymer')
        cyl('Optic_Windage_%s_FaceInset'%letter,(.006,side*.0270,.1344),.0109,.0007,'metal','Y')
        cyl('Optic_Windage_%s_CenterInset'%letter,(.006,side*.02755,.1344),.0025,.0005,'dark','Y',vertices=32)
    text('Optic_Windage_LeftMicroMark','L  0.1',(-.0015,-.0277,.1373),.0017)
    text('Optic_Tube_ManufacturingMark','LPVO  1-6x',(-.064,-.01865,.1280),.0020)
    text('Optic_Tube_SerialMark','O-6071',(.029,-.01865,.1240),.0016,'edge')
    # Double ring cantilever mount, with real open space below the tube.
    box('Optic_Mount_RailFoot',(-.011,0,.0710),(.134,.0360,.0060),'metal',.0011)
    box('Optic_Mount_ClampingLip',(-.011,0,.0700),(.126,.0410,.0034),'dark',.00065)
    side_shape=[(-.078,.074),(.056,.074),(.056,.083),(.047,.100),
                (.043,.108),(.034,.108),(.027,.101),(.023,.086),
                (-.027,.086),(-.029,.104),(-.035,.108),(-.044,.108),
                (-.046,.084),(-.078,.084)]
    for side in (-1,1):
        p=profile('Optic_Mount_CantileverSide_%s'%('L' if side<0 else 'R'),
                  side_shape,.0033,'metal',.0009)
        if hasattr(p,'location'):
            p.location.y=side*.0160
    for i,x in enumerate((-.037,.039)):
        tube('Optic_Mount_Ring_%02d'%i,(x,0,scope_z),.0219,.01815,.0130,'metal')
        # Side ears are tiny flanges, not oversized turret blocks.
        for side in (-1,1):
            letter='L' if side<0 else 'R'
            box('Optic_Mount_RingEar_%02d_%s'%(i,letter),
                (x,side*.0216,.1285),(.0130,.0055,.0085),'metal',.0006)
            box('Optic_Mount_RingSeam_%02d_%s'%(i,letter),
                (x,side*.02445,.1285),(.0128,.00035,.0005),'dark',.00006)
            for j,dx in enumerate((-.0038,.0038)):
                cyl('Optic_Mount_RingBolt_%02d_%s_%02d'%(i,letter,j),
                    (x+dx,side*.0247,.1320),.0017,.0012,'steel','Y',vertices=12)
                cyl('Optic_Mount_RingSocket_%02d_%s_%02d'%(i,letter,j),
                    (x+dx,side*.0254,.1320),.00085,.0003,'dark','Y',vertices=6)
    for i,x in enumerate((-.059,.037)):
        cyl('Optic_Mount_ClampBolt_%02d'%i,(x,-.0230,.0760),.0038,.0052,'metal','Y',vertices=12)
        cyl('Optic_Mount_ClampSocket_%02d'%i,(x,-.0258,.0760),.0017,.0006,'dark','Y',vertices=6)
        cyl('Optic_Mount_ClampOpposite_%02d'%i,(x,.0210,.0760),.0027,.0015,'steel','Y',vertices=12)
    # Lens caps lie folded back along the top, preserving the slender silhouette.
    for label,x,r,z,hinge in (
        ('Objective',.111,.0245,.1603,.139),
        ('Ocular',-.132,.0185,.1547,-.153)):
        cyl('Optic_%s_FoldedCover'%label,(x,0,z),r,.0021,'polymer','Z',vertices=64)
        tube('Optic_%s_CoverBorder'%label,(x,0,z+.0011),r+.0002,r-.0014,.0008,'rubber','Z')
        cyl('Optic_%s_CoverHinge'%label,(hinge,0,z-.0020),.0025,.0180,'polymer','Y',vertices=32)
        box('Optic_%s_HingeTab'%label,((hinge+x)/2,0,z-.0007),
            (abs(hinge-x),.0075,.0028),'polymer',.00065)
        box('Optic_%s_CoverLatch'%label,(x,0,z+.0017),(.0075,.0030,.0018),'rubber',.0005)
    # Compact folded iron sights on the receiver and near the handguard nose.
    box('SightRear_RailFoot',(-.153,0,.0668),(.028,.024,.0036),'metal',.00065)
    cyl('SightRear_Hinge',(-.162,0,.0696),.0026,.022,'metal','Y',vertices=32)
    profile('SightRear_FoldedLeaf',[(-.162,.0698),(-.141,.0698),(-.138,.0720),
            (-.140,.0760),(-.149,.0760),(-.159,.0734)],.014,'metal',.0007)
    tube('SightRear_FoldedAperture',(-.145,0,.0760),.0045,.0027,.0014,'dark','Z',segments=40)
    for side in (-1,1):
        box('SightRear_ProtectiveEar_%s'%side,(-.146,side*.0088,.0728),(.016,.0026,.0062),'metal',.00065)
        cyl('SightRear_HingePin_%s'%side,(-.162,side*.0119,.0696),.0017,.0010,'steel','Y',vertices=12)
    box('SightFront_RailFoot',(.480,0,.0667),(.025,.023,.0034),'metal',.0006)
    cyl('SightFront_Hinge',(.472,0,.0693),.0026,.0215,'metal','Y',vertices=32)
    profile('SightFront_FoldedPost',[ (.471,.0695),(.491,.0695),(.493,.0712),
            (.489,.0754),(.484,.0762),(.475,.0725)],.0080,'metal',.0007)
    box('SightFront_PostBlade',(.486,0,.0766),(.0019,.0021,.0032),'dark',.00035)
    for side in (-1,1):
        ear=profile('SightFront_FoldedEar_%s'%side,[(.478,.0705),(.491,.0715),
                    (.490,.0762),(.484,.0775),(.479,.0750)],.0028,'metal',.0005)
        if hasattr(ear,'location'):
            ear.location.y=side*.0065
        cyl('SightFront_HingePin_%s'%side,(.472,side*.0117,.0693),.0017,.0010,'steel','Y',vertices=12)
    return objects
