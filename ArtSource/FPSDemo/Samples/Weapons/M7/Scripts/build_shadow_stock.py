"""Shadow rail stock reconstructed from the user's corrected side silhouette.

Reference/ShadowRail_Side_User.png is the shape authority. The earlier
three-quarter image supplies material separation and small mechanical layers.
Importing the module does not modify Blender; build(builder) is called by the
attachment-library generator. Coordinates are assembly world metres, with
Stock=(-.179,0,-.0025) unchanged. These are visual art proportions.
"""
import math
import bpy

MOUNT = (-.179, 0., -.0025)
IMAGE_SCALE = .00066
REFERENCE_IMAGE = 'Reference/ShadowRail_Side_User.png'
DETAIL_REFERENCE = 'Reference/ShadowRail_Stock_User.png'
PIXEL_MOUNT = (29., 91.)
UPPER_BEAM_ANCHORS_PX = [(105,29),(178,45),(270,69),(321,82)]
UPPER_BEAM_ANGLE_DEGREES = math.degrees(math.atan2(82-29,321-105))


def p(px, py):
    return (MOUNT[0]-(px-PIXEL_MOUNT[0])*IMAGE_SCALE,
            MOUNT[2]+(PIXEL_MOUNT[1]-py)*IMAGE_SCALE)


def contour(points):
    return [p(x,z) for x,z in points]


def rounded(points, distance=.0006, segments=4):
    """Round only the corners; the new inclined top stays a straight beam."""
    result=[]
    for i,point in enumerate(points):
        a,b=points[i-1],points[(i+1)%len(points)]
        da=math.hypot(a[0]-point[0],a[1]-point[1])
        db=math.hypot(b[0]-point[0],b[1]-point[1])
        d=min(distance,da*.23,db*.23)
        start=(point[0]+(a[0]-point[0])*d/da,point[1]+(a[1]-point[1])*d/da)
        end=(point[0]+(b[0]-point[0])*d/db,point[1]+(b[1]-point[1])*d/db)
        for j in range(segments+1):
            t=j/segments
            result.append(((1-t)**2*start[0]+2*t*(1-t)*point[0]+t*t*end[0],
                           (1-t)**2*start[1]+2*t*(1-t)*point[1]+t*t*end[1]))
    return result


def circle(x,z,radius,sides=64):
    return [(x+radius*math.cos(i*math.tau/sides),
             z+radius*math.sin(i*math.tau/sides)) for i in range(sides)]


def finish(obj,segments=4):
    # Four bevel subdivisions are sufficient on these small metal edges;
    # 64-sided circular hardware and sampled contour arcs remain smooth.
    for modifier in obj.modifiers:
        if modifier.type=='BEVEL':modifier.segments=segments
    return obj


def prism(builder,name,points,y0,y1,role,bevel=.0004,corner=.0006):
    return finish(builder.prism(name,rounded(contour(points),corner),y0,y1,role,bevel))


def side(builder,name,points,sign,depth,thickness,role='coat',bevel=.0003):
    lo,hi=sorted((sign*depth,sign*(depth+thickness)))
    return prism(builder,name,points,lo,hi,role,bevel,bevel*1.2)


def frame(builder,name,outside,inside,y0,y1,role,bevel=.0004,corner=.0007):
    """Closed annular solid: side panels, interior wall and real opening."""
    assert len(outside)==len(inside),(name,'Matching frame contours required')
    outer=rounded(contour(outside),corner)
    inner=rounded(contour(inside),corner*.7)
    n=len(outer)
    vertices=[(x,y,z) for y,loop in ((y0,outer),(y1,outer),(y0,inner),(y1,inner))
              for x,z in loop]
    faces=[]
    for i in range(n):
        j=(i+1)%n
        faces.extend(((i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),
                      (i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)))
    return finish(builder.mesh(name,vertices,faces,role,bevel))


