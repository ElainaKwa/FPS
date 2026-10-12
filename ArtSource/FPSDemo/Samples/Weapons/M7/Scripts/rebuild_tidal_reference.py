"""Replace only the Tidal visual front assembly from the approved side reference.

Run in the material sample's Blender source, then rebake this asset.  Dimensions
are inferred art proportions; the image is a perspective view, not a blueprint.
Importing this module does not change the scene.
"""
import bpy
import bmesh
import json
import math
import sys
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[1]
ASSET = "SM_M7_Barrel_Tidal"
REFERENCE = ROOT / 'Reference/Tidal_User.png'
if not REFERENCE.is_file():
    REFERENCE = Path("C:/Users/97853/AppData/Local/Temp/codex-clipboard-3848e7d6-224c-46f8-8bf3-636a3e6c3b87.png")
SCALE_X = .055 / 89
SCALE_Z = SCALE_X
LOWER_SCALE_Z = .050 / 67
BARREL_EXTENSION = 0.
HANDGUARD_EXTENSION = .120
MID_BODY_BULGE = .0060
FLANK_CROWN = .0035
UPPER_NECK_HALF_WIDTH = .0105
LOWER_EDGE_HALF_WIDTH = .0230
MID_ZONE_Z = -.0015
SHOULDER_BREAK_Z = .0125
SHOULDER_ROLL_INSET = .0015
MOUNT = (.158, 0., 0.)


def p(px, py):
    # Side reference: the bore is nearly midway between the two rail planes.
    # One X/upper-Z scale removes the previous 21% horizontal stretch.  The
    # lower silhouette uses the new side view's depth, rather than its old
    # perspective foreshortening.  This is an art reconstruction, not a scan.
    base=MOUNT[0]+(886-px)*SCALE_X
    # Insert length through the rear/middle body; translate the whole front
    # nose by the extension without stretching its approved local arc.
    offset=HANDGUARD_EXTENSION*smooth(0.,436.,886-px)
    return base+offset,(260-py)*(SCALE_Z if py<=260 else LOWER_SCALE_Z)


def body_envelope(x):
    # The user wants the middle band equally full over its entire length.
    return 1.


def mid_bulge(x):
    return MID_BODY_BULGE*body_envelope(x)


def side_surface(x,z,wall=0.):
    # The upper vent tier is a fixed narrow neck. Only the central flank
    # receives the longitudinal swell/crown, rolling back inward above and
    # below the crest rather than widening the complete cross-section.
    crest=.0275-wall+(MID_BODY_BULGE+FLANK_CROWN)*body_envelope(x)
    if z>=MID_ZONE_Z:
        if z<=SHOULDER_BREAK_Z:
            u=(z-MID_ZONE_Z)/(SHOULDER_BREAK_Z-MID_ZONE_Z)
            return crest-SHOULDER_ROLL_INSET*u*u
        # A deliberate slope change separates the round central flank from
        # the straight triangular shoulder. It is not an all-smooth oval.
        t=max(0.,min(1.,(z-SHOULDER_BREAK_Z)/(p(0,185)[1]-SHOULDER_BREAK_Z)))
        crest-=SHOULDER_ROLL_INSET
        edge=UPPER_NECK_HALF_WIDTH-wall
    else:
        t=smooth(0.,MID_ZONE_Z-p(0,318)[1],MID_ZONE_Z-z)
        edge=LOWER_EDGE_HALF_WIDTH-wall
    return crest+(edge-crest)*t


def sampled_round_rect(x0, x1, z0, z1, radius, arc_steps=8, straight_steps=18):
    arcs = []
    for cx, cz, start in ((x1-radius,z1-radius,0),(x0+radius,z1-radius,90),
                          (x0+radius,z0+radius,180),(x1-radius,z0+radius,270)):
        arcs.append([(cx+radius*math.cos(math.radians(start+i*90/arc_steps)),
                      cz+radius*math.sin(math.radians(start+i*90/arc_steps)))
                     for i in range(arc_steps+1)])
    contour = []
    for i, arc in enumerate(arcs):
        contour.extend(arc)
        a, b = arc[-1], arcs[(i+1)%4][0]
        contour.extend([(a[0]+(b[0]-a[0])*j/straight_steps,
                         a[1]+(b[1]-a[1])*j/straight_steps)
                        for j in range(1,straight_steps)])
    return contour


def front_x(z):
    """Outward-convex front connection, with a shared rounded crest tangent.

    Analytic convex quarters prevent the previous concave S-shaped transition
    caused by sparse interpolation controls. The crest and end X anchors stay
    fixed; only the local connecting silhouette changes.
    """
    top_z,crest_z,bottom_z=p(0,185)[1],p(0,252)[1],p(0,321)[1]
    top_x,crest_x,bottom_x=p(348,0)[0],p(305.5,0)[0],p(372,0)[0]
    if z>=top_z:return top_x
    if z<=bottom_z:return bottom_x
    if z>=crest_z:
        t=(top_z-z)/(top_z-crest_z)
        return top_x+(crest_x-top_x)*math.sin(math.pi*t/2)
    t=(crest_z-z)/(crest_z-bottom_z)
    return crest_x-(crest_x-bottom_x)*(1-math.cos(math.pi*t/2))


