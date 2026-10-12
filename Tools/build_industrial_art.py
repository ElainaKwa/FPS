"""Blender-authored industrial kit. Run Blender --background --python this_file.

Editable objects/modifiers stay in blend libraries; evaluated meshes export to FBX.
Only owns ArtSource/FPSDemo/Industrial. Does not touch legacy sources or game assets.
"""
import bpy
import math
import json
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource/FPSDemo/Industrial'
OUT.mkdir(parents=True, exist_ok=True)
for folder in ('Exports', 'Previews', 'Icons'):
    (OUT / folder).mkdir(exist_ok=True)
PALETTE = {
    'Graphite': ((.065, .077, .083), .55, .36),
    'Steel': ((.30, .34, .35), .82, .29),
    'Polymer': ((.023, .030, .032), .0, .64),
    'Ceramic': ((.66, .68, .64), .12, .43),
    'Concrete': ((.29, .31, .30), .0, .84),
    'Yellow': ((.82, .59, .075), .25, .43),
    'Rubber': ((.015, .018, .021), .0, .83),
    'Fabric': ((.095, .115, .12), .0, .9),
    'Visor': ((.07, .15, .17), .72, .17),
    'Blue': ((.035, .26, .49), .20, .45),
    'Red': ((.48, .065, .042), .20, .48),
    'Light': ((.82, .88, .78), .0, .3),
}
MATS = {}
ASSETS = []
COLLECTION = None


def reset():
    global MATS
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1.0
    MATS = {}
    for name, (color, metal, rough) in PALETTE.items():
        mat = bpy.data.materials.new('MI_IND_' + name)
        mat.diffuse_color = (*color, 1)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Base Color'].default_value = (*color, 1)
        bsdf.inputs['Metallic'].default_value = metal
        bsdf.inputs['Roughness'].default_value = rough
        if name == 'Light':
            bsdf.inputs['Emission Color'].default_value = (*color, 1)
            bsdf.inputs['Emission Strength'].default_value = 2
        MATS[name] = mat


def group(name):
    global COLLECTION
    COLLECTION = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(COLLECTION)
    return COLLECTION


def finish(obj, name, mat, bevel=0):
    obj.name = name
    for col in list(obj.users_collection):
        col.objects.unlink(obj)
    COLLECTION.objects.link(obj)
    obj.data.materials.append(MATS[mat])
    if bevel:
        mod = obj.modifiers.new('Machined edges', 'BEVEL')
        mod.width = bevel
        mod.segments = 3
        mod.affect = 'EDGES'
        mod = obj.modifiers.new('Weighted corner normals', 'WEIGHTED_NORMAL')
        mod.keep_sharp = True
    return obj


