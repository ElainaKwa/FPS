"""Reusable mesh helpers for the M7 visual prop."""
import bpy, bmesh, math

def enum_set(owner,key,value):
    valid=[i.identifier for i in owner.bl_rna.properties[key].enum_items]
    if valid and valid != ['NONE'] and value not in valid:
        raise ValueError((key,value,valid))
    setattr(owner,key,value)

def configure(collection, asset_root, materials):
    global active_collection, asset, root, mats
    active_collection=asset=collection
    root=asset_root
    mats=materials

def move_to(obj, name, mat):
    obj.name = name
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    active_collection.objects.link(obj)
    if active_collection == asset:
        obj.parent = root
    if mat:
        obj.data.materials.append(mats[mat] if isinstance(mat, str) else mat)
    return obj


def bevel_obj(o, amount=.002, segments=3):
    if amount > 0:
        m = o.modifiers.new('Soft machined cartoon edge', 'BEVEL')
        m.width = amount
        m.segments = segments
        enum_set(m, 'limit_method', 'ANGLE')
        m.harden_normals = True
        n = o.modifiers.new('Weighted broad face normals', 'WEIGHTED_NORMAL')
        n.keep_sharp = True
        n.weight = 50
    return o


def mesh(name, verts, faces, mat, bevel=.001):
    data = bpy.data.meshes.new(name + '_Geometry')
    data.from_pydata(verts, [], faces)
    data.update()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    o = bpy.data.objects.new(name, data)
    active_collection.objects.link(o)
    o.parent = root if active_collection == asset else None
    data.materials.append(mats[mat] if isinstance(mat, str) else mat)
    return bevel_obj(o, bevel)


def box(name, loc, dims, mat, bevel=.002):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = move_to(bpy.context.object, name, mat)
    o.dimensions = dims
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return bevel_obj(o, min(bevel, min(dims)*.45))


def cyl(name, loc, radius, depth, mat, axis='X', vertices=32):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    o = move_to(bpy.context.object, name, mat)
    if axis == 'X':
        o.rotation_euler[1] = math.pi/2
    elif axis == 'Y':
        o.rotation_euler[0] = math.pi/2
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for p in o.data.polygons:
        p.use_smooth = len(p.vertices) == 4
    return bevel_obj(o, min(radius*.09, depth*.15, .0015))


def tube(name, loc, radius, inner, depth, mat, axis='X', segments=48):
    verts = []
    for a,r in [(-depth/2,radius),(depth/2,radius),(-depth/2,inner),(depth/2,inner)]:
        for i in range(segments):
            u,v = r*math.cos(i*math.tau/segments), r*math.sin(i*math.tau/segments)
            pt = (a,u,v) if axis == 'X' else ((u,a,v) if axis == 'Y' else (u,v,a))
            verts.append(tuple(loc[k]+pt[k] for k in range(3)))
    faces=[]
    for i in range(segments):
        j=(i+1)%segments
        faces.extend([(i,j,segments+j,segments+i),(2*segments+i,3*segments+i,3*segments+j,2*segments+j),
                      (i,2*segments+i,2*segments+j,j),(segments+i,segments+j,3*segments+j,3*segments+i)])
    o = mesh(name,verts,faces,mat,.0007)
    for i,p in enumerate(o.data.polygons):
        p.use_smooth = i%4 in (0,1)
    return o


def profile(name, points, width, mat, bevel=.002):
    n=len(points)
    verts=[(x,y,z) for y in (-width/2,width/2) for x,z in points]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,verts,faces,mat,bevel)


def text(name,body,loc,size,mat,rotation=(0,0,0)):
    d=bpy.data.curves.new(name+'_Lettering','FONT')
    d.body=body
    d.size=size
    d.extrude=.00012
    d.bevel_depth=.000035
    d.bevel_resolution=1
    o=bpy.data.objects.new(name,d)
    active_collection.objects.link(o)
    o.parent=root if active_collection==asset else None
    o.location=loc
    o.rotation_euler=rotation
    d.materials.append(mats[mat] if isinstance(mat, str) else mat)
    return o


def screw(name,loc,r=.0034,axis='Y'):
    o=cyl(name+'_Head',loc,r,.0022,'steel',axis,12)
    if axis == 'Y':
        box(name+'_Socket',(loc[0],loc[1]+(.0013 if loc[1]>0 else -.0013),loc[2]),(r*.92,.0007,r*.22),'dark',.0001)
    elif axis == 'Z':
        box(name+'_Socket',(loc[0],loc[1],loc[2]+.0013),(r*.9,r*.2,.0007),'dark',.0001)
    return o


def apply_mods(o):
    bpy.context.view_layer.objects.active=o
    for m in list(o.modifiers):
        bpy.ops.object.modifier_apply(modifier=m.name)