def smooth(a, b, value):
    t = max(0., min(1., (value - a) / (b - a)))
    return t * t * (3 - 2 * t)


def rounded_rect(x0, x1, z0, z1, radius, segments=8, shear=0.):
    """Counterclockwise round rectangle in X/Z, with a slanted vertical edge."""
    radius = min(radius, (x1 - x0) / 2, (z1 - z0) / 2)
    result = []
    for cx, cz, start in ((x1-radius, z1-radius, 0),
                          (x0+radius, z1-radius, 90),
                          (x0+radius, z0+radius, 180),
                          (x1-radius, z0+radius, 270)):
        for i in range(segments + 1):
            angle = math.radians(start + i * 90 / segments)
            x, z = cx + radius * math.cos(angle), cz + radius * math.sin(angle)
            result.append((x + shear * (z - (z0+z1)/2), z))
    return result


class Builder:
    def __init__(self, collection, root, materials):
        self.collection, self.root, self.materials = collection, root, materials
        self.objects, self.boolean_count = [], 0

    def mesh(self, name, vertices, faces, role, bevel=0., smooth_faces=False):
        data = bpy.data.meshes.new(name + "_Geometry")
        data.from_pydata(vertices, [], faces)
        data.update()
        bm = bmesh.new(); bm.from_mesh(data)
        # Dense sampled rings can coincide once stored as float32. Clean the
        # actual raw Boolean input, before the exact solver sees zero edges.
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
        bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=1e-7)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        if name=='TIDAL_HandguardShell':
            assert not any(len(e.link_faces)!=2 for e in bm.edges),'Raw shell must be a closed solid before apertures'
        bm.to_mesh(data); bm.free()
        obj = bpy.data.objects.new(name, data)
        self.collection.objects.link(obj)
        obj.parent = self.root
        obj.matrix_parent_inverse = Matrix.Identity(4)
        obj.matrix_basis = Matrix.Identity(4)
        data.materials.append(self.materials[role])
        for face in data.polygons:
            face.use_smooth = smooth_faces
        # Only newly authored data receives this provisional UV.  The atlas
        # baker replaces it for this asset without touching the other 16.
        uv = data.uv_layers.new(name="UVMap")
        for loop in data.loops:
            co = data.vertices[loop.vertex_index].co
            uv.data[loop.index].uv = ((co.x - .15) / .68, (co.z + .055) / .12)
        uv.active_render = True
        self.objects.append(obj)
        if bevel:
            self.bevel(obj, bevel)
        return obj

    @staticmethod
    def bevel(obj, width):
        edge = obj.modifiers.new("Fine rounded machined edges", "BEVEL")
        edge.width = width; edge.segments = 8; edge.limit_method = 'ANGLE'
        edge.angle_limit = math.radians(34)
        edge.use_clamp_overlap = True; edge.harden_normals = True
        normal = obj.modifiers.new("Broad surface normals", "WEIGHTED_NORMAL")
        normal.keep_sharp = True; normal.weight = 50

    def prism(self, name, outline, y0, y1, role, bevel=0.):
        n = len(outline)
        verts = [(x, y, z) for y in (y0, y1) for x, z in outline]
        faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
        faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
        return self.mesh(name, verts, faces, role, bevel)

    def box(self, name, center, dims, role, bevel=.0004):
        x, y, z = center; dx, dy, dz = [v/2 for v in dims]
        return self.prism(name, [(x-dx,z-dz),(x+dx,z-dz),(x+dx,z+dz),(x-dx,z+dz)],
                          y-dy, y+dy, role, bevel)

    def tube(self, name, x0, x1, radius, inner, role, z=0., sides=64):
        vertices = []
        for x, r in ((x0,radius),(x1,radius),(x0,inner),(x1,inner)):
            vertices += [(x,r*math.cos(i*math.tau/sides),z+r*math.sin(i*math.tau/sides)) for i in range(sides)]
        faces=[]
        for i in range(sides):
            j=(i+1)%sides
            faces += [(i,j,sides+j,sides+i),(2*sides+i,3*sides+i,3*sides+j,2*sides+j),
                      (i,2*sides+i,2*sides+j,j),(sides+i,sides+j,3*sides+j,3*sides+i)]
        obj=self.mesh(name,vertices,faces,role,.00022)
        for i,face in enumerate(obj.data.polygons):face.use_smooth=i%4 in (0,1)
        return obj

    def ring_y(self, name, x, z, y0, y1, outer, inner, role):
        n=64;verts=[];faces=[]
        for y,r in ((y0,outer),(y1,outer),(y0,inner),(y1,inner)):
            verts += [(x+r*math.cos(i*math.tau/n),y,z+r*math.sin(i*math.tau/n)) for i in range(n)]
        for i in range(n):
            j=(i+1)%n
            faces += [(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),
                      (i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)]
        obj=self.mesh(name,verts,faces,role,.00012)
        for i,face in enumerate(obj.data.polygons):face.use_smooth=i%4 in (0,1)
        return obj

    def cut(self, targets, outline, name):
        cutter=self.prism("TEMP_TIDAL_"+name,outline,-.11,.11,"coat")
        x0,x1=min(v[0] for v in outline),max(v[0] for v in outline)
        z0,z1=min(v[1] for v in outline),max(v[1] for v in outline)
        for target in targets:
            bounds=[v.co for v in target.data.vertices]
            if (max(v.x for v in bounds)<x0 or min(v.x for v in bounds)>x1 or
                    max(v.z for v in bounds)<z0 or min(v.z for v in bounds)>z1):
                continue
            bpy.context.view_layer.objects.active=target
            target.select_set(True)
            modifier=target.modifiers.new("Through aperture: "+name,"BOOLEAN")
            modifier.operation='DIFFERENCE';modifier.object=cutter
            solvers={item.identifier for item in modifier.bl_rna.properties['solver'].enum_items}
            modifier.solver='MANIFOLD' if 'MANIFOLD' in solvers else 'EXACT'
            if hasattr(modifier,'use_self'):modifier.use_self=False
            result=bpy.ops.object.modifier_apply(modifier=modifier.name)
            assert 'FINISHED' in result,(name,target.name)
            if target.name=='TIDAL_HandguardShell':
                points=[v.co for v in target.data.vertices]
                spans=[max(v[k] for v in points)-min(v[k] for v in points) for k in range(3)] if points else [0,0,0]
                assert len(points)>1000 and spans[0]>.40 and spans[2]>.08,(name,'Boolean damaged shell',len(points),spans)
            self.boolean_count+=1
            print('TIDAL_APERTURE',name,target.name,flush=True)
            target.select_set(False)
        self.objects.remove(cutter)
        data=cutter.data;bpy.data.objects.remove(cutter,do_unlink=True)
        if not data.users:bpy.data.meshes.remove(data)