def box(name, loc, size, mat='Graphite', bevel=.004, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if rot:
        obj.rotation_euler = rot
    return finish(obj, name, mat, min(bevel, min(size) * .22))


def cylinder(name, loc, radius, depth, mat='Steel', axis='Z', vertices=32):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    obj = bpy.context.object
    obj.rotation_euler = (0, math.pi / 2, 0) if axis == 'X' else ((math.pi / 2, 0, 0) if axis == 'Y' else (0, 0, 0))
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return finish(obj, name, mat, min(.002, radius * .15))


def profile(name, points, width, mat='Graphite', bevel=.003):
    # A side profile in X/Z, symmetric about Y.
    n = len(points)
    verts = [(x, y, z) for y in (-width/2, width/2) for x, z in points]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    finish(obj, name, mat, bevel)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    obj.select_set(False)
    return obj


def label(name, text, loc, size=.07, mat='Ceramic', rotation=(math.pi/2, 0, 0)):
    curve = bpy.data.curves.new(name, 'FONT')
    curve.body = text
    curve.size = size
    curve.extrude = .0004
    curve.align_x = 'CENTER'
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = rotation
    finish(obj, name, mat)
    return obj


def bolt(name, loc, radius=.007, axis='Y'):
    cylinder(name, loc, radius, .003, 'Steel', axis, 6)


def join_evaluated(col, name):
    bpy.ops.object.select_all(action='DESELECT')
    deps = bpy.context.evaluated_depsgraph_get()
    copies = []
    for src in list(col.objects):
        if src.type not in {'MESH', 'FONT'}:
            continue
        evaluated = src.evaluated_get(deps)
        mesh = bpy.data.meshes.new_from_object(evaluated, depsgraph=deps)
        obj = bpy.data.objects.new(name + '_part', mesh)
        bpy.context.scene.collection.objects.link(obj)
        obj.matrix_world = src.matrix_world.copy()
        obj.select_set(True)
        copies.append(obj)
    assert copies, name
    bpy.context.view_layer.objects.active = copies[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    # All exported assets have usable non-overlapping smart UVs; no external textures.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.1519, island_margin=.012)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def export(col, kind, name=None):
    name = name or col.name
    obj = join_evaluated(col, name)
    coords = [obj.matrix_world @ Vector(v) for v in obj.bound_box]
    lo = [min(v[i] for v in coords) for i in range(3)]
    hi = [max(v[i] for v in coords) for i in range(3)]
    obj.data.calc_loop_triangles()
    entry = dict(name=name, category=kind, file='Exports/' + name + '.fbx',
                 bounds_m={'min':lo, 'max':hi}, triangles=len(obj.data.loop_triangles),
                 materials=[s.material.name for s in obj.material_slots], uv_layers=len(obj.data.uv_layers))
    bpy.ops.export_scene.fbx(filepath=str(OUT / entry['file']), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE',
        axis_forward='X', axis_up='Z', use_space_transform=False,
        mesh_smooth_type='FACE', use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)
    bpy.data.objects.remove(obj, do_unlink=True)
    ASSETS.append(entry)
    return entry


def stud_rows(x_values, y, z):
    for i, x in enumerate(x_values):
        bolt('Captive fastener_%02d' % i, (x, y, z))


def trigger(x=0):
    profile('Pistol grip', [(x-.027,-.025),(x+.028,-.035),(x-.005,-.145),(x-.056,-.14)], .035, 'Polymer', .004)
    for j in range(5):
        box('Grip traction', (x-.016-j*.003, 0, -.07-j*.012), (.043,.037,.003), 'Rubber', .0005)
    box('Trigger guard lower',(x+.055,0,-.082),(.087,.013,.009),'Graphite',.003)
    box('Trigger guard front',(x+.095,0,-.054),(.012,.014,.052),'Graphite',.003,rot=(0,-.12,0))
    box('Trigger',(x+.043,0,-.05),(.008,.009,.038),'Steel',.002,rot=(0,-.2,0))


def muzzle(start, end, radius=.009, z=0):
    cylinder('Barrel', ((start+end)/2,0,z),radius,end-start,'Steel','X')
    cylinder('Muzzle device',(end-.008,0,z),radius*1.55,.032,'Graphite','X')
    cylinder('Bore',(end+.0081,0,z),radius*.64,.0008,'Rubber','X')
    for side in (-1,1):
        for x in (end-.019,end-.009):
            box('Brake port',(x,side*radius*1.51,z),(.005,.001,.009),'Rubber',.0002)


def rail(x0,x1,z=.047):
    box('Rail base',((x0+x1)/2,0,z),(x1-x0,.024,.009),'Graphite',.001)
    count=int((x1-x0)/.016)
    for i in range(count):
        box('Rail tooth',(x0+i*.016+.006,0,z+.006),(.009,.033,.007),'Steel',.001)


def sights(front, height):
    for side in (-1,1):
        box('Rear sight ear',(-.012,side*.012,height-.009),(.019,.007,.018),'Graphite',.001)
    box('Front sight base',(front,0,height*.5),(.012,.020,height),'Graphite',.002)
    box('Front post',(front,0,height-.006),(.006,.005,.012),'Ceramic',.0005)


def magazine(points,width=.031):
    profile('Magazine',points,width,'Graphite',.003)
    # Dark and bright longitudinal breaks retain readable silhouette.
    for side in (-1,1):
        for x,z in [(points[0][0]+.013,-.09),(points[0][0]+.028,-.105)]:
            box('Magazine stamped rib',(x,side*(width/2+.0005),z),(.004,.0015,.075),'Steel',.0005,rot=(0,-.10,0))


def weapon(kind):
    col=group('SM_IND_'+kind)
    pistol=kind in ('M9','HeavyPistol')
    if pistol:
        heavy=kind=='HeavyPistol'
        end=.25 if heavy else .20
        profile('Slide',[(-.053,-.005),(-.053,.026),(-.040,.042),(end-.014,.042),(end,.027),(end,-.008)],.050 if heavy else .041,'Steel' if heavy else 'Graphite',.002)
        box('Frame',(.073,0,-.020),(.242 if heavy else .203,.034,.025),'Polymer',.003)
        trigger(-.008)
        for side in (-1,1):
            for i in range(7):
                box('Slide serration',(-.033+i*.007,side*(.0252 if heavy else .0207),.019),(.002,.0015,.030),'Rubber',.0003,rot=(0,-.15,0))
            bolt('Grip fastener',(-.025,side*.0185,-.110),.004)
            box('Slide release',(.011,side*.022,-.008),(.029,.006,.006),'Steel',.001)
        muzzle(end-.010,end+.005,.009 if heavy else .007,.012)
        sights(end-.020,.056)
        box('Magazine heel',(-.035,0,-.147),(.056,.038,.013),'Rubber',.003)
        box('Ejection recess',(.066,-.021,.027),(.039,.002,.018),'Rubber',.001)
    else:
        smg=kind in ('MP5','UMP45')
        isak=kind=='AK'
        end=.385 if smg else (.59 if kind=='BR3' else .565)
        width=.054 if kind=='UMP45' else .049
        profile('Receiver',[(-.085,-.033),(-.085,.024),(-.060,.039),(.18,.039),(.205,.018),(.197,-.04),(.06,-.055)],width,'Polymer' if kind=='UMP45' else 'Graphite',.004)
        trigger(-.008)
        if kind=='MP5':
            cylinder('Roller receiver',(.045,0,.022),.024,.27,'Graphite','X')
            for side in (-1,1):
                cylinder('Retractable stock arm',(-.18,side*.02,.012),.0045,.24,'Steel','X',16)
            box('Stock pad',(-.315,0,-.024),(.021,.044,.099),'Rubber',.004)
            profile('Handguard',[(.16,.022),(.285,.022),(.31,-.009),(.27,-.046),(.17,-.044)],.060,'Polymer',.004)
        elif kind=='UMP45':
            box('Stock upper',(-.20,0,.011),(.245,.026,.022),'Polymer',.003)
            box('Stock lower',(-.20,0,-.059),(.245,.025,.020),'Polymer',.003)
            box('Stock pad',(-.325,0,-.02),(.025,.043,.115),'Rubber',.004)
            profile('Fore-end',[(.175,.036),(.30,.036),(.325,.017),(.30,-.04),(.175,-.04)],.059,'Polymer',.003)
        elif isak:
            profile('Fixed stock',[(-.36,.016),(-.10,.022),(-.075,-.020),(-.30,-.091),(-.362,-.091)],.044,'Polymer',.006)
            box('Butt pad',(-.365,0,-.035),(.018,.049,.12),'Rubber',.005)
            profile('Handguard',[(.18,.025),(.39,.025),(.405,-.017),(.365,-.04),(.18,-.04)],.060,'Ceramic',.004)
            cylinder('Gas tube',(.295,0,.04),.009,.27,'Graphite','X')
        else:
            cylinder('Buffer tube',(-.185,0,.012),.014,.22,'Steel','X')
            profile('Adjustable stock',[(-.354,.030),(-.23,.03),(-.17,.0),(-.19,-.030),(-.30,-.084),(-.356,-.084)],.049,'Polymer',.005)
            box('Butt pad',(-.36,0,-.027),(.019,.054,.133),'Rubber',.005)
            profile('Stock inset',[(-.323,.008),(-.251,.008),(-.225,-.013),(-.315,-.055)],.050,'Graphite',.003)
            profile('Handguard',[(.18,-.031),(.18,.033),(.407,.033),(.43,.015),(.43,-.023),(.407,-.035)],.060,'Ceramic' if kind=='M4' else 'Graphite',.005)
        if kind=='UMP45':
            magazine([(.105,-.037),(.166,-.037),(.166,-.252),(.105,-.252)],.034)
        elif isak:
            magazine([(.103,-.035),(.177,-.035),(.19,-.115),(.253,-.239),(.19,-.26),(.135,-.147)],.033)
        else:
            magazine([(.10,-.035),(.165,-.035),(.167,-.123),(.191,-.219),(.134,-.230),(.107,-.134)],.031)
        muzzle(.285 if smg else .40,end)
        if kind!='MP5':
            rail(-.04,.285 if smg else .407,.047)
        sights(.29 if smg else .42,.065 if smg else .077)
        for side in (-1,1):
            for i in range(5 if smg else 7):
                x=.20+i*(.017 if smg else .028)
                box('Cooling slot',(x,side*.0303,-.002),(.012 if smg else .018,.0018,.016),'Rubber',.002)
            stud_rows([-.048,.01,.11,.175], side*(width/2+.001), -.008)
            box('Selector lever',(-.017,side*(width/2+.004),-.017),(.018,.007,.005),'Steel',.001,rot=(0,.35,0))
        box('Ejection port',(.085,-width/2-.001,.014),(.068,.002,.020),'Rubber',.001)
        cylinder('Charging handle',(.077,-width/2-.02,.024),.006,.04,'Steel','Y',16)
        box('Identification stripe',(.12,width/2+.0015,-.020),(.035,.0015,.003),'Yellow',.0002)
    export(col,'Weapons')
    return col


def envkit():
    # Metrics and origins: meters in Blender, centimeter conversion at FBX import.
    col=group('SM_IND_Floor400')
    box('Slab',(0,0,-.10),(4,4,.20),'Concrete',.015)
    for y in (-1.96,1.96):
        box('Expansion strip',(0,y,.003),(3.92,.028,.006),'Rubber',.001)
    for x in (-1.96,1.96):
        box('Expansion strip',(x,0,.003),(.028,3.92,.006),'Rubber',.001)
    export(col,'Environment')
    col=group('SM_IND_Wall400')
    box('Wall',(0,0,1.5),(4,.30,3),'Concrete',.028)
    for x in (-1.87,0,1.87):
        box('Structural stile',(x,-.17,1.5),(.11,.085,3),'Graphite',.010)
    for side in (-1,1):
        box('Base armor',(0,side*.169,.30),(3.80,.065,.50),'Graphite',.012)
        box('Safety line',(0,side*.206,.60),(3.78,.009,.04),'Yellow',.001)
        for x in (-1.66,1.66):
            bolt('Wall fixing',(x,side*.206,.29),.023)
    export(col,'Environment')
    col=group('SM_IND_Pillar300')
    box('Column',(0,0,1.50),(.38,.38,3),'Graphite',.028)
    for z in (.10,1.3,2.9):
        box('Collar',(0,0,z),(.44,.44,.14),'Steel',.012)
    box('Column marker',(0,-.196,1.67),(.19,.013,.60),'Yellow',.006)
    export(col,'Environment')
    col=group('SM_IND_Cargo240')
    box('Shell',(0,0,.68),(2.4,1.25,1.36),'Graphite',.045)
    for side in (-1,1):
        box('Recess',(0,side*.633,.71),(2.10,.026,.91),'Polymer',.015)
        for x in (-.95,-.48,0,.48,.95):
            box('Side rib',(x,side*.672,.72),(.035,.065,1.04),'Steel',.006)
        box('Upper safety edge',(0,side*.678,1.27),(2.13,.018,.045),'Yellow',.004)
    for x in (-1.13,1.13):
        for y in (-.56,.56):
            box('Stacking block',(x,y,1.35),(.16,.16,.11),'Steel',.012)
            box('Stacking foot',(x,y,.065),(.19,.19,.13),'Steel',.012)
    box('Lid',(0,0,1.37),(2.14,1.03,.08),'Graphite',.018)
    for x in (-.88,.88):
        box('Latch',(x,-.681,1.08),(.14,.038,.16),'Steel',.012)
    export(col,'Environment')
    col=group('SM_IND_Barrier200')
    profile('Barrier body',[(-1,0),(1,0),(1,.84),(.91,1.02),(-.91,1.02),(-1,.84)],.55,'Concrete',.03)
    for side in (-1,1):
        box('Recessed face',(0,side*.278,.52),(1.69,.025,.50),'Graphite',.018)
        for i in range(7):
            box('Hazard slash',(-.72+i*.24,side*.294,.54),(.10,.008,.39),'Yellow',.003,rot=(0,.43,0))
    for x in (-.84,.84):
        box('Barrier foot',(x,0,.065),(.29,.75,.13),'Graphite',.014)
    export(col,'Environment')
    col=group('SM_IND_Railing200')
    for x in (-.95,.95):
        box('Post',(x,0,.55),(.065,.065,1.1),'Graphite',.008)
        box('Foot',(x,0,.025),(.19,.19,.05),'Steel',.004)
    for z in (.55,1.1):
        cylinder('Handrail',(0,0,z),.027,2,'Yellow','X')
    box('Toe plate',(0,0,.13),(1.90,.028,.17),'Graphite',.004)
    export(col,'Environment')
    col=group('SM_IND_Gateway400')
    for x in (-1.68,1.68):
        box('Jamb',(x,0,1.5),(.64,.48,3),'Concrete',.035)
        box('Jamb armor',(x,-.27,1.10),(.36,.095,1.35),'Graphite',.018)
        box('Jamb marker',(x,-.322,1.4),(.12,.014,.55),'Yellow',.003)
    box('Lintel',(0,0,2.86),(4,.48,.36),'Concrete',.035)
    box('Header inset',(0,-.257,2.84),(2.80,.036,.21),'Graphite',.007)
    for x in (-1,1):
        box('Work light',(x,-.29,2.81),(.30,.045,.045),'Light',.005)
    export(col,'Environment')
    col=group('SM_IND_Deck400')
    box('Deck',(0,0,-.10),(4,2,.20),'Graphite',.016)
    for x in (-1.93,1.93):
        box('Girder',(x,0,-.20),(.14,2,.40),'Steel',.015)
    for i in range(20):
        box('Grip strip',(-1.90+i*.20,0,.007),(.023,1.85,.014),'Steel',.003)
    export(col,'Environment')
    col=group('SM_IND_WeaponRack')
    box('Base',(0,0,.12),(1.50,.65,.24),'Graphite',.035)
    for x in (-.66,.66):
        box('Upright',(x,.17,.78),(.075,.10,1.35),'Steel',.008)
    box('Backplane',(0,.20,.91),(1.29,.075,.95),'Graphite',.022)
    for x in (-.42,0,.42):
        for z in (.68,1.20):
            box('Rubber cradle',(x,.025,z),(.13,.29,.055),'Rubber',.007)
    box('ID strip',(0,-.331,.17),(1.1,.015,.055),'Yellow',.004)
    export(col,'Environment')
    col=group('SM_IND_TargetStand')
    box('Base',(0,0,.05),(.65,.65,.1),'Graphite',.02)
    cylinder('Support',(0,.035,.66),.045,1.22,'Steel')
    profile('Target plate',[(-.30,.88),(.30,.88),(.30,1.50),(.15,1.67),(-.15,1.67),(-.30,1.50)],.045,'Ceramic',.012)
    cylinder('Bullseye',(0,-.029,1.27),.13,.013,'Red','Y',48)
    cylinder('Inner target',(0,-.037,1.27),.058,.015,'Graphite','Y',48)
    export(col,'Environment')
    col=group('SM_IND_WorkLight')
    box('Base',(0,0,.08),(.6,.55,.16),'Graphite',.025)
    cylinder('Mast',(0,0,1.60),.040,3,'Steel')
    box('Lamp casing',(0,-.04,3.12),(.65,.23,.23),'Graphite',.025)
    box('Diffuser',(0,-.167,3.12),(.54,.026,.15),'Light',.01)
    box('Service band',(0,0,.47),(.096,.096,.16),'Yellow',.007)
    export(col,'Environment')
    col=group('SM_IND_BaseBeacon')
    box('Backplate',(0,0,.70),(.6,.13,1.4),'Graphite',.022)
    box('Team panel',(0,-.078,.80),(.47,.028,.89),'Blue',.011)
    label('Functional identifier','A',(0,-.099,.65),.37)
    box('Signal light',(0,-.095,1.30),(.42,.024,.035),'Light',.004)
    export(col,'Environment')


def armor_segment(name,length,radius):
    # +Z local segment, made in anatomical layout below rather than 1cm stretch mesh.
    cylinder(name+' inner',(0,0,length*.5),radius*.70,length,'Fabric')
    box(name+' front armor',(0,-radius*.40,length*.52),(radius*1.75,radius*1.15,length*.72),'Ceramic',radius*.25)
    box(name+' back shell',(0,radius*.38,length*.48),(radius*1.5,radius*.7,length*.58),'Graphite',radius*.18)
    for z in (.10*length,.86*length):
        cylinder(name+' seal',(0,0,z),radius*.84,length*.075,'Rubber')
    box(name+' yellow notch',(-radius*.43,-radius*.998,length*.66),(radius*.30,.006,length*.15),'Yellow',.001)


def operator():
    parts={}
    def begin(name):
        parts[name]=group(name)
    begin('Pelvis')
    box('Hip soft shell',(0,0,.91),(.31,.22,.20),'Fabric',.065)
    box('Utility belt',(0,0,1.0),(.34,.25,.065),'Graphite',.016)
    box('Buckle',(0,-.135,1.0),(.065,.023,.045),'Steel',.006)
    for side in (-1,1):
        box('Hip plate',(side*.165,0,.91),(.067,.22,.17),'Ceramic',.018)
        box('Utility pouch',(side*.11,-.16,.95),(.11,.065,.10),'Fabric',.012)
    begin('Torso')
    box('Undersuit',(0,0,1.20),(.38,.235,.37),'Fabric',.078)
    box('Back shell',(0,.12,1.22),(.33,.10,.32),'Graphite',.035)
    profile('Chest plate',[(-.19,1.15),(.19,1.15),(.215,1.34),(.14,1.42),(-.14,1.42),(-.215,1.34)],.06,'Ceramic',.018)
    for obj in list(COLLECTION.objects):
        if obj.name.startswith('Chest plate'):
            obj.location.y=-.145
    box('Chest inlay',(0,-.183,1.30),(.23,.019,.075),'Graphite',.008)
    box('Chest marker',(-.095,-.196,1.31),(.028,.01,.040),'Yellow',.002)
    for side in (-1,1):
        box('Abdominal plates',(side*.092,-.142,1.12),(.15,.046,.072),'Graphite',.012)
        box('Pack rail',(side*.105,.183,1.24),(.028,.045,.27),'Steel',.007)
    cylinder('Neck seal',(0,0,1.455),.077,.10,'Rubber')
    begin('Head')
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=20, location=(0,0,1.61))
    obj=bpy.context.object
    obj.scale=(.121,.119,.151)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    finish(obj,'Helmet main shell','Ceramic')
    box('Visor surround',(0,-.093,1.64),(.236,.081,.115),'Graphite',.030)
    box('Dark visor',(0,-.135,1.647),(.200,.024,.063),'Visor',.018)
    box('Brow',(0,-.123,1.705),(.238,.054,.025),'Ceramic',.007)
    box('Respirator',(0,-.111,1.55),(.125,.078,.066),'Graphite',.018)
    for x in (-.036,-.018,0,.018,.036):
        box('Breathing grille',(x,-.153,1.55),(.005,.005,.035),'Steel',.001)
    for side in (-1,1):
        cylinder('Ear module',(side*.120,0,1.61),.046,.031,'Graphite','X')
        box('Helmet stripe',(side*.083,-.153,1.704),(.016,.008,.019),'Yellow',.002)
    for side,sgn in [('L',-1),('R',1)]:
        begin('UpperArm_'+side)
        armor_segment('Upper arm',.27,.066)
        for obj in list(COLLECTION.objects):
            obj.location += Vector((sgn*.266,0,1.075))
        cap=profile('Shoulder cap',[(-.095,0),(.095,0),(.085,.080),(.045,.125),(-.055,.105)],.225,'Ceramic',.015)
        cap.location=(sgn*.263,0,1.31)
        begin('Forearm_'+side)
        armor_segment('Forearm',.25,.051)
        for obj in list(COLLECTION.objects):
            obj.location += Vector((sgn*.266,0,.798))
        box('Wrist terminal',(sgn*.266,-.064,.965),(.066,.026,.065),'Graphite',.008)
        begin('Hand_'+side)
        box('Palm',(sgn*.266,-.004,.74),(.091,.05,.115),'Fabric',.016)
        box('Knuckle plate',(sgn*.266,-.036,.767),(.085,.023,.044),'Graphite',.011)
        for i in range(4):
            x=sgn*.266-.034+i*.022
            box('Finger',(x,-.011,.665),(.019,.039,.067),'Rubber',.007)
        box('Thumb',(sgn*.323,-.016,.724),(.033,.04,.070),'Rubber',.010,rot=(0,sgn*.30,0))
        begin('Thigh_'+side)
        armor_segment('Thigh',.37,.092)
        for obj in list(COLLECTION.objects):
            obj.location += Vector((sgn*.111,0,.47))
        begin('Shin_'+side)
        armor_segment('Shin',.34,.069)
        for obj in list(COLLECTION.objects):
            obj.location += Vector((sgn*.111,0,.105))
        box('Knee cap',(sgn*.111,-.057,.468),(.145,.096,.115),'Ceramic',.028)
        begin('Foot_'+side)
        box('Boot upper',(sgn*.111,-.047,.092),(.13,.265,.14),'Graphite',.034)
        box('Sole',(sgn*.111,-.047,.027),(.143,.282,.046),'Rubber',.010)
        box('Toe guard',(sgn*.111,-.145,.091),(.126,.095,.072),'Ceramic',.018)
        for i in range(5):
            box('Tread',(sgn*.111,-.15+i*.046,.007),(.146,.012,.012),'Rubber',.002)
    # Export one coherent standing static mesh and a rigged version with rigid armor weights.
    merged=group('SM_IND_Operator')
    for col in parts.values():
        for obj in list(col.objects):
            merged.objects.link(obj)
    export(merged,'Character')
    bpy.data.collections.remove(merged)
    bpy.ops.object.armature_add(enter_editmode=True)
    rig=bpy.context.object
    rig.name='SK_IND_Operator'
    bones=rig.data.edit_bones
    bones.remove(bones[0])
    def bone(name,head,tail,parent=None):
        b=bones.new(name); b.head=head; b.tail=tail
        if parent:b.parent=bones[parent]
    bone('Root',(0,0,0),(0,0,.2))
    bone('Pelvis',(0,0,.88),(0,0,1.04),'Root')
    bone('Torso',(0,0,1.04),(0,0,1.43),'Pelvis')
    bone('Head',(0,0,1.45),(0,0,1.76),'Torso')
    for side,sgn in [('L',-1),('R',1)]:
        bone('UpperArm_'+side,(sgn*.266,0,1.38),(sgn*.266,0,1.06),'Torso')
        bone('Forearm_'+side,(sgn*.266,0,1.06),(sgn*.266,0,.79),'UpperArm_'+side)
        bone('Hand_'+side,(sgn*.266,0,.79),(sgn*.266,0,.65),'Forearm_'+side)
        bone('Thigh_'+side,(sgn*.111,0,.89),(sgn*.111,0,.47),'Pelvis')
        bone('Shin_'+side,(sgn*.111,0,.47),(sgn*.111,0,.105),'Thigh_'+side)
        bone('Foot_'+side,(sgn*.111,0,.105),(sgn*.111,-.17,.05),'Shin_'+side)
    bpy.ops.object.mode_set(mode='OBJECT')
    rigged=[]
    for bonename,col in parts.items():
        obj=join_evaluated(col,'Skin_'+bonename)
        vg=obj.vertex_groups.new(name=bonename)
        vg.add(list(range(len(obj.data.vertices))),1.0,'REPLACE')
        mod=obj.modifiers.new('Armor rig','ARMATURE'); mod.object=rig
        obj.parent=rig
        rigged.append(obj)
        # Hide authoring subparts in viewport/render, keeping editable collections.
        col.hide_render=True; col.hide_viewport=True
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    for obj in rigged:obj.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports/SK_IND_Operator.fbx'),use_selection=True,
        object_types={'MESH','ARMATURE'},axis_forward='X',axis_up='Z',use_space_transform=False,
        add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE',use_armature_deform_only=True)
    ASSETS.append(dict(name='SK_IND_Operator',category='Character',file='Exports/SK_IND_Operator.fbx',skeletal=True,bones=len(rig.data.bones)))
    # Separate arm/hand static resources for first-person presentation workflows.
    for side in ('L','R'):
        col=group('SM_IND_FPArm_'+side)
        for bonename in ('Forearm_'+side,'Hand_'+side):
            src=next(o for o in rigged if o.name=='Skin_'+bonename)
            dup=src.copy(); dup.data=src.data.copy(); dup.parent=None
            dup.modifiers.clear(); col.objects.link(dup)
        export(col,'Character')
        col.hide_render=True; col.hide_viewport=True
    return rig


def lights_camera(target,location,ortho,filepath):
    scene=bpy.context.scene
    scene.render.engine='CYCLES'
    scene.cycles.samples=32
    scene.cycles.use_denoising=True
    scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('Studio')
    scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.15,.17,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.4
    col=group('Studio')
    for name,pos,power,size in [('Key',(3,-4,6),1700,5),('Fill',(-4,-1,3),1000,4),('Rim',(1,4,5),2000,3)]:
        data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size
        obj=bpy.data.objects.new(name,data);col.objects.link(obj);obj.location=pos
        obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    data=bpy.data.cameras.new('PreviewCamera');cam=bpy.data.objects.new('PreviewCamera',data);col.objects.link(cam)
    cam.location=location;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=ortho;scene.camera=cam
    scene.render.filepath=str(filepath)
    scene.view_settings.view_transform='AgX'
    return col


def run():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    mode=args[0] if args else 'all'
    old=json.loads((OUT/'manifest.json').read_text()) if (OUT/'manifest.json').exists() else {'assets':[]}
    touched=[]
    if mode in ('all','weapons'):
        reset()
        for i,kind in enumerate(('M4','AK','BR3','MP5','UMP45','M9','HeavyPistol')):
            col=weapon(kind)
            for obj in list(col.objects):
                obj.location.x+=(i%2)*1.13
                obj.location.z+=(3-i//2)*.45
        lights_camera((.65,0,.66),(1.3,-5,1.65),2.95,OUT/'Previews/Weapons.png')
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WeaponLibrary.blend'))
        bpy.ops.render.render(write_still=True)
        touched.append('Weapons')
    if mode in ('all','environment'):
        reset();envkit()
        for i,col in enumerate([c for c in bpy.data.collections if c.name.startswith('SM_IND_')]):
            for obj in list(col.objects):
                obj.location.x+=(i%4)*4.6;obj.location.y+=(i//4)*4.8
        lights_camera((6.9,4.8,1),(18,-16,19),22,OUT/'Previews/Environment.png')
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'EnvironmentKit.blend'))
        bpy.ops.render.render(write_still=True)
        touched.append('Environment')
    if mode in ('all','character'):
        reset();operator()
        lights_camera((0,0,.90),(2.7,-4.0,2.1),3.12,OUT/'Previews/Operator.png')
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Operator.blend'))
        bpy.ops.render.render(write_still=True)
        touched.append('Character')
    old['assets']=[a for a in old['assets'] if a['category'] not in touched]+ASSETS
    old.update(generator='Tools/build_industrial_art.py',blender=bpy.app.version_string,
               units='meters in blend; centimeters in Unreal',materials={name:{'color':c,'metallic':m,'roughness':r} for name,(c,m,r) in PALETTE.items()})
    (OUT/'manifest.json').write_text(json.dumps(old,indent=2)+'\n',encoding='utf-8')
    print('INDUSTRIAL_ART_COMPLETE',len(ASSETS),flush=True)


if __name__=='__main__':run()