def difference(builder,target,outline,label,y0=-.08,y1=.08):
    cutter=builder.prism('SHADOW_TEMP_'+label,outline,y0,y1,'dark')
    bpy.context.view_layer.objects.active=target;target.select_set(True)
    modifier=target.modifiers.new('True through opening '+label,'BOOLEAN')
    modifier.operation='DIFFERENCE';modifier.object=cutter
    options={v.identifier for v in modifier.bl_rna.properties['solver'].enum_items}
    modifier.solver='MANIFOLD' if 'MANIFOLD' in options else 'EXACT'
    if hasattr(modifier,'use_self'):modifier.use_self=False
    while list(target.modifiers).index(modifier)>0:
        bpy.ops.object.modifier_move_up(modifier=modifier.name)
    result=bpy.ops.object.modifier_apply(modifier=modifier.name)
    assert 'FINISHED' in result,(target.name,label)
    assert len(target.data.vertices)>=12,(target.name,label,'Damaged solid')
    target.select_set(False);builder.boolean_count+=1
    builder.objects.remove(cutter);mesh=cutter.data
    bpy.data.objects.remove(cutter,do_unlink=True)
    if not mesh.users:bpy.data.meshes.remove(mesh)


def cylinder_z(builder,name,x,z0,z1,radius,role):
    sides=64
    vertices=[(x+radius*math.cos(i*math.tau/sides),
               radius*math.sin(i*math.tau/sides),z)
              for z in (z0,z1) for i in range(sides)]
    faces=[tuple(reversed(range(sides))),tuple(range(sides,2*sides))]
    faces.extend((i,(i+1)%sides,(i+1)%sides+sides,i+sides) for i in range(sides))
    obj=finish(builder.mesh(name,vertices,faces,role,.0002))
    for polygon in obj.data.polygons:polygon.use_smooth=len(polygon.vertices)==4
    return obj


def fastener(builder,name,px,py,sign,depth,radius=.0018):
    x,z=p(px,py);lo,hi=sorted((sign*depth,sign*(depth+.00065)))
    return finish(builder.ring_y(name,x,z,lo,hi,radius,radius*.32,'hardware'),3)


def aperture(name,px,py,objects):
    x,z=p(px,py)
    return {'name':name,'center':[x,0.,z],'direction':[0.,1.,0.],
            'objects':objects,'through':True}