def shell(builder):
    """Continuous hollow shell, rounded nose, narrow roof and rolled shoulder."""
    vertices=[];rings=[]
    rear = MOUNT[0]
    front = p(307.5,0)[0]
    candidates=sorted([rear+i*(front-rear)/20 for i in range(21)]+[front-.090]+[front-.080+i*.080/14 for i in range(15)])
    stations=[]
    for x in candidates:
        if not stations or x-stations[-1]>1e-7:stations.append(x)
    steps=len(stations)-1
    for inner in (False,True):
        wall = .0020 if inner else 0.
        for i,nominal in enumerate(stations):
            half=.0275+mid_bulge(nominal)-wall
            upper=p(0,185)[1]-wall
            lower=p(0,318)[1]+wall
            contour=sampled_round_rect(-half,half,lower,upper,.0072-wall,8,78)
            # Put the shoulder break exactly on a contour row on both flanks.
            split=[]
            for j,(y,z) in enumerate(contour):
                split.append((y,z));yn,zn=contour[(j+1)%len(contour)]
                if abs(y-yn)<1e-9 and min(z,zn)<SHOULDER_BREAK_Z<max(z,zn):
                    split.append((y,SHOULDER_BREAK_Z))
            contour=split
            ids=[]
            for y,z in contour:
                # Roof band is narrower than the main flank.  The round
                # shoulder at y~218 is part of the shell, not an added slab.
                y *= side_surface(nominal,z,wall)/half
                # Only the last 80mm sweep into the shaped front rim.  The
                # straight body and rail are not stretched as a function of Z.
                weight=smooth(front-.080,front,nominal)
                x=nominal-weight*(front-front_x(z))
                ids.append(len(vertices));vertices.append((x,y,z))
            rings.append(ids)
    n=len(rings[0]);count=steps+1;faces=[]
    for layer in (0,1):
        for i in range(steps):
            a,b=rings[layer*count+i],rings[layer*count+i+1]
            for j in range(n):
                k=(j+1)%n
                quad=(a[j],a[k],b[k],b[j])
                if layer:quad=tuple(reversed(quad))
                # Axial transitions are small triangles, avoiding bent ngons.
                faces.extend(((quad[0],quad[1],quad[2]),(quad[0],quad[2],quad[3])))
    for i in (0,steps):
        a,b=rings[i],rings[count+i]
        for j in range(n):
            k=(j+1)%n;quad=(a[j],b[j],b[k],a[k])
            faces += [(quad[0],quad[1],quad[2]),(quad[0],quad[2],quad[3])]
    obj=builder.mesh('TIDAL_HandguardShell',vertices,faces,'coat',smooth_faces=True)
    obj['m7_front_rim_refinement']=json.dumps({
        'construction':'Direct outward-convex analytic nose arcs with matching inner and outer rims and a shared rounded crest tangent',
        'cross_section_vertices':n,'vertical_straight_samples':78,
        'longitudinal_rings':count,'scope':'shell front sweep; aperture coordinates retained',
        'post_boolean_subdivision':False})
    return obj


