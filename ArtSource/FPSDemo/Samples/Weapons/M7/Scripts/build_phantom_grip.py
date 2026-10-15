"""Phantom rear grip traced from the latest authoritative user side view.

Only ``build(builder)`` creates geometry. It neither saves a source library nor
updates materials, manifests, exports or scenes. The existing Core interface
and RearGrip socket remain fixed; a narrow hidden tongue connects the shorter,
more inclined visible skeleton to the receiver. Depth is inferred from the
earlier three-quarter image, never claimed as manufacturing measurements.
"""
import math

from mathutils import Vector


MOUNT = (-.0625, 0., -.065)
PREFIX = 'PHANTOM_'
REFERENCE_FILE = 'Reference/Phantom_Grip_Side_User.png'
DEPTH_REFERENCE_FILE = 'Reference/Phantom_Grip_User.png'
PIXEL_SCALE = .00110
PIXEL_ORIGIN = (40., 58.)
WORLD_ORIGIN_XZ = (-.045, -.0785)
BODY_LENGTH_SCALE = 1.20
BODY_REAR_LEAN_PER_PIXEL = .52

# Retained as the fixed-interface record, not extruded into an exposed cover.
INTERFACE_XZ = [
    (-.045, -.078), (-.0525, -.0765), (-.075, -.0765),
    (-.085, -.0735), (-.091, -.0625), (-.1015, -.0535),
    (-.114, -.048), (-.123, -.050),
]
CORE_LOWER_INTERFACE_XZ = [(-.1016, -.0673), (-.0635, -.0785), (-.0427, -.0779)]


def p(px, py):
    """Left in the reference is +X; downward in the reference is -Z."""
    lower_pixels = max(0., py-PIXEL_ORIGIN[1])
    rear_shift = (BODY_LENGTH_SCALE-1.)*BODY_REAR_LEAN_PER_PIXEL*lower_pixels
    length_extension = (BODY_LENGTH_SCALE-1.)*lower_pixels
    return (WORLD_ORIGIN_XZ[0]-(px-PIXEL_ORIGIN[0]+rear_shift)*PIXEL_SCALE,
            WORLD_ORIGIN_XZ[1]-(py-PIXEL_ORIGIN[1]+length_extension)*PIXEL_SCALE)


def _points(pixels):
    return [p(*v) for v in pixels]


def _soft_outline(points, distance=.00055, steps=6):
    """Local corner rounding only; retain the traced envelope and hole angles."""
    output = []
    for i, point in enumerate(points):
        a = Vector(points[i-1])
        b = Vector(point)
        c = Vector(points[(i+1) % len(points)])
        incoming, outgoing = a-b, c-b
        if incoming.length < 1e-8 or outgoing.length < 1e-8:
            continue
        trim = min(distance, incoming.length*.20, outgoing.length*.20)
        left = b+incoming.normalized()*trim
        right = b+outgoing.normalized()*trim
        for j in range(steps+1):
            t = j/steps
            output.append(tuple((1-t)**2*left+2*(1-t)*t*b+t*t*right))
    return output


def _side_piece(builder, name, outline, side, y, depth, role, bevel=.00015):
    lo, hi = sorted((side*y, side*(y+depth)))
    return builder.prism(PREFIX+name, outline, lo, hi, role, bevel)


def _beam_outline(start, end, half_width):
    a, b = Vector(start), Vector(end)
    tangent = (b-a).normalized()
    normal = Vector((-tangent.y, tangent.x))*half_width
    return [tuple(a+normal), tuple(b+normal), tuple(b-normal), tuple(a-normal)]


def _pixel_y(z):
    delta = WORLD_ORIGIN_XZ[1]-z
    length_scale = BODY_LENGTH_SCALE if delta > 0. else 1.
    return PIXEL_ORIGIN[1]+delta/(PIXEL_SCALE*length_scale)


def _front_center(z):
    # A slim continuous front grasping pole follows the side-view lean.
    py = _pixel_y(z)
    t = max(0., min(1., (py-64.)/62.))
    px = 44.1+(72.1-44.1)*t+.45*math.sin(math.pi*t)
    return p(px, py)[0]


def _back_center(z):
    py = _pixel_y(z)
    t = max(0., min(1., (py-65.)/60.))
    return p(80.2+(116.0-80.2)*t, py)[0]


