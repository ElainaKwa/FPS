"""Editable original stylized AR. Execute inside Blender via MCP.

Visual game prop, metres, +X muzzle, +Z up. No mechanical internals.
"""
import bpy
import bmesh
import math
import json
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(r'D:/Unreal Projects/MCP_Test/ArtSource/ToonRifle')
ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / 'Source'))


def enum_set(owner, key, value):
    prop = owner.bl_rna.properties[key]
    valid = [i.identifier for i in prop.enum_items]
    if value not in valid and valid != ['NONE']:
        raise ValueError((key, value, valid))
    setattr(owner, key, value)


if bpy.data.collections.get('ASSET_TIDELINE_AR01'):
    raise RuntimeError('Asset already exists. Rebuild in a fresh Blender session to preserve editable work.')
scene = bpy.data.scenes.new('TIDELINE_AR01_Studio')
bpy.context.window.scene = scene
enum_set(scene.unit_settings, 'system', 'METRIC')
scene.unit_settings.scale_length = 1.0
asset = bpy.data.collections.new('ASSET_TIDELINE_AR01')
scene.collection.children.link(asset)
studio = bpy.data.collections.new('STUDIO_Preview_Only')
scene.collection.children.link(studio)
active_collection = asset
root = bpy.data.objects.new('AR01_Root_GripOrigin', None)
asset.objects.link(root)
root['asset_id'] = 'TIDELINE_AR01'
root['coordinate_system'] = 'metres; +X muzzle, +Z up'
root['description'] = 'Original detailed cartoon assault rifle, tactical visual reference.'


def linear(c):
    return c / 12.92 if c <= .04045 else ((c + .055) / 1.055) ** 2.4