def side_plate(builder, side):
    """Shallow 0.7mm rolled step with a rounded front corner, not a wedge."""
    x0=p(884,0)[0];z0,z1=p(0,273)[1],p(0,229)[1]
    nx=100;zrows=sorted(set([z0+(z1-z0)*i/16 for i in range(17)]+[SHOULDER_BREAK_Z]))
    nz=len(zrows)-1;verts=[];faces=[]
    for back in (False,True):
        for iz,z in enumerate(zrows):
            t=(z-z0)/(z1-z0)
            xmax=p(443-26*t,0)[0]
            for ix in range(nx+1):
                x=x0+(xmax-x0)*ix/nx
                edge=min((x-x0)/.0012,(xmax-x)/.0012,(z-z0)/.0012,(z1-z)/.0012)
                rise=.00085*smooth(0,1,edge)
                offset=-.0012 if back else rise
                verts.append((x,side*(side_surface(x,z)+offset),z))
    stride=nx+1;layer=stride*(nz+1)
    for back in (0,1):
        for iz in range(nz):
            for ix in range(nx):
                a=back*layer+iz*stride+ix;quad=(a,a+1,a+stride+1,a+stride)
                if back:quad=tuple(reversed(quad))
                faces.extend(((quad[0],quad[1],quad[2]),(quad[0],quad[2],quad[3])))
    rim=list(range(stride))+[iz*stride+nx for iz in range(1,nz+1)]
    rim+=list(range(nz*stride+nx-1,nz*stride-1,-1))+[iz*stride for iz in range(nz-1,0,-1)]
    for i,a in enumerate(rim):
        b=rim[(i+1)%len(rim)];faces.append((a,b,b+layer,a+layer))
    return builder.mesh('TIDAL_UpperSideShoulder_'+str(side),verts,faces,'coat',smooth_faces=True)


def textured_fields(builder):
    # These flush fields carry a fine orthogonal bump shader.  No coarse,
    # undersampled corrugation is written into the silhouette.
    for side in (-1,1):
        for i,(a,b) in enumerate(((570,670),(680,772),(822,875))):
            x0,z1=p(b,240);x1,z0=p(a,262)
            nx=max(8,math.ceil((x1-x0)/.003));nz=6;vertices=[];faces=[]
            for back in (False,True):
                for iz in range(nz+1):
                    z=z0+(z1-z0)*iz/nz
                    for ix in range(nx+1):
                        x=x0+(x1-x0)*ix/nx
                        offset=.00084 if back else .00092
                        vertices.append((x,side*(side_surface(x,z)+offset),z))
            stride=nx+1;layer=stride*(nz+1)
            for back in (0,1):
                for iz in range(nz):
                    for ix in range(nx):
                        a=back*layer+iz*stride+ix;quad=(a,a+1,a+stride+1,a+stride)
                        if back:quad=tuple(reversed(quad))
                        faces.extend(((quad[0],quad[1],quad[2]),(quad[0],quad[2],quad[3])))
            rim=list(range(stride))+[iz*stride+nx for iz in range(1,nz+1)]
            rim+=list(range(nz*stride+nx-1,nz*stride-1,-1))+[iz*stride for iz in range(nz-1,0,-1)]
            for j,a in enumerate(rim):
                b=rim[(j+1)%len(rim)];faces.append((a,b,b+layer,a+layer))
            builder.mesh('TIDAL_FineTextureField_%s_%d'%(side,i),vertices,faces,'panel',smooth_faces=True)


def circle(px,py,radius_px):
    x,z=p(px,py);r=radius_px*SCALE_Z
    return [(x+r*math.cos(i*math.tau/64),z+r*math.sin(i*math.tau/64)) for i in range(64)]


def stable_triangulation(objects):
    # Freeze n-gon tessellation before derived copies move to their export
    # origins.  The source modifiers and material normals remain editable.
    for obj in objects:
        weld=obj.modifiers.get('M7 Remove collapsed bevel vertices') or obj.modifiers.new('M7 Remove collapsed bevel vertices','WELD')
        weld.merge_threshold=1e-7
        modifier=obj.modifiers.get('M7 Stable export triangulation') or obj.modifiers.new('M7 Stable export triangulation','TRIANGULATE')
        modifier.min_vertices=4;modifier.quad_method='FIXED';modifier.ngon_method='BEAUTY'
        if hasattr(modifier,'keep_custom_normals'):modifier.keep_custom_normals=True