def build(builder):
    start=len(builder.objects);old_booleans=builder.boolean_count;holes=[]

    # The installation collar stays at the same fixed Stock socket. Its short
    # body is visibly separate from the new angled cheek/rail construction.
    adapter=prism(builder,'SHADOW_FrontClampBody',[(30,62),(56,63),(71,71),
        (77,83),(77,115),(69,124),(32,123)],-.016,.016,'coat',.0006)
    difference(builder,adapter,rounded(contour([(38,73),(61,74),(64,109),
        (40,110)]),.0010),'ClampWindow')
    for j,(x0,x1) in enumerate(((28,33),(34,38))):
        a,b=p(x0,65),p(x1,120)
        finish(builder.box('SHADOW_MountUpright_'+str(j),
               ((a[0]+b[0])/2,0.,(a[1]+b[1])/2),
               (abs(a[0]-b[0]),.036,abs(a[1]-b[1])),'hardware',.00035))
    finish(builder.box('SHADOW_MountTongue',(-.1785,0.,-.0030),
                       (.0065,.025,.019),'hardware',.00045))
    pivot_x=p(65,91)[0]
    cylinder_z(builder,'SHADOW_FoldingPivot',pivot_x,p(0,119)[1],p(0,72)[1],
               .0047,'hardware')
    for j,py in enumerate((81,109)):
        z=p(0,py)[1]
        cylinder_z(builder,'SHADOW_PivotLand_'+str(j),pivot_x,z-.00065,z+.00065,
                   .0052,'coat')
    for sign in (-1,1):
        tag='L' if sign<0 else 'R'
        outside=[(35,66),(55,66),(68,75),(71,116),(63,120),(36,118)]
        inside=[(40,74),(54,73),(61,78),(64,109),(59,112),(41,110)]
        frame(builder,'SHADOW_ClampFace_'+tag,outside,inside,
              *sorted((sign*.016,sign*.0182)),'coat',.0003,.00045)
        for j,py in enumerate((81,102)):
            fastener(builder,'SHADOW_ClampBolt_'+tag+str(j),45,py,sign,.0182,.0020)

    # Shape authority: the top descends by 53 pixels across 216 pixels
    # (13.79 degrees), while the bottom is almost level at y=109. A flat
    # parallel rectangle would contradict the corrected user side view.
    beam_outer=[(79,81),(142,81),(148,43),(105,29),(178,45),(270,69),
                (321,82),(323,110),(79,110)]
    rail=prism(builder,'SHADOW_InclinedOpenRailFrame',beam_outer,-.0118,.0118,
                'coat',.00055,.00065)
    window_specs=[
        ('FrontRailWindow',[(92,90),(135,86),(135,99),(126,104),(92,104)],(113,97)),
        ('TaperedMiddleWindow',[(166,59),(252,80),(252,103),(160,103)],(210,95)),
        ('TaperedRearWindow',[(278,88),(310,92),(312,102),(302,104),(278,104)],(295,98)),
    ]
    for name,outline,probe in window_specs:
        difference(builder,rail,rounded(contour(outline),.0010),name)
        holes.append(aperture(name,*probe,['SHADOW_InclinedOpenRailFrame']))
    # The separate slim cheek cover follows the SAME descending top line.
    # This is a raised layer with a tapered front tip, not another level bar.
    prism(builder,'SHADOW_SlopingCheekCover',[(105,29),(178,45),(270,69),
          (317,81),(316,85),(268,75),(176,51),(106,33)],-.0138,.0138,
          'polymer',.00045,.0004)
    for sign in (-1,1):
        tag='L' if sign<0 else 'R'
        side(builder,'SHADOW_LevelLowerGuide_'+tag,[(81,106),(321,106),
             (321,109),(81,109)],sign,.0118,.0010,'hardware',.00014)
        # Front and back locks have their own side-view silhouettes. The
        # taller front clamp follows the elevated cheek; the back clamp is
        # compact at the low rail end. Both contain genuine small openings.
        lever_specs=[
            ('Front',[(147,53),(174,60),(168,108),(144,108)],
                     [(154,73),(164,76),(158,99),(150,100)],(157,86),(160,104)),
            ('Rear',[(263,79),(276,84),(277,112),(255,112),(255,92)],
                    [(264,90),(271,93),(271,104),(261,104),(261,97)],
                    (266,99),(269,108)),
        ]
        for label,outside,inside,probe,bolt in lever_specs:
            name='SHADOW_'+label+'RailLock_'+tag
            frame(builder,name,outside,inside,
                  *sorted((sign*.0128,sign*.0168)),'hardware',.00030,.0006)
            holes.append(aperture(label+'LockWindow_'+tag,*probe,[name]))
            fastener(builder,'SHADOW_'+label+'LockPivot_'+tag,*bolt,sign,.0168,.0013)

    # Rear cheek shell is much shorter than the old three-quarter inference.
    # Its layered facets step down into the narrow curved pad, as in the new
    # silhouette, rather than attaching to a large rectangular head.
    head_points=[(322,84),(332,80),(340,69),(370,68),(385,73),(395,91),
                 (400,119),(397,136),(385,144),(350,144),(328,130),(322,112)]
    head=prism(builder,'SHADOW_RearCheekHousing',head_points,-.0155,.0155,
                'polymer',.00065,.0009)
    difference(builder,head,rounded(contour([(381,88),(389,90),(392,103),
        (389,109),(381,108)]),.0007),'RearHousingWindow')
    holes.append(aperture('RearHousingWindow',386,99,['SHADOW_RearCheekHousing']))
    qx,qz=p(382,122)
    difference(builder,head,circle(qx,qz,.00465),'QD_SlingBore')
    holes.append(aperture('QD_SlingBore',382,122,['SHADOW_RearCheekHousing']))

    tail_outer=[(350,140),(371,138),(392,137),(393,154),(385,173),(376,192),
        (366,208),(355,221),(334,222),(326,215),(329,204),(337,198),
        (343,179),(344,156),(341,148)]
    tail_inner=[(354,152),(367,150),(379,148),(380,158),(373,175),(365,192),
        (358,206),(350,214),(339,215),(335,211),(337,206),(343,200),
        (351,181),(352,160),(350,155)]
    frame(builder,'SHADOW_CurvedOpenTailSupport',tail_outer,tail_inner,
          -.0135,.0135,'polymer',.00065,.0011)
    holes.append(aperture('LongButtpadOpening',357,185,['SHADOW_CurvedOpenTailSupport']))
    contact_path=[(394,132),(400,145),(394,165),(385,185),(374,205),(360,223)]
    rubber_outline=contact_path+[(x+4,y+.2) for x,y in reversed(contact_path)]
    prism(builder,'SHADOW_CurvedRubberButtpad',rubber_outline,-.018,.018,
          'rubber',.0004,.0008)
    for j in range(10):
        u=(j+.6)/10*(len(contact_path)-1)
        idx=min(int(u),len(contact_path)-2);t=u-idx
        a,b=contact_path[idx],contact_path[idx+1]
        px=a[0]+(b[0]-a[0])*t+3.7;py=a[1]+(b[1]-a[1])*t
        dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
        tx,tz=dx/length,dy/length;nx,nz=tz,-tx
        points=[(px+tx*1.0+nx*.7,py+tz*1.0+nz*.7),
                (px-tx*1.0+nx*.7,py-tz*1.0+nz*.7),
                (px-tx*1.0-nx*.7,py-tz*1.0-nz*.7),
                (px+tx*1.0-nx*.7,py+tz*1.0-nz*.7)]
        finish(builder.prism('SHADOW_RubberContactRidge_'+str(j),contour(points),
               -.01825,.01825,'rubber',.00012),3)

    for sign in (-1,1):
        tag='L' if sign<0 else 'R'
        side(builder,'SHADOW_CheekLayer_'+tag,[(331,88),(371,87),(378,94),
             (377,119),(366,134),(347,134),(332,125)],sign,.0155,.0012,
             'polymer',.0004)
        # A shallow vented cheek cap retains the major raised layer seen in
        # the secondary perspective image. Only five wide vents are needed.
        side(builder,'SHADOW_CheekVentWell_'+tag,[(341,72),(369,72),
             (373,80),(336,80)],sign,.0157,.00018,'dark',.00010)
        for j in range(5):
            px=341+j*5.5
            side(builder,'SHADOW_CheekVentRib_'+tag+str(j),[(px+1.4,73),
                 (px+3,73),(px+1,79),(px-.6,79)],sign,.0159,.0010,
                 'coat',.00012)
        lo,hi=sorted((sign*.0155,sign*.0202))
        finish(builder.ring_y('SHADOW_QDSocket_'+tag,qx,qz,lo,hi,
                             .0077,.00475,'hardware'),4)
        # The long narrow outer rib follows the back leg, leaving its opening
        # intact and clearly visible in a true side view.
        side(builder,'SHADOW_TailRearLayer_'+tag,[(380,152),(384,156),
             (378,174),(367,196),(358,213),(351,217),(358,203),(368,182),
             (377,162)],sign,.0135,.0008,'coat',.0003)

    created=builder.objects[start:]
    assert all(o.name.startswith('SHADOW_') for o in created)
    points=[v.co for o in created for v in o.data.vertices]
    bounds={'min':[min(v[k] for v in points) for k in range(3)],
            'max':[max(v[k] for v in points) for k in range(3)]}
    for obj in created:
        obj['m7_accessory']='Shadow Rail Stock'
        obj['m7_reference_scope']='Corrected side silhouette with sloping cheek rail'
    return {'asset':'SM_M7_Stock_ShadowRail','mount':'Stock','mount_world':list(MOUNT),
        'reference':REFERENCE_IMAGE,'secondary_reference':DETAIL_REFERENCE,
        'reference_priority':'Corrected user side image is the silhouette authority',
        'pixel_mapping':{'image_size_px':[417,225],'pixel_mount':list(PIXEL_MOUNT),
            'uniform_metres_per_pixel':IMAGE_SCALE,'world_x_direction':'front +X, rear -X',
            'world_z_direction':'image up +Z'},
        'side_silhouette':{'upper_beam_anchors_px':UPPER_BEAM_ANCHORS_PX,
            'upper_beam_downward_angle_degrees':UPPER_BEAM_ANGLE_DEGREES,
            'lower_beam_anchors_px':[[79,109],[321,109]],
            'rear_shell_outline_px':head_points,'tail_outer_outline_px':tail_outer,
            'construction':'Inclined cheek rail and tapered triangular open frame; '
                'level slender lower guide; two shaped clamps; short layered rear '
                'shell; narrow curved open tail and rubber contact strip'},
        'object_count':len(created),'boolean_operations':builder.boolean_count-old_booleans,
        'raw_world_bounds':bounds,'holes':holes,
        'material_roles':['coat','hardware','polymer','rubber','dark'],
        'verification':'Y-axis rays through named objects for each aperture; '
            'the three beam windows and long tail opening must also stay visibly '
            'open in the whole-asset side render. Check the 13.79-degree upper beam '
            'against the new reference before user delivery.'}