def material(name, color, metal=0, rough=.4):
    m = bpy.data.materials.new('AR01_' + name)
    m.use_nodes = True
    rgba = tuple(linear(c) for c in color) + (1,)
    m.diffuse_color = rgba
    n = next(n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    n.inputs['Base Color'].default_value = rgba
    n.inputs['Metallic'].default_value = metal
    n.inputs['Roughness'].default_value = rough
    return m


mats = {
    'navy': material('01_MidnightCeramic', (.16,.23,.31), .32,.32),
    'teal': material('02_LagoonPolymer', (.12,.57,.57), .10,.36),
    'ivory': material('03_WarmPorcelain', (.88,.89,.80), .12,.32),
    'orange': material('04_SignalAmber', (1.0,.44,.13), .15,.32),
    'rubber': material('05_GripRubber', (.10,.145,.18), .0,.72),
    'steel': material('06_SatinSteel', (.44,.56,.59), .72,.29),
    'dark': material('07_RecessBlack', (.045,.079,.10), .1,.51),
    'glass': material('08_OpticalGlass', (.20,.78,.79), .1,.13),
    'emission': material('09_SightIndicator', (.34,1.0,.82), .05,.22),
}
gn = next(n for n in mats['glass'].node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
gn.inputs['Transmission Weight'].default_value = .93
gn.inputs['IOR'].default_value = 1.45
en = next(n for n in mats['emission'].node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
en.inputs['Emission Color'].default_value = (.05,.7,.42,1)
en.inputs['Emission Strength'].default_value = 2.5


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


# Receiver: multiple stepped forged shells, separate controls and recesses.
profile('Receiver_UpperForgedShell',[(-.145,-.016),(-.145,.055),(-.125,.076),(.16,.076),(.18,.06),(.18,-.016),(.12,-.028),(-.10,-.028)],.068,'navy',.005)
profile('Receiver_LowerShell',[(-.135,-.021),(.16,-.021),(.159,-.079),(.079,-.092),(.061,-.062),(-.018,-.062),(-.046,-.077),(-.115,-.06)],.063,'teal',.004)
box('Receiver_MagwellCollar',(.12,0,-.077),(.102,.072,.024),'navy',.0035)
cyl('Receiver_BufferSocket',(-.154,0,.023),.032,.022,'steel','X')
cyl('Stock_BufferTube',(-.254,0,.025),.021,.195,'navy','X')
for x in (-.183,-.205,-.229):
    tube('Stock_BufferAdjustmentBand',(x,0,.025),.022,.019,.005,'steel')

for s in (-1,1):
    # Ceramic side accents follow the silhouette, avoiding a flat toy box.
    p=profile('Receiver_SideCeramicPanel_'+str(s),[(-.122,.051),(-.112,.067),(.143,.067),(.16,.053),(.155,.039),(-.119,.039)],.0035,'ivory',.002)
    p.location.y=s*.035
    p=profile('Receiver_LowerInset_'+str(s),[(-.116,-.031),(-.022,-.031),(-.025,-.055),(-.066,-.066),(-.113,-.052)],.002,'navy',.002)
    p.location.y=s*.032
    for x,z in [(-.127,.015),(.151,.003),(-.09,-.039),(.139,-.054)]:
        screw('Receiver_Pin_'+str(s)+'_'+str(x),(x,s*.035,z),.0038)
    cyl('Receiver_SelectorHub_'+str(s),(-.06,s*.035,-.041),.010,.006,'steel','Y',24)
    sw=box('Receiver_SelectorPaddle_'+str(s),(-.068,s*.040,-.045),(.024,.009,.007),'navy',.002)
    sw.rotation_euler[1]=-.38
    for a in (-.8,0,.8):
        x=-.06+math.sin(a)*.014
        z=-.041+math.cos(a)*.014
        box('Receiver_SelectorTick_'+str(s),(x,s*.035,z),(.0018,.0009,.003),'orange',.0003)

# Near-side recessed plate with original branding.
box('Receiver_IdentityRecess',(-.035,-.0358,.013),(.17,.003,.042),'dark',.005)
box('Receiver_IdentityPlate',(-.035,-.0378,.014),(.163,.002,.035),'navy',.004)
text('Receiver_TIDELINE','TIDELINE',(-.108,-.0394,.014),.0135,'ivory',(math.pi/2,0,0))
text('Receiver_AR01','AR / 01   •   FIELD SERIES',(-.107,-.0395,.003),.0051,'steel',(math.pi/2,0,0))
box('Receiver_IDStripe',(.061,-.036,.016),(.009,.003,.033),'orange',.0014)
for j in range(3):
    box('Receiver_AmberInlay',(.077+j*.006,-.036,.015),(.0025,.002,.032),'orange',.0006)

# Far-side ejection-shaped visual recess and raised charging handle.
box('Receiver_PortRecess',(.017,.035,.024),(.097,.003,.032),'dark',.004)
box('Receiver_PortInner',(.025,.037,.026),(.079,.002,.022),'steel',.003)
box('Receiver_DustCover',(.016,.039,.007),(.100,.006,.010),'navy',.002)
cyl('Receiver_DustCoverHinge',(.016,.041,.005),.003,.09,'steel','X',16)
box('Receiver_ChargingLatch',(-.135,0,.071),(.025,.094,.013),'navy',.003)
for s in (-1,1):
    box('Receiver_ChargingGrip_'+str(s),(-.139,s*.045,.072),(.027,.012,.018),'rubber',.003)
    box('Receiver_BoltCatch_'+str(s),(-.01,s*.038,-.017),(.014,.01,.017),'steel',.002)

# Trigger guard made from separate rounded beams: visible genuine opening.
profile('Trigger_Guard',[(-.027,-.056),(-.021,-.056),(-.020,-.093),(.045,-.093),(.049,-.059),(.056,-.059),(.056,-.095),(.048,-.101),(-.021,-.101),(-.029,-.095)],.014,'navy',.0025)
profile('Trigger_Curved',[(.011,-.055),(.019,-.055),(.017,-.075),(.009,-.086),(.003,-.085),(.010,-.074)],.007,'orange',.0015)

# Open octagonal handguard shell, with real rounded side slots.
outer=[(-.029,.066),(.029,.066),(.043,.052),(.043,-.010),(.029,-.026),(-.029,-.026),(-.043,-.010),(-.043,.052)]
inner=[(-.025,.058),(.025,.058),(.035,.048),(.035,-.006),(.025,-.018),(-.025,-.018),(-.035,-.006),(-.035,.048)]
verts=[(x,y,z) for x,ring in [(.18,outer),(.48,outer),(.18,inner),(.48,inner)] for y,z in ring]
faces=[]
for i in range(8):
    j=(i+1)%8
    faces.extend([(i,j,8+j,8+i),(16+i,24+i,24+j,16+j),(i,16+i,16+j,j),(8+i,8+j,24+j,24+i)])
guard=mesh('Handguard_OpenOctagonalShell',verts,faces,'teal',0)
cutters=[]
for x in (.210,.252,.294,.336,.378,.420,.458):
    for z in (.042,.003):
        o=box('TEMP_SlotCutter',(x,0,z),(.028,.12,.013),'dark',.006)
        apply_mods(o)
        cutters.append(o)
bpy.ops.object.select_all(action='DESELECT')
for o in cutters:o.select_set(True)
bpy.context.view_layer.objects.active=cutters[0]
bpy.ops.object.join()
cutter=bpy.context.object
bpy.context.view_layer.objects.active=guard
bo=guard.modifiers.new('Real ventilation cutouts','BOOLEAN')
enum_set(bo,'operation','DIFFERENCE')
enum_set(bo,'solver','EXACT')
bo.object=cutter
bpy.ops.object.modifier_apply(modifier=bo.name)
bpy.data.objects.remove(cutter,do_unlink=True)
bevel_obj(guard,.0015,3)

# Handguard collars; exposed internal visual barrel appears through cutouts.
cyl('Barrel_Main',(.331,0,.020),.011,.48,'dark','X',48)
for x in (.18,.476):
    outer2=[(x-.006,y,z) for y,z in outer]+[(x+.006,y,z) for y,z in outer]
    vv=outer2+[(x-.006,y,z) for y,z in inner]+[(x+.006,y,z) for y,z in inner]
    c=mesh('Handguard_EndCollar',vv,faces,'ivory',.0015)
    for s in (-1,1):
        screw('Handguard_CollarBolt',(x,s*.044,.023),.0032)
for s in (-1,1):
    box('Handguard_SideSpine_'+str(s),(.33,s*.043,.023),(.26,.004,.010),'navy',.0015)
    for x in (.22,.28,.34,.40,.455):
        box('Handguard_IndexMark',(x,s*.046,.024),(.005,.0008,.003),'ivory',.0003)
    text('Handguard_Serial_'+str(s),'07 / TIDE',(.389,s*.047,.026),.006,'ivory',(math.pi/2 if s<0 else -math.pi/2,0,0))

# Continuous raised rail, top and bottom; individual chamfered teeth.
box('Rail_TopSpine',(.169,0,.080),(.632,.045,.013),'navy',.002)
for i in range(35):
    x=-.137+i*.018
    profile('Rail_TopTooth_%02d'%i,[(x-.006,.085),(x-.006,.094),(x-.004,.098),(x+.004,.098),(x+.006,.094),(x+.006,.085)],.056,'navy',.0008)
box('Rail_BottomSpine',(.328,0,-.029),(.177,.042,.009),'navy',.0015)
for i in range(10):
    box('Rail_BottomTooth_%02d'%i,(.249+i*.0175,0,-.036),(.011,.049,.008),'navy',.001)

# Front barrel collars and hollow, tiered muzzle shroud.
cyl('Barrel_Exposed',(.523,0,.020),.0115,.096,'steel','X',48)
for x,r,d in [(.494,.017,.011),(.508,.014,.008),(.555,.016,.012)]:
    cyl('Barrel_Collar',(x,0,.020),r,d,'navy','X',48)
shroud=tube('Muzzle_HollowShroud',(.59,0,.020),.025,.014,.067,'navy','X',48)
tube('Muzzle_RearAmberRing',(.560,0,.020),.0248,.014,.007,'orange')
tube('Muzzle_FrontSteelLip',(.625,0,.020),.025,.014,.005,'steel')
for i in range(8):
    a=i*math.tau/8
    y,z=.0237*math.sin(a),.020+.0237*math.cos(a)
    p=box('Muzzle_LongitudinalInset_%02d'%i,(.591,y,z),(.041,.006,.002),'dark',.0008)
    p.rotation_euler[0]=-a
    p=box('Muzzle_AmberTip_%02d'%i,(.612,y,z),(.008,.005,.0023),'orange',.0006)
    p.rotation_euler[0]=-a

# Sculpted short foregrip and layered textured insert.
box('Foregrip_RailClamp',(.312,0,-.048),(.054,.060,.024),'steel',.003)
profile('Foregrip_Main',[(.282,-.054),(.336,-.054),(.33,-.074),(.30,-.167),(.274,-.170),(.261,-.157),(.284,-.083)],.048,'navy',.004)
for s in (-1,1):
    p=profile('Foregrip_RubberPanel_'+str(s),[(.286,-.079),(.317,-.076),(.294,-.151),(.275,-.151)],.003,'rubber',.002)
    p.location.y=s*.024
    screw('Foregrip_ClampBolt',(.315,s*.032,-.050),.004)
    for j in range(7):
        z=-.091-j*.008
        x=.300+(z+.091)*.26
        o=box('Foregrip_Traction',(x,s*.026,z),(.018,.0025,.003),'teal',.001)
        o.rotation_euler[1]=-.24
box('Foregrip_AmberBase',(.283,0,-.166),(.028,.049,.008),'orange',.002)

api={k:globals()[k] for k in ('box','cyl','tube','mesh','profile','text','mats')}
from furniture import build_furniture
from accessories import build_accessories
build_furniture(api)
build_accessories(api)

# Keep the editable source convenient to pose, hide and recolor by component.
part_prefixes=[('Magazine_','Magazine',(.12,0,-.08)),('Stock_','Stock',(-.154,0,.025)),
    ('Grip_','PistolGrip',(-.07,0,-.06)),('Foregrip_','Foregrip',(.312,0,-.04)),
    ('Muzzle_','Muzzle',(.56,0,.02)),('Trigger_','Trigger',(.012,0,-.055)),
    ('ACC_Optic_','Optic',(0,0,.108)),('ACC_SideModule_','SideModule',(.37,-.05,.05)),
    ('ACC_RearIron_','RearSight',(-.118,0,.108)),('ACC_FrontIron_','FrontSight',(.44,0,.108))]
part_roots={}
for prefix,name,pos in part_prefixes+[('','ReceiverAndHandguard',(0,0,0))]:
    g=bpy.data.objects.new('PART_'+name,None)
    asset.objects.link(g)
    g.parent=root
    g.location=pos
    g.empty_display_size=.025
    part_roots[name]=g
bpy.context.view_layer.update()
for o in list(asset.objects):
    if o.type not in ('MESH','FONT'):continue
    name=next((name for prefix,name,pos in part_prefixes if o.name.startswith(prefix)),'ReceiverAndHandguard')
    world=o.matrix_world.copy()
    o.parent=part_roots[name]
    o.matrix_world=world
root['detachable_parts']=11
root['unrigged_visual_prop']=True

# Asset Browser metadata is part of the source .blend.
asset.asset_mark()
asset.asset_data.description='TIDELINE AR-01 | original detailed stylized modular assault rifle | metres +X muzzle +Z up'
asset.asset_data.author='MCP_Test'
for t in ('Weapon','Stylized','Assault Rifle','Modular','TIDELINE'):
    asset.asset_data.tags.new(t)
for key in ('navy','teal','ivory','orange','rubber','steel','glass'):
    mats[key].asset_mark()

# Studio is excluded from exports. Wide lights make round bevels readable.
active_collection=studio
floor=material('StudioBackdrop',(.72,.79,.78),0,.86)
box('Studio_Ground',(.08,0,-.344),(200,200,.015),floor,.0)
world=bpy.data.worlds.new('AR01_StudioWorld')
world.use_nodes=True
wn=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND')
wn.inputs['Color'].default_value=(.16,.24,.30,1)
wn.inputs['Strength'].default_value=.42
scene.world=world


def point_at(o,target):
    o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()


def light(name,loc,power,size,color,target=(.08,0,-.025)):
    d=bpy.data.lights.new(name,'AREA')
    d.energy=power
    d.shape='DISK' if 'DISK' in [i.identifier for i in d.bl_rna.properties['shape'].enum_items] else d.shape
    d.size=size
    d.color=color
    o=bpy.data.objects.new(name,d)
    studio.objects.link(o)
    o.location=loc
    point_at(o,target)


light('Studio_Key',(.0,-1.0,1.5),150,1.35,(1,.91,.79))
light('Studio_Fill',(.5,.8,.8),90,1.0,(.72,.90,1))
light('Studio_Rim',(-.65,.4,.45),100,.8,(.72,1,.90))
camdata=bpy.data.cameras.new('AR01_PresentationCamera')
cam=bpy.data.objects.new('AR01_PresentationCamera',camdata)
studio.objects.link(cam)
cam.location=(1.13,-1.8,.83)
point_at(cam,(.085,0,-.035))
enum_set(camdata,'type','ORTHO')
camdata.ortho_scale=1.30
camdata.lens=55
scene.camera=cam
try:
    scene.render.engine='CYCLES'
except TypeError:
    pass
scene.cycles.samples=48
scene.cycles.use_denoising=True
scene.render.resolution_x=1800
scene.render.resolution_y=1120
scene.render.resolution_percentage=100
enum_set(scene.render.image_settings,'file_format','PNG')
enum_set(scene.view_settings,'view_transform','AgX')
scene.view_settings.exposure=-.85
# Color-management enums are dynamic; assignment checks the current OCIO config.
try:
    scene.view_settings.look='AgX - Medium High Contrast'
except TypeError:
    pass
scene.render.filepath=str(ROOT/'Preview_Hero.png')

# Human-facing asset metadata and stable reproducibility settings.
scene['asset_collection']='ASSET_TIDELINE_AR01'
scene['preview_only_collection']='STUDIO_Preview_Only'
scene['build_source']=str(ROOT/'Source/build_rifle.py')
scene['scope']='Single original cartoon tactical assault rifle; no animation rig.'
bpy.ops.object.select_all(action='DESELECT')
for o in asset.objects:
    if o.type=='MESH':o.select_set(True)
bpy.context.view_layer.objects.active=next(o for o in asset.objects if o.type=='MESH')
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_perspective='CAMERA'
        area.spaces.active.shading.type='MATERIAL'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'TIDELINE_AR01.blend'))
print(json.dumps({'stage':'model_built','objects':len(asset.objects),'source':str(ROOT/'TIDELINE_AR01.blend')},ensure_ascii=False))