def fine_panel_material(material):
    nodes,links=material.node_tree.nodes,material.node_tree.links
    bsdf=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    old=bsdf.inputs['Normal'].links[0].from_socket if bsdf.inputs['Normal'].is_linked else None
    geo=nodes.new('ShaderNodeNewGeometry');geo.label='Fine grid in physical metres'
    sep=nodes.new('ShaderNodeSeparateXYZ');links.new(geo.outputs['Position'],sep.inputs[0])
    signals=[]
    for axis in ('X','Z'):
        scale=nodes.new('ShaderNodeMath');scale.operation='MULTIPLY';scale.inputs[1].default_value=math.tau/.00065
        links.new(sep.outputs[axis],scale.inputs[0])
        sine=nodes.new('ShaderNodeMath');sine.operation='SINE';links.new(scale.outputs[0],sine.inputs[0])
        remap=nodes.new('ShaderNodeMath');remap.operation='MULTIPLY_ADD';remap.inputs[1].default_value=.5;remap.inputs[2].default_value=.5
        links.new(sine.outputs[0],remap.inputs[0])
        power=nodes.new('ShaderNodeMath');power.operation='POWER';power.inputs[1].default_value=6
        links.new(remap.outputs[0],power.inputs[0]);signals.append(power.outputs[0])
    dots=nodes.new('ShaderNodeMath');dots.operation='MULTIPLY'
    links.new(signals[0],dots.inputs[0]);links.new(signals[1],dots.inputs[1])
    bump=nodes.new('ShaderNodeBump');bump.label='Subtle 0.65mm orthogonal dimples'
    grain=nodes.get('M7_FineFinishNormal')
    if grain:grain.name='M7_GrainNormal'
    bump.name='M7_FineFinishNormal'
    bump.inputs['Strength'].default_value=.32;bump.inputs['Distance'].default_value=.000035
    bump.invert=True;links.new(dots.outputs[0],bump.inputs['Height'])
    if old:links.new(old,bump.inputs['Normal'])
    links.new(bump.outputs['Normal'],bsdf.inputs['Normal'])
    material['fine_grid_pitch_m']=.00065;material['fine_grid_effective_depth_m']=.000035*.32


def aperture(builder, walls, checks, name, px0, px1, py0, py1, radius_px=3., shear_px=0.):
    x0,z1=p(px1,py0);x1,z0=p(px0,py1)
    shear=-shear_px*SCALE_X/SCALE_Z
    outline=rounded_rect(x0,x1,z0,z1,radius_px*SCALE_Z,8,shear)
    builder.cut(walls,outline,name)
    checks.append({'name':name,'center_m':[(x0+x1)/2,0.,(z0+z1)/2],
                   'outline_m':outline})