def _loft_rod(builder, name, z0, z1, center, radius_x, radius_y,
              role, length_steps=32, radial_steps=64):
    vertices, faces = [], []
    for j in range(length_steps+1):
        t = j/length_steps
        z = z0+(z1-z0)*t
        rx = radius_x(t) if callable(radius_x) else radius_x
        ry = radius_y(t) if callable(radius_y) else radius_y
        for i in range(radial_steps):
            angle = i*math.tau/radial_steps
            vertices.append((center(z)+rx*math.cos(angle), ry*math.sin(angle), z))
    for j in range(length_steps):
        for i in range(radial_steps):
            k = (i+1) % radial_steps
            a, b = j*radial_steps, (j+1)*radial_steps
            faces.append((a+i, a+k, b+k, b+i))
    faces.append(tuple(reversed(range(radial_steps))))
    faces.append(tuple(length_steps*radial_steps+i for i in range(radial_steps)))
    obj = builder.mesh(PREFIX+name, vertices, faces, role, .00013,
                       smooth_faces=True)
    for face in obj.data.polygons[-2:]:
        face.use_smooth = False
    return obj


def _rib_band(builder, name, z, center, rx, ry, role='rubber',
              height=.00058, rise=.00024, radial_steps=64):
    loops = [(-height/2, -.00007), (-height*.30, rise),
             (height*.30, rise), (height/2, -.00007)]
    vertices, faces = [], []
    for dz, extra in loops:
        zz = z+dz
        for i in range(radial_steps):
            angle = i*math.tau/radial_steps
            vertices.append((center(zz)+(rx+extra)*math.cos(angle),
                             (ry+extra)*math.sin(angle), zz))
    for j in range(3):
        for i in range(radial_steps):
            k = (i+1) % radial_steps
            a, b = j*radial_steps, (j+1)*radial_steps
            faces.append((a+i, a+k, b+k, b+i))
    faces.append(tuple(reversed(range(radial_steps))))
    faces.append(tuple(3*radial_steps+i for i in range(radial_steps)))
    obj = builder.mesh(PREFIX+name, vertices, faces, role, smooth_faces=True)
    for face in obj.data.polygons[-2:]:
        face.use_smooth = False
    return obj