def build_geometry(builder):
    guard=shell(builder)
    plates=[side_plate(builder,side) for side in (-1,1)]
    walls=[guard,*plates];checks=[]
    for i in range(10):
        center=363+31.5*i
        aperture(builder,walls,checks,'UpperVent_%02d'%i,center-11.5,center+11.5,196,218,2.4,.34)
    builder.cut(walls,circle(676,209,7.1),'UpperCircularPort')
    checks.append({'name':'UpperCircularPort','center_m':[p(676,209)[0],0.,p(676,209)[1]]})
    for i,(a,b) in enumerate(((455,484),(494,520),(531,559))):
        # A wider round lobe meets a narrower stem: connected real keyholes.
        aperture(builder,walls,checks,'KeyStem_%d'%i,a,b-7,246,257,4.8)
        center=b-8
        builder.cut(walls,circle(center,252,9.0),'KeyLobe_%d'%i)
        checks.append({'name':'KeyLobe_%d'%i,'center_m':[p(center,252)[0],0.,p(center,252)[1]]})
    for i,(a,b) in enumerate(((458,559),(573,663),(677,764))):
        aperture(builder,walls,checks,'LowerLongSlot_%d'%i,a,b,284,302,5.6,.12)
    for i,(a,b) in enumerate(((342,359),(374,391),(407,424))):
        aperture(builder,walls,checks,'FrontDiagonal_%d'%i,a,b,247,307,3.7,-.43)
    builder.cut(walls,circle(799,248,6.0),'QDThroughPort')
    checks.append({'name':'QDThroughPort','center_m':[p(799,248)[0],0.,p(799,248)[1]]})
    for wall in walls:builder.bevel(wall,.00048)
    # Preserve the authored shoulder transition in shading after booleans.
    for wall in walls:
        for edge in wall.data.edges:
            a,b=(wall.data.vertices[i].co for i in edge.vertices)
            if abs(a.z-SHOULDER_BREAK_Z)<1e-7 and abs(b.z-SHOULDER_BREAK_Z)<1e-7 and a.y*b.y>0:
                edge.use_edge_sharp=True
    for side in (-1,1):
        x,z=p(799,248)
        walls.append(builder.ring_y('TIDAL_QDSocket_'+str(side),x,z,side*(side_surface(x,z)+.00070),side*(side_surface(x,z)+.0014),
                                    .0047,6.0*SCALE_Z,'hardware'))
    textured_fields(builder)

    # User side view and marked connection override the old lowered roof.
    # Rail height, width, tooth pitch and phase match the existing receiver.
    lo,hi=MOUNT[0],p(350,0)[0]
    builder.box('TIDAL_UpperRailBase',((lo+hi)/2,0,.0472), (hi-lo,.024,.009),'coat',.00035)
    i=0;x=.1632
    while x-.0045 < hi:
        left,right=x-.0045,min(x+.0045,hi)
        if right-left>.0005:
            shoulder=min(.0010,(right-left)/5)
            outline=[(left,.0485),(left,.0527),(left+shoulder,.0545),
                     (right-shoulder,.0545),(right,.0527),(right,.0485)]
            builder.prism('TIDAL_UpperRailTooth_%02d'%i,outline,-.016,.016,'coat',.00020)
        i+=1;x=.1632+i*.0133
    lower_lo,lower_hi=p(870,0)[0],p(365,0)[0]
    builder.box('TIDAL_LowerRailBase',((lower_lo+lower_hi)/2,0,-.0446),
                (lower_hi-lower_lo,.022,.0044),'coat',.00035)
    tooth_count=29
    for i in range(tooth_count):
        x=lower_lo+.004+i*(lower_hi-lower_lo-.008)/(tooth_count-1)
        builder.box('TIDAL_LowerRailTooth_%02d'%i,(x,0,-.0481),(.0065,.029,.0028),'coat',.00020)
    main_front=p(53,0)[0]+BARREL_EXTENSION;muzzle=p(35,0)[0]+BARREL_EXTENSION
    builder.tube('TIDAL_BarrelMain',.154,main_front,11.0*SCALE_X,.00355,'hardware')
    builder.tube('TIDAL_FrontSteppedCollar',p(61,0)[0]+BARREL_EXTENSION,p(49,0)[0]+BARREL_EXTENSION,12.5*SCALE_X,.00355,'hardware')
    builder.tube('TIDAL_ExternalFrontCrown',p(49,0)[0]+BARREL_EXTENSION,muzzle,7.4*SCALE_X,.00325,'hardware')
    builder.tube('TIDAL_RearBarrelCollar',.163,.184,.0138,.0092,'hardware')
    builder.tube('TIDAL_VisibleGasTube',.174,p(356,0)[0],.0027,.0012,'hardware',z=.015)
    builder.box('TIDAL_InternalGasBlock',(p(400,0)[0],0,.006),(.014,.017,.018),'hardware',.0010)
    stable_triangulation(builder.objects)
    return walls,checks


def verify_apertures(walls, checks):
    bpy.context.view_layer.update()
    graph=bpy.context.evaluated_depsgraph_get();graph.update()
    evaluated=[obj.evaluated_get(graph) for obj in walls]
    for check in checks:
        center=Vector(check['center_m'])
        probes=[center]
        outline=check.get('outline_m')
        if outline:
            for x,z in outline[::max(1,len(outline)//8)]:
                probes.append(Vector(((x+center.x)/2,0,(z+center.z)/2)))
        else:
            probes += [center+Vector((dx,0,dz)) for dx,dz in ((.001,0),(-.001,0),(0,.001),(0,-.001))]
        blocked=[]
        for index,point in enumerate(probes):
            origin=point.copy();origin.y=.10
            for obj in evaluated:
                inv=obj.matrix_world.inverted()
                hit,*_=obj.ray_cast(inv@origin,inv.to_3x3()@Vector((0,-1,0)),distance=.20)
                if hit:blocked.append({'object':obj.name,'probe':index})
        check['sidewalls_through_open']=not blocked
        check['blockers']=blocked
        check['interior_ray_probe_count']=len(probes)
    assert all(c['sidewalls_through_open'] for c in checks),checks
    return checks


def verify_rail_connection():
    def bounds(obj):
        points=[obj.matrix_world@Vector(v) for v in obj.bound_box]
        return [[min(v[k] for v in points) for k in range(3)],
                [max(v[k] for v in points) for k in range(3)]]
    core=bounds(bpy.data.objects['CORE_ReceiverTopRailBase'])
    guard=bounds(bpy.data.objects['TIDAL_UpperRailBase'])
    c=bounds(bpy.data.objects['CORE_RailTooth_23'])
    g=bounds(bpy.data.objects['TIDAL_UpperRailTooth_00'])
    lower=bounds(bpy.data.objects['TIDAL_LowerRailBase'])
    tooth=bounds(bpy.data.objects['TIDAL_LowerRailTooth_00'])
    gap=guard[0][0]-core[1][0]
    pitch=(g[0][0]+g[1][0]-c[0][0]-c[1][0])/2
    height=g[1][2]-c[1][2]
    assert abs(gap)<.00005 and abs(pitch-.0133)<.00005 and abs(height)<.00005
    assert lower[0][2]<=tooth[1][2]+.00001,'Lower rail teeth are separated from their base'
    return {'measured':True,'receiver_base_end_x_m':core[1][0],
            'handguard_base_start_x_m':guard[0][0],'base_gap_m':gap,
            'tooth_pitch_at_joint_m':pitch,'tooth_top_height_difference_m':height,
            'tooth_top_z_m':g[1][2],'lower_rail_overlap_m':tooth[1][2]-lower[0][2]}


def rebuild_tidal_reference():
    manifest_path=ROOT/'AssetManifest.json'
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    row=next(a for a in manifest['assets'] if a['name']==ASSET)
    source=(ROOT/manifest['source_file']).resolve()
    assert source.is_relative_to(ROOT.resolve()),source
    assert bpy.data.collections.get(row['collection']),"Open the material-study source first."
    collection=bpy.data.collections[row['collection']]
    root=bpy.data.objects[ASSET+'_Root']
    assert all(abs(root.matrix_world[i][j]-(1. if i==j else 0.))<1e-7 for i in range(4) for j in range(4)),"Source root must remain identity."
    assert (Vector(collection.instance_offset)-Vector(MOUNT)).length<1e-6
    sockets={o.get('mount_key'):o for o in collection.objects if o.type=='EMPTY' and o!=root and o.get('mount_key')}
    assert {'Foregrip','Muzzle','FrontIron'}<=sockets.keys(),list(sockets)
    previous=bpy.context.window.scene
    edit=bpy.data.scenes['M7_Library_Edit'];bpy.context.window.scene=edit
    layers=list(edit.view_layers[0].layer_collection.children)
    excluded=[(layer,layer.exclude) for layer in layers]
    for layer in layers:layer.exclude=False
    edit.view_layers[0].update()
    if bpy.context.object and bpy.context.object.mode!='OBJECT':bpy.ops.object.mode_set(mode='OBJECT')
    try:
        sys.path.insert(0,str(ROOT/'Scripts'))
        from build_materials import create_material,CONFIG
        contact=[m for m in CONFIG['wear_masks'] if m['id']=='handguard_support_hand_contact']
        materials={
            'coat':create_material('M7M_Barrel_Tidal_Guard_coated_handguard','coated_handguard',contact),
            'hardware':create_material('M7M_Barrel_Tidal_Guard_dark_steel_hardware','dark_steel_hardware',[]),
            'panel':create_material('M7M_Barrel_Tidal_Guard_fine_grid','coated_handguard',contact),
        }
        fine_panel_material(materials['panel'])
        removed=[]
        for obj in list(collection.objects):
            if obj.type not in ('MESH','FONT'):continue
            removed.append(obj.name);data=obj.data
            bpy.data.objects.remove(obj,do_unlink=True)
            if not data.users:
                if isinstance(data,bpy.types.Mesh):bpy.data.meshes.remove(data)
                else:bpy.data.curves.remove(data)
        builder=Builder(collection,root,materials)
        walls,checks=build_geometry(builder)
        verify_apertures(walls,checks)
        rail_connection=verify_rail_connection()
        grip_px=886-(.347-MOUNT[0])/SCALE_X
        positions={'Muzzle':[p(35,0)[0]+BARREL_EXTENSION,0.,0.],
                   'Foregrip':[p(grip_px,0)[0],0.,-.0495],
                   'FrontIron':[p(374,0)[0],0.,.0545]}
        for key,position in positions.items():
            socket=sockets[key]
            local=[position[i]-MOUNT[i] for i in range(3)]
            socket.matrix_parent_inverse=Matrix.Identity(4)
            socket.matrix_basis=Matrix.Translation(position)
            socket['mount_position_m']=position;socket['local_mount_position_m']=local
            row['local_mounts_m'][key]=local
        row['glb_mount_positions_m']=dict(row['local_mounts_m'])
        for key in ('texture_sets','baked_uv_layer','atlas_resolution'):row.pop(key,None)
        row['preserved_constant_materials']=[]
        row['editable_objects']=len(collection.objects)
        row['geometry_review']='TidalReferenceReview.json'
        report={
            'success':True,'asset':ASSET,'reference_image':str(REFERENCE),
            'iteration':'Handguard +120mm; constant full central band along the entire length; narrower triangular shoulders with a deliberate slope break; lower edge rolls inward. Exposed tube retains reference length.',
            'reference_images':[str(REFERENCE),str(ROOT/'Reference/Tidal_Side_User.png'),
                                str(ROOT/'Reference/Tidal_Rail_Connection_User.png'),str(ROOT/'Reference/Tidal_Nose_User.png')],
            'mapping':{'x_formula':'.158 + (886-px) * (.055/89) + .120 * smoothstep(0,436,886-px)',
                       'z_formula':'(260-py) * (.055/89) above bore, (.050/67) below bore',
                       'metres_per_pixel_x':SCALE_X,'metres_per_pixel_z':SCALE_Z,
                       'lower_metres_per_pixel_z':LOWER_SCALE_Z,
                       'depth_inferred':True,'perspective_corrected':False,
                       'scale_basis':'new side view silhouette and existing receiver rail plane'},
            'construction':'Continuous rounded hollow shoulder shell, densely sampled nose, shallow rolled side step, exact through apertures and flush fine-grid fields.',
            'sidewall_m':.0020,'rounded_edge_segments':8,'circle_segments':64,
            'collapsed_bevel_vertex_weld_m':1e-7,
            'front_rim_curve_refinement':json.loads(bpy.data.objects['TIDAL_HandguardShell']['m7_front_rim_refinement']),
            'upper_window_count':10,'through_apertures':checks,
            'ray_scope':'Multiple interior samples across both sidewalls and sideplates; barrel/gas tube are modeled inside the open cavity.',
            'boolean_operations':builder.boolean_count,
            'boolean_solver':'MANIFOLD when available, EXACT fallback; closed cleaned input solids',
            'removed_geometry':removed,'new_geometry_objects':len(builder.objects),
            'top_rail_world_x_m':[MOUNT[0],p(350,0)[0]],
            'exposed_barrel_extension_m':BARREL_EXTENSION,
            'handguard_length_extension_m':HANDGUARD_EXTENSION,
            'central_half_width_bulge_m':MID_BODY_BULGE,
            'transverse_flank_crown_m':FLANK_CROWN,
            'upper_neck_half_width_m':UPPER_NECK_HALF_WIDTH,
            'lower_edge_half_width_m':LOWER_EDGE_HALF_WIDTH,
            'central_crest_z_m':MID_ZONE_Z,
            'upper_profile':'straight inclined shoulders begin at an explicit slope break above the round middle; narrow truncated apex supports rail',
            'central_width_constant_along_length':True,
            'shoulder_break_z_m':SHOULDER_BREAK_Z,
            'front_nose_revision':'User-corrected outward-convex connection removes the concave S-transition; rounded crest, endpoints and peak X anchors retained.',
            'central_cross_section_flat_band':False,
            'central_max_outer_width_m':2*side_surface((MOUNT[0]+p(307.5,0)[0])/2,MID_ZONE_Z),
            'rail_connection':rail_connection,
            'muzzle_world_m':positions['Muzzle'],'muzzle_local_m':row['local_mounts_m']['Muzzle'],
            'foregrip_world_m':positions['Foregrip'],'front_iron_world_m':positions['FrontIron'],
            'uv_status':'Provisional UV only on new geometry; this asset requires a fresh atlas bake.',
            'source_file':manifest['source_file'],'unreal_tested':False,
        }
        material_path=ROOT/'MaterialReport.json'
        material_report=json.loads(material_path.read_text(encoding='utf-8'))
        material_report['atlases']=[a for a in material_report.get('atlases',[]) if a.get('asset')!=ASSET]
        material_report['assignments']=[a for a in material_report.get('assignments',[]) if a.get('asset')!=ASSET]
        material_report['assignments'] += [{'asset':ASSET,'object':o.name,'material':o.data.materials[0].name,
                                            'role':o.data.materials[0]['m7_role']} for o in builder.objects]
        material_report['geometry_unchanged']=False
        material_report['maps_baked']=False
        material_report['status']='Tidal redone from side view and rail connection correction; fresh atlas and exports pending'
        material_report.setdefault('geometry_changes',[]).append('Tidal redone after rejected attempt, with connected rail and corrected shell proportions.')
        manifest['material_stage']='Tidal reference geometry rebuilt; its atlas bake and exports pending'
        manifest['tidal_reference_review']='TidalReferenceReview.json'
        (ROOT/'TidalReferenceReview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        material_path.write_text(json.dumps(material_report,ensure_ascii=False,indent=2),encoding='utf-8')
        manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        text=bpy.data.texts.get('M7_AssetManifest.json')
        if text:text.clear();text.from_string(json.dumps(manifest,ensure_ascii=False,indent=2))
        # Keep live assemblies in sync with the changed dependent sockets.
        rows={a['name']:a for a in manifest['assets']}
        for scene in bpy.data.scenes:
            instances=[o for o in scene.objects if o.instance_collection and o.instance_collection.name in rows]
            barrels=[o for o in instances if rows[o.instance_collection.name]['mount_key']=='Barrel']
            if len(barrels)==1 and barrels[0].instance_collection==collection:
                for obj in instances:
                    key=rows[obj.instance_collection.name]['mount_key']
                    if key in positions:obj.location=positions[key]
                scene.view_layers[0].update()
    finally:
        for layer,value in excluded:layer.exclude=value
        bpy.context.window.scene=previous
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(source),compress=True)
    print(json.dumps(report,ensure_ascii=False))
    return report


if __name__=='__main__':
    rebuild_tidal_reference()