def build(builder):
    """Create a compact inclined open-frame grip; return actual hole probes."""
    before = len(builder.objects)

    # The old tall cap is absent. This narrow adapter nests inside Core's
    # +/-23.5 mm casing and follows its actual lower interface. Only the short
    # lower transition reaches the exposed grip; no Core plate is duplicated.
    tongue_outline = [(-.0427, -.0779), (-.03935, -.0673),
                      (-.070, -.0545), (-.103, -.0470), (-.114, -.0480),
                      (-.109, -.0590), (-.1016, -.0673), (-.089, -.0740)]
    tongue_outline += _points([(80,61), (75,66), (46,67), (42,61)])
    tongue = builder.prism(PREFIX+'HiddenCoreInterfaceTongue',
                           _soft_outline(tongue_outline, .00035, 6),
                           -.010, .010, 'polymer')
    tongue['core_hidden_connector'] = True
    tongue['maximum_connector_half_depth_m'] = .010

    neck_pixels = [(44,54), (70,54), (79,61), (77,67), (49,69), (43,63)]
    neck = builder.prism(PREFIX+'ShortExposedShoulder',
                         _soft_outline(_points(neck_pixels), .00055, 7),
                         -.0178, .0178, 'polymer')

    # Traced front and rear edges lean backwards together. The toe turns
    # slightly forward while the underside rises steeply towards the rear.
    silhouette_pixels = [
        (43,56), (72,55), (81,64), (86,78), (96,96), (108,116),
        (118,125), (116,128), (69,142), (67,141), (68,136), (69,126),
        (58,104), (48,80), (41,63),
    ]
    frame = builder.prism(PREFIX+'InclinedOpenSkeleton',
                          _soft_outline(_points(silhouette_pixels), .0006, 7),
                          -.0138, .0138, 'polymer')

    # Two inclined triangular windows have different aspect and orientation.
    # The bottom is divided by a narrow diagonal into a long slim opening and
    # a large skew rhombus, matching the paired columns in the latest image.
    holes = [
        {'name': 'UpperObliqueTriangle',
         'pixels': [(50,66), (69,65), (77,77)],
         'center_px': (65.3333,69.3333)},
        {'name': 'MiddleObliqueTriangle',
         'pixels': [(61,87), (81,81), (94,98)],
         'center_px': (78.6667,88.6667)},
        {'name': 'LowerNarrowDiagonalOpening',
         'pixels': [(67,103), (86.5,102), (79,133)],
         'center_px': (77.5,112.6667)},
        {'name': 'LowerSkewRhombus',
         'pixels': [(91,105), (104,110), (113,123), (82,133)],
         'center_px': (97.5,117.75)},
    ]
    checks = []
    for i, hole in enumerate(holes):
        outline = _soft_outline(_points(hole['pixels']), .00045, 8)
        targets = [frame, neck, tongue] if i == 0 else [frame]
        builder.cut(targets, outline, PREFIX+hole['name'])
        cx, cz = p(*hole['center_px'])
        # The probes sample genuine interior space rather than the boundary,
        # and the caller tests every new mesh, including rods and fine lands.
        radius = .00070 if i == 0 else .00095
        checks.append({'name': PREFIX+hole['name'],
                       'center_m': [cx,0.,cz], 'outline_m': outline,
                       'reference_outline_px': [list(v) for v in hole['pixels']],
                       'probe_points_m': [[cx,0.,cz], [cx+radius,0.,cz],
                                          [cx-radius,0.,cz], [cx,0.,cz+radius],
                                          [cx,0.,cz-radius]],
                       'construction': 'Full-depth actual aperture across all intersecting structural parts'})
    builder.bevel(tongue, .00038)
    builder.bevel(neck, .00046)
    builder.bevel(frame, .00040)

    # Round only the grasping poles. They follow the traced envelope and do not
    # introduce a broad flat side plate or block any of the triangular gaps.
    _loft_rod(builder, 'SlimFrontGraspingPole', p(0,64)[1], p(0,126)[1],
              _front_center, lambda t: .00365-.00035*t,
              lambda t: .0181-.0014*t, 'polymer', 34, 64)
    _loft_rod(builder, 'SlimRearSpine', p(0,65)[1], p(0,125)[1],
              _back_center, .00185, lambda t: .0148-.0006*t,
              'polymer', 32, 64)
    # The side view shows a short dense ribbed grasping band, not horizontal
    # rings over the complete handle height. Keep ribs inside the silhouette.
    for i in range(19):
        py = 93.5+i*1.70
        z = p(0,py)[1]
        t = (py-64.)/62.
        _rib_band(builder, 'FrontAntiSlipRib_%02d'%i, z, _front_center,
                  .00365-.00035*t, .0181-.0014*t,
                  height=.00055, rise=.00020)
    for i in range(15):
        py = 92.5+i*2.10
        t = (py-65.)/60.
        _rib_band(builder, 'RearMicroRib_%02d'%i, p(0,py)[1], _back_center,
                  .00185, .0148-.0006*t, role='polymer',
                  height=.00040, rise=.00013)

    # Narrow lands are seated on the actual web separating adjacent openings.
    # Their geometry describes the reference sloped supports, not extra slabs.
    upper_web = _beam_outline(p(59,79), p(79,78), .00120)
    lower_web = _beam_outline(p(71,98), p(94,101), .00115)
    inner_column = _beam_outline(p(88.8,104), p(81,130), .00140)
    for side in (-1,1):
        tag = 'L' if side < 0 else 'R'
        _side_piece(builder, 'UpperDiagonalLand_'+tag, upper_web, side,
                    y=.0136, depth=.00055, role='coat', bevel=.00011)
        _side_piece(builder, 'MiddleDiagonalLand_'+tag, lower_web, side,
                    y=.0136, depth=.00055, role='coat', bevel=.00011)
        # The lower narrow column is part of the structural frame. The seated
        # bed and two thin inlays only add the long mechanical slot detail.
        _side_piece(builder, 'InnerColumnInsetBed_'+tag, inner_column, side,
                    y=.0136, depth=.00035, role='dark', bevel=.00011)
        for lane in (-1,1):
            offset = lane*.00053
            a, b = p(88.4,106), p(81.7,128)
            a = (a[0]+offset,a[1]); b = (b[0]+offset,b[1])
            _side_piece(builder, 'ThinColumnMetalInlay_%s_%s'%(tag,lane),
                        _beam_outline(a,b,.00020), side,
                        y=.0139, depth=.00024, role='hardware', bevel=.00006)

    # Small slanted heel: its lower edge climbs towards the rear, unlike the
    # rejected broad horizontal bottom frame. It embeds in both end columns.
    heel_pixels = [(72,133), (113,122), (118,125), (116,128),
                   (69,142), (67,141), (68,136)]
    heel = builder.prism(PREFIX+'SlantedCompactHeel',
                         _soft_outline(_points(heel_pixels), .00055, 7),
                         -.0162,.0162,'polymer',.00040)
    # The rubber land follows the rising underside rather than adding a flat
    # sole. It sits on the solid heel band and does not cap the rhombus opening.
    sole = _beam_outline(p(70,139.2),p(115.2,126.5),.00060)
    for side in (-1,1):
        _side_piece(builder, 'InclinedHeelLand_'+('L' if side<0 else 'R'),
                    sole,side,y=.0160,depth=.00045,role='rubber',bevel=.00009)

    objects = builder.objects[before:]
    for obj in objects:
        obj['m7_attachment'] = 'PhantomRearGrip'
        obj['reference_shape'] = 'Side-traced compact inclined skeleton; two upper triangles and paired lower columns with skew rhombus'
    coordinates = [vertex.co for obj in objects for vertex in obj.data.vertices]
    actual_bounds = [[min(v[k] for v in coordinates) for k in range(3)],
                     [max(v[k] for v in coordinates) for k in range(3)]]
    return {
        'asset': 'SM_M7_Grip_Phantom', 'display_name': '幻影后握把',
        'mount_key': 'RearGrip', 'mount_position_m': list(MOUNT),
        'reference_image': REFERENCE_FILE,
        'depth_reference_image': DEPTH_REFERENCE_FILE, 'depth_inferred': True,
        'iteration': 'Latest side-view skeleton; exposed body below y58 lengthened by 20 percent without widening its apertures or moving the core interface',
        'mapping': {'metres_per_pixel_x':PIXEL_SCALE,
                    'metres_per_pixel_z':PIXEL_SCALE,
                    'pixel_origin':list(PIXEL_ORIGIN),
                    'world_origin_xz_m':list(WORLD_ORIGIN_XZ),
                    'body_length_scale':BODY_LENGTH_SCALE,
                    'rear_lean_pixels_per_lower_pixel':BODY_REAR_LEAN_PER_PIXEL,
                    'x_formula':'-.045-(px-40+(BODY_LENGTH_SCALE-1)*.52*max(0,py-58))*.00110',
                    'z_formula':'-.0785-(py-58+(BODY_LENGTH_SCALE-1)*max(0,py-58))*.00110',
                    'calibration':'Visible upper body near reference y58 meets Core actual lower interface z-.0785; the Core crop is not duplicated as a grip cover'},
        'objects':[obj.name for obj in objects], 'through_apertures':checks,
        'ray_scope':'All new Phantom meshes, including hidden adapter, narrow poles, side lands and heel',
        'interface_profile_m':[list(v) for v in INTERFACE_XZ],
        'actual_core_lower_interface_m':[list(v) for v in CORE_LOWER_INTERFACE_XZ],
        'hidden_connector_half_depth_m':.010,
        'visible_silhouette_reference_px':[list(v) for v in silhouette_pixels],
        'visible_body_height_m':p(0,54)[1]-p(0,142)[1],
        'lengthened_lower_body_height_m':(142.-58.)*PIXEL_SCALE*BODY_LENGTH_SCALE,
        'expected_world_bounds_m':actual_bounds,
        'radial_segments':64, 'bevel_segments':8,
        'front_rib_count':19, 'back_rib_count':15,
        'geometry_features':['Short shoulder nesting into unchanged Core through a narrow internal tongue',
                              'Steep backward lean traced with a uniform image scale',
                              'Two distinct upper oblique triangular apertures',
                              'Lower long diagonal opening and skew rhombus between thin structural columns',
                              'Seated long inlays and compact heel with a rising rear underside'],
        'unreal_tested':False, 'visual_acceptance':'pending user review',
    }
