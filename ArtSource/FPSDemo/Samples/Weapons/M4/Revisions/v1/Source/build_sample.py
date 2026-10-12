"""M4 exterior visual-study asset. Run with Blender --background --factory-startup.

Isolated art sample: no game-content writes and no internal operating mechanism.
All exterior dimensions are art proportions chosen for the supplied FPS view.
"""
import bpy, bmesh, math, json, random
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
random.seed(112)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.name = 'M4_Exterior_Study'
scene.unit_settings.system = 'METRIC'
scene['purpose'] = 'Standalone exterior art sample; not integrated into Unreal'
asset = bpy.data.collections.new('M4_PRECISION_SAMPLE')
scene.collection.children.link(asset)
asset.asset_mark()
asset.asset_data.description = 'M4 inspired cinematic exterior study with editable parts and procedural surface materials'
groups = {}
for name in ['Receiver', 'Handguard', 'Barrel', 'Stock', 'Grip', 'Magazine', 'Optic', 'Sights', 'Markings']:
    col = bpy.data.collections.new('M4_'+name)
    asset.children.link(col)
    groups[name] = col
current = groups['Receiver']

def material(name, color, metal, rough, grain=0):
    m=bpy.data.materials.new('M4_'+name);m.use_nodes=True
    m.diffuse_color=(*color,1)
    nodes=m.node_tree.nodes;links=m.node_tree.links
    bs=nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1)
    bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough
    if grain:
        tex=nodes.new('ShaderNodeTexCoord')
        n=nodes.new('ShaderNodeTexNoise');n.inputs['Scale'].default_value=1800
        n.inputs['Detail'].default_value=2
        links.new(tex.outputs['Object'],n.inputs['Vector'])
        b=nodes.new('ShaderNodeBump');b.inputs['Strength'].default_value=.22
        b.inputs['Distance'].default_value=grain
        links.new(n.outputs['Fac'],b.inputs['Height']);links.new(b.outputs['Normal'],bs.inputs['Normal'])
        n2=nodes.new('ShaderNodeTexNoise');n2.inputs['Scale'].default_value=110
        n2.inputs['Detail'].default_value=3
        links.new(tex.outputs['Object'],n2.inputs['Vector'])
        ramp=nodes.new('ShaderNodeMapRange');ramp.inputs['To Min'].default_value=rough-.045;ramp.inputs['To Max'].default_value=rough+.055
        links.new(n2.outputs['Fac'],ramp.inputs['Value']);links.new(ramp.outputs['Result'],bs.inputs['Roughness'])
    return m

mats={
 'body':material('01_Anodized_Graphite',(.032,.038,.043),.78,.33,.000022),
 'guard':material('02_Cerakote_Charcoal',(.023,.029,.032),.64,.40,.000028),
 'steel':material('03_Parkerized_Steel',(.048,.054,.057),.86,.30,.000018),
 'edge':material('04_Burnished_Hardware',(.14,.15,.155),.9,.27,.000008),
 'poly':material('05_Moulded_Polymer',(.019,.023,.025),0,.51,.000068),
 'rubber':material('06_Grip_Rubber',(.010,.012,.013),0,.69,.0001),
 'dark':material('07_Recess',(.004,.006,.007),.05,.68),
 'marks':material('08_Laser_Marking',(.31,.33,.31),.15,.50),
 'yellow':material('09_Inspection_Paint',(.52,.31,.045),.05,.48),
 'glass':material('10_Optical_Glass',(.10,.24,.20),0,.07),
}
glass=mats['glass'].node_tree.nodes.get('Principled BSDF')
glass.inputs['Transmission Weight'].default_value=.98;glass.inputs['IOR'].default_value=1.46

def place(o,name,mat):
    o.name=name
    for c in list(o.users_collection):c.objects.unlink(o)
    current.objects.link(o)
    if mat:o.data.materials.append(mats[mat])
    return o
def bevel(o,w=.001,segments=3):
    if w:
        b=o.modifiers.new('Machined edge radius','BEVEL');b.width=w;b.segments=segments;b.limit_method='ANGLE';b.harden_normals=True
        n=o.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL');n.keep_sharp=True;n.weight=50
    return o
def mesh(name,verts,faces,mat,w=.001):
    d=bpy.data.meshes.new(name+'_Mesh');d.from_pydata(verts,[],faces);d.update()
    bm=bmesh.new();bm.from_mesh(d);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(d);bm.free()
    o=bpy.data.objects.new(name,d);current.objects.link(o);d.materials.append(mats[mat]);return bevel(o,w)
def box(name,loc,dims,mat,w=.001):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=place(bpy.context.object,name,mat);o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return bevel(o,min(w,min(dims)*.35))
def profile(name,points,depth,mat,w=.001,y=0):
    n=len(points);v=[(x,yy+y,z) for yy in (-depth/2,depth/2) for x,z in points]
    f=[tuple(reversed(range(n))),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name,v,f,mat,w)
def cyl(name,loc,r,length,mat,axis='X',verts=64,w=.0004):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=length,location=loc)
    o=place(bpy.context.object,name,mat)
    if axis=='X':o.rotation_euler.y=math.pi/2
    elif axis=='Y':o.rotation_euler.x=math.pi/2
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
    return bevel(o,w)
def tube(name,loc,r,inner,length,mat,axis='X',N=64,w=.0003):
    v=[]
    for x,rr in [(-length/2,r),(length/2,r),(-length/2,inner),(length/2,inner)]:
        for i in range(N):
            a=math.tau*i/N;p=(x,rr*math.cos(a),rr*math.sin(a))
            if axis=='Y':p=(p[1],p[0],p[2])
            v.append(tuple(loc[j]+p[j] for j in range(3)))
    f=[]
    for i in range(N):
        j=(i+1)%N
        f.extend([(i,j,N+j,N+i),(2*N+i,3*N+i,3*N+j,2*N+j),(i,2*N+i,2*N+j,j),(N+i,N+j,3*N+j,3*N+i)])
    o=mesh(name,v,f,mat,w)
    for i,p in enumerate(o.data.polygons):p.use_smooth=i%4<2
    return o
def apply(o):
    bpy.context.view_layer.objects.active=o
    for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
def cut(o,cutters):
    for c in cutters:apply(c)
    bpy.ops.object.select_all(action='DESELECT')
    for c in cutters:c.select_set(True)
    bpy.context.view_layer.objects.active=cutters[0]
    if len(cutters)>1:bpy.ops.object.join()
    c=bpy.context.object;bpy.context.view_layer.objects.active=o
    m=o.modifiers.new('Exterior openings','BOOLEAN');m.operation='DIFFERENCE';m.solver='EXACT';m.object=c
    bpy.ops.object.modifier_apply(modifier=m.name);bpy.data.objects.remove(c,do_unlink=True)
def line(name,points,r,mat):
    d=bpy.data.curves.new(name,'CURVE');d.dimensions='3D';d.resolution_u=2;d.bevel_depth=r;d.bevel_resolution=3
    s=d.splines.new('POLY');s.points.add(len(points)-1)
    for p,co in zip(s.points,points):p.co=(*co,1)
    o=bpy.data.objects.new(name,d);current.objects.link(o);d.materials.append(mats[mat]);return o
def text(name,body,loc,size,mat='marks',side=-1):
    d=bpy.data.curves.new(name,'FONT');d.body=body;d.size=size;d.extrude=.000012
    d.space_character=1.1
    o=bpy.data.objects.new(name,d);current.objects.link(o);o.location=loc
    o.rotation_euler=(math.pi/2,0,0) if side<0 else (math.pi/2,0,math.pi)
    d.materials.append(mats[mat]);return o
def screw(name,x,y,z,r=.003,side=-1):
    head=cyl(name,(x,y,z),r,.0016,'steel','Y',48,0)
    socket=cyl('cut_hex',(x,y+side*.0008,z),r*.49,.0012,'dark','Y',6,0)
    cut(head,[socket]);bevel(head,.00015,2)
    return head
def rail(name,x0,x1,z,width=.027,pitch=.010):
    box(name+'_Spine',((x0+x1)/2,0,z-.003),(x1-x0,width*.65,.009),'body',.0004)
    n=int((x1-x0)/pitch)
    for i in range(n):
        x=x0+(i+.5)*(x1-x0)/n
        verts=[(xx,yy,zz) for xx in (x-.0031,x+.0031) for yy,zz in [(-width*.5,z),(-width*.42,z+.0035),(width*.42,z+.0035),(width*.5,z),(width*.37,z-.003),(-width*.37,z-.003)]]
        mesh(name+f'_Tooth_{i:02d}',verts,[tuple(reversed(range(6))),tuple(range(6,12))]+[(j,(j+1)%6,(j+1)%6+6,j+6) for j in range(6)],'body',.00025)

# Upper receiver: contoured forging, seam, separate ejection port and controls.
current=groups['Receiver']
upper=profile('Upper_receiver_forging',[(-.142,.052),(-.13,.066),(.125,.066),(.15,.047),(.145,.005),(.125,-.007),(-.132,-.007),(-.145,.006)],.052,'body',0)
port=box('cut_ejection',(.054,-.029,.030),(.070,.026,.022),'dark',.004)
cut(upper,[port]);bevel(upper,.005,6)
box('Ejection_port_shadow',(.054,-.016,.030),(.067,.002,.018),'dark',.002)
cyl('Bolt_visible_surface',(.047,-.020,.029),.011,.060,'steel','X',48,.0005)
cover=box('Dust_cover_open',(.054,-.042,.008),(.077,.0019,.025),'body',.001)
cover.rotation_euler.x=math.radians(58)
cyl('Dust_cover_hinge',(.054,-.028,.017),.0025,.084,'steel','X',32,.00025)
for x in [.017,.032,.074,.09]:cyl('Hinge_sleeve',(x,-.028,.017),.0033,.007,'body','X',32,.0002)
profile('Brass_deflector',[(.000,.044),(-.014,.042),(-.009,.015),(.006,.013)],.010,'body',.0015,y=-.029)
cyl('Forward_assist_body',(-.051,-.027,.032),.008,.031,'body','X',48,.0008)
cyl('Forward_assist_cap',(-.069,-.027,.032),.009,.007,'poly','X',48,.0005)
for z in [.027,.031,.035,.039]:box('Assist_cap_rib',(-.073,-.027,z),(.0015,.014,.0008),'poly',.0002)
lower=profile('Lower_receiver_forging',[(-.139,-.009),(.145,-.009),(.138,-.069),(.032,-.081),(.010,-.052),(-.076,-.052),(-.089,-.034),(-.137,-.029)],.047,'body',.0017)
profile('Magazine_well_flare',[(.025,-.041),(.142,-.040),(.143,-.078),(.030,-.090)],.056,'body',.0016)
for side in [-1,1]:
    y=side*.0246
    screw('Pivot_pin',.125,y,-.017,.0045,side);screw('Rear_pin',-.118,y,-.015,.0041,side)
    for x,z in [(-.063,-.025),(-.022,-.025)]:cyl('Trigger_group_pin',(x,y,z),.0021,.0015,'steel','Y',32,.0001)
    cyl('Selector_hub',(-.092,y,-.018),.006,.003,'steel','Y',48,.0004)
    lever=box('Selector_lever',(-.102,y+side*.002,-.021),(.021,.004,.005),'steel',.001)
    lever.rotation_euler.y=-.2
    text('Selector_safe','SAFE',(-.112,y+side*.002,-.004),.0042,side=side)
    text('Selector_fire','SEMI',(-.083,y+side*.002,-.004),.0042,side=side)
box('Magazine_release_recess',(.026,-.027,-.024),(.018,.002,.012),'dark',.003)
box('Magazine_release_button',(.026,-.029,-.024),(.012,.003,.009),'steel',.002)
for x in [.022,.025,.028,.031]:box('Release_serration',(x,-.0307,-.024),(.00065,.0003,.006),'edge',.0001)
box('Bolt_catch_plate',(-.009,.027,.002),(.014,.004,.028),'steel',.001)
for z in [-.005,0,.005,.01]:box('Bolt_catch_rib',(-.009,.030,z),(.011,.001,.001),'edge',.0002)
guard=profile('Trigger_guard',[(-.086,-.049),(.023,-.051),(.022,-.087),(-.074,-.085),(-.09,-.069)],.013,'body',0)
cut(guard,[profile('cut_trigger_window',[(-.078,-.052),(.014,-.054),(.014,-.077),(-.07,-.077),(-.079,-.067)],.04,'dark',.003)])
bevel(guard,.0015,4)
line('Trigger_visible',[(-.038,0,-.043),(-.031,0,-.054),(-.033,0,-.067),(-.041,0,-.074)],.0027,'steel')
box('Charging_handle_spine',(-.145,0,.058),(.026,.022,.009),'steel',.001)
for side in [-1,1]:box('Charging_handle_latch',(-.15,side*.022,.058),(.018,.021,.012),'body',.0018)
rail('Receiver_top_rail',-.138,.137,.073)
text('Receiver_identifier','M4  /  FIELD SERIES',(.037,.029,-.061),.0052,side=1)
text('Receiver_serial','IND-0417',(.091,-.0291,-.069),.0038)
# Forged shoulders and recessed panels break up the receiver's broad surfaces.
for side in [-1,1]:
    profile('Receiver_forged_shoulder',[(-.131,.048),(-.113,.061),(.120,.061),(.134,.048)],.006,'body',.002,y=side*.021)
    profile('Magwell_side_panel',[(.039,-.047),(.131,-.046),(.132,-.072),(.045,-.083)],.0008,'guard',.001,y=side*.0285)
    line('Lower_forging_edge',[(-.130,side*.024,-.029),(-.091,side*.024,-.033),(-.076,side*.024,-.048),(.009,side*.024,-.048)],.0011,'body')

# Vented octagonal handguard; actual side and diagonal apertures.
current=groups['Handguard']
N=8;angles=[math.pi/8+math.tau*i/N for i in range(N)]
verts=[]
for x,rad in [(.143,.038),(.443,.0335),(.143,.0315),(.443,.027)]:
    verts.extend([(x,rad*math.cos(a),.032+rad*math.sin(a)) for a in angles])
faces=[]
for i in range(N):
    j=(i+1)%N
    faces.extend([(i,j,N+j,N+i),(2*N+i,3*N+i,3*N+j,2*N+j),(i,2*N+i,2*N+j,j),(N+i,N+j,3*N+j,3*N+i)])
hand=mesh('Handguard_machined_shell',verts,faces,'guard',0)
holes=[]
for i in range(7):
    x=.177+i*.037
    holes.append(box('cut_side_slot',(x,0,.033),(.026,.14,.012),'dark',.0045))
    for a in [-math.pi/4,math.pi/4]:
        c=box('cut_diagonal_slot',(x,0,.032),(.025,.12,.010),'dark',.003)
        c.rotation_euler.x=a;holes.append(c)
cut(hand,holes);bevel(hand,.00065,3)
tube('Handguard_rear_collar',(.145,0,.032),.0375,.029,.019,'body',N=64,w=.0007)
for side in [-1,1]:
    for x in [.151,.170]:screw('Handguard_clamp_bolt',x,side*.035,.009,.0038,side)
    box('Clamp_side_plate',(.159,side*.033,.01),(.043,.004,.023),'body',.002)
rail('Handguard_top_rail',.153,.440,.073)
# Short side accessory rails leave the rear vents open.
for side in [-1,1]:
    y=side*.036
    box('Side_rail_backplate',(.361,y,.031),(.12,.006,.020),'body',.001)
    for i in range(12):box('Side_rail_lug',(.307+i*.0097,y+side*.004,.031),(.005,.005,.025),'body',.0004)
    for x in [.31,.41]:screw('Side_rail_screw',x,y+side*.007,.031,.003,side)
    for i in range(3):
        x=.211+i*.037
        text('Vent_index',str(i+1),(x,side*.035,.015),.0038,side=side)
for i in range(23):box('Bottom_rail_lug',(.190+i*.010,0,-.009),(.006,.026,.007),'body',.0004)
for i in range(7):
    t=text('Top_rail_index',f'T{2*i+4}',(.168+i*.039,-.005,.0771),.0028)
    t.rotation_euler=(0,0,0)

current=groups['Barrel']
cyl('Barrel_exterior',(.357,0,.032),.0105,.413,'steel','X',64,.00045)
for x,r,depth in [(.173,.018,.035),(.465,.014,.020),(.541,.016,.015)]:cyl('Barrel_step',(x,0,.032),r,depth,'steel','X',64,.0006)
cyl('Gas_tube_visible',(.312,0,.058),.0022,.35,'steel','X',24,.0001)
box('Front_gas_block',(.463,0,.038),(.025,.028,.046),'body',.003)
muzzle=tube('Muzzle_shroud',(.584,0,.032),.018,.010,.058,'steel',N=64,w=0)
cuts=[]
for a in [0,math.pi/2,math.pi,3*math.pi/2]:
    c=box('cut_muzzle_slot',(.586,.017*math.cos(a),.032+.017*math.sin(a)),(.028,.018,.006),'dark',.001)
    c.rotation_euler.x=a;cuts.append(c)
cut(muzzle,cuts);bevel(muzzle,.00045,3)
tube('Muzzle_lip',(.613,0,.032),.0178,.0105,.006,'steel')
# Dark shallow visual cap; no internal bore or operating parts.
cyl('Muzzle_recess_shadow',(.566,0,.032),.01,.002,'dark','X',48,.0001)

# Telescoping furniture: open triangular web, rubber pad and molded grip panels.
current=groups['Stock']
cyl('Receiver_extension',(-.248,0,.029),.016,.215,'steel','X',64,.0005)
for x in [-.147,-.151,-.156]:tube('Castle_nut_band',(x,0,.029),.021,.016,.004,'body',N=48)
stock=profile('Stock_polymer_shell',[(-.204,.057),(-.387,.057),(-.441,.081),(-.444,-.136),(-.416,-.146),(-.379,-.098),(-.282,-.053),(-.211,-.029)],.054,'poly',0)
cut(stock,[profile('cut_stock_web',[(-.289,-.026),(-.411,-.061),(-.411,-.100),(-.304,-.049)],.09,'dark',.006)])
bevel(stock,.003,5)
pad=profile('Buttpad_rubber',[(-.438,.079),(-.449,.077),(-.456,-.133),(-.444,-.146),(-.434,-.138)],.060,'rubber',.0025)
for z in [i*.009-.13 for i in range(23)]:box('Buttpad_traction',(-.454,0,z),(.003,.053,.003),'rubber',.0005)
for side in [-1,1]:
    profile('Cheek_weld', [(-.217,.052),(-.385,.053),(-.419,.034),(-.407,.008),(-.229,.009)],.010,'poly',.003,y=side*.028)
    line('Stock_mold_seam',[(-.215,side*.0285,-.02),(-.282,side*.0285,-.048),(-.375,side*.0285,-.092)],.0003,'steel')
    tube('Stock_sling_socket',(-.395,side*.030,-.029),.008,.005,.005,'steel',axis='Y',N=48)
    for x in [-.239,-.366]:screw('Stock_fastener',x,side*.035,.028,.003,side)
profile('Stock_adjustment_lever',[(-.235,-.032),(-.329,-.063),(-.347,-.054),(-.249,-.019)],.029,'poly',.002)
for i in range(7):box('Lever_grip_ridge',(-.266-i*.010,0,-.044-i*.003),(.003,.031,.0025),'rubber',.0005)
current=groups['Grip']
grip=profile('Pistol_grip_shell',[(-.091,-.037),(-.056,-.042),(-.073,-.094),(-.104,-.177),(-.154,-.166),(-.139,-.139),(-.115,-.064)],.036,'poly',.004)
for side in [-1,1]:
    panel=profile('Grip_overmold',[(-.103,-.073),(-.076,-.082),(-.112,-.165),(-.140,-.156)],.0022,'rubber',.0015,y=side*.018)
    # A fine diamond grip pattern, following the slanted panel rather than a flat block.
    for row in range(19):
        z=-.078-row*.0041;cx=-.094-row*.0017
        for col in range(7):
            x=cx+(col-3)*.0032
            o=box('Grip_stipple',(x,side*.0195,z),(.0013,.0006,.0013),'rubber',.00025)
            o.rotation_euler.y=math.pi/4
    screw('Grip_panel_pin',-.12,side*.020,-.151,.0022,side)
profile('Grip_base_plate',[(-.109,-.171),(-.153,-.163),(-.155,-.171),(-.108,-.181)],.036,'rubber',.001)

current=groups['Magazine']
magpoints=[(.035,-.059),(.118,-.054),(.118,-.103),(.132,-.156),(.164,-.215),(.093,-.244),(.061,-.185),(.043,-.124)]
mag=profile('Magazine_curved_shell',magpoints,.031,'steel',.0022)
for side in [-1,1]:
    for j in range(4):
        x=.050+j*.015
        line('Magazine_stamped_flute',[(x,side*.0165,-.083),(x+.001,side*.0165,-.121),(x+.014,side*.0165,-.171),(x+.042,side*.0165,-.215)],.0014,'body')
    for z in [-.105,-.129]:line('Magazine_spot_weld',[ (.042,side*.0168,z),(.114,side*.0168,z+.005)],.00035,'edge')
profile('Magazine_floorplate',[(.088,-.236),(.161,-.208),(.168,-.221),(.094,-.250)],.037,'body',.0013)
text('Magazine_lot','04 / 17',(.086,-.017,-.194),.0045)

# Window sight: open machined hood, glass, battery compartment, dial and clamp.
current=groups['Optic']
box('Optic_rail_mount',(-.040,0,.088),(.105,.040,.018),'body',.002)
for x in [-.07,-.014]:
    cyl('Optic_crossbolt',(x,0,.086),.0038,.051,'steel','Y',32,.0003)
    screw('Optic_clamp_screw',x,-.027,.086,.0045,-1)
outer=[(-.029,.104),(-.033,.148),(-.026,.170),(-.017,.180),(.017,.180),(.026,.170),(.033,.148),(.029,.104)]
inner=[(-.020,.110),(-.023,.147),(-.018,.164),(-.011,.171),(.011,.171),(.018,.164),(.023,.147),(.020,.110)]
v=[]
for xx,pts in [(-.085,outer),(-.045,outer),(-.085,inner),(-.045,inner)]:v.extend([(xx,y,z) for y,z in pts])
f=[]
for i in range(8):
 j=(i+1)%8;f.extend([(i,j,8+j,8+i),(16+i,24+i,24+j,16+j),(i,16+i,16+j,j),(8+i,8+j,24+j,24+i)])
mesh('Optic_protective_window_hood',v,f,'body',.0012)
box('Optical_window',(-.064,0,.138),(.002,.043,.055),'glass',.003)
box('Sight_electronics_base',(-.060,0,.110),(.065,.050,.027),'poly',.0025)
box('Battery_housing',(.003,0,.114),(.070,.042,.043),'guard',.004)
for side in [-1,1]:
    for x in [-.076,-.05]:screw('Hood_screw',x,side*.032,.123,.0028,side)
    cyl('Optic_adjustment_dial',(-.029,side*.026,.112),.010,.006,'steel','Y',48,.0007)
    for i in range(24):
        a=i*math.tau/24
        box('Dial_knurl',(-.029+.0095*math.cos(a),side*.027,.112+.0095*math.sin(a)),(.001,.006,.001),'body',.0002)
    text('Optic_adjustment_mark','UP',(-.040,side*.032,.128),.0035,side=side)
for z in [.099,.108,.117,.126]:box('Battery_end_grip',(.039,0,z),(.002,.033,.0015),'poly',.0003)
text('Optic_serial','H-01',(-.063,-.034,.158),.0045)

current=groups['Sights']
for x,front in [(-.126,False),(.424,True)]:
    box('Sight_base',(x,0,.083),(.019,.035,.011),'steel',.001)
    for side in [-1,1]:
        profile('Sight_protective_ear',[(x-.007,.086),(x-.005,.113),(x+.005,.119),(x+.008,.11),(x+.007,.086)],.003,'steel',.0006,y=side*.010)
    if front:cyl('Front_sight_post',(x,0,.103),.0015,.026,'steel','Z',20,.0002)
    else:tube('Rear_aperture',(x,0,.109),.0065,.003,.004,'steel','X',N=40,w=.0003)

current=groups['Markings']
# Tiny inspection marks and limited contact wear, not bright outline tracing.
for x,y,z in [(.145,-.036,.01),(-.072,-.031,.086)]:
    o=box('Inspection_witness',(x,y,z),(.005,.0002,.0012),'yellow',.00015);o.rotation_euler.y=.35
for i in range(15):
    x=random.uniform(-.12,.125)
    line('Receiver_contact_scuff',[(x,-.0269,.056),(x+random.uniform(.002,.007),-.0269,.0565)],.000055,'edge')

# Compact furniture proportions; transform the complete assemblies together.
from mathutils import Matrix
for group,anchor,factor in [('Stock',.029,.70),('Optic',.080,.76)]:
    transform=Matrix.Translation((0,0,anchor)) @ Matrix.Diagonal((1,1,factor,1)) @ Matrix.Translation((0,0,-anchor))
    for obj in groups[group].objects:obj.matrix_world=transform @ obj.matrix_world
# Hide reverse-side identifiers from being seen through the trigger opening.
bpy.data.objects['Receiver_identifier'].location.z=-.040

# Smart UVs are retained for further hand painting/baking; Blender shading uses
# packed procedural nodes. The study is intentionally not a retopologized LOD.
bpy.ops.object.select_all(action='DESELECT')
mesh_objects=[o for o in asset.all_objects if o.type=='MESH']
for o in mesh_objects:o.select_set(True)
bpy.context.view_layer.objects.active=mesh_objects[0]
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.025,scale_to_bounds=True)
bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT')
for o in asset.all_objects:
    o['asset_scope']='Standalone visual prototype'

# Studio presentation, separate from the export collection.
studio=bpy.data.collections.new('STUDIO_NOT_FOR_EXPORT');scene.collection.children.link(studio)
current=studio
floor=material('Studio_floor',(.043,.050,.058),.1,.5)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.271));ground=bpy.context.object
for c in list(ground.users_collection):c.objects.unlink(ground)
studio.objects.link(ground);ground.name='Studio_ground';ground.data.materials.append(floor)
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
def light(name,loc,power,size,color,shape='DISK',size_y=None):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape=shape;d.size=size;d.color=color
    if size_y is not None:d.size_y=size_y
    o=bpy.data.objects.new(name,d);studio.objects.link(o);o.location=loc;aim(o,(.08,0,-.025))
light('Large_softbox',(.05,-.8,1.3),95,1.1,(.89,.94,1),'RECTANGLE',.7)
light('Warm_edge_strip',(.30,.55,.68),130,.85,(1,.82,.63),'RECTANGLE',.17)
light('Receiver_fill',(-.6,-.4,.17),18,.65,(.68,.82,1))
light('Barrel_rim',(.9,.15,.3),55,.45,(.88,.94,1),'RECTANGLE',.12)
world=bpy.data.worlds.new('Studio_World');scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.14,.17,.21,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.22
def camera(name,loc,target,lens=58,ortho=None):
    d=bpy.data.cameras.new(name);o=bpy.data.objects.new(name,d);studio.objects.link(o);o.location=loc;aim(o,target)
    d.lens=lens;d.clip_start=.01;d.clip_end=1000
    if ortho:d.type='ORTHO';d.ortho_scale=ortho
    return o
camera('CAM_Hero',(.68,-1.95,.67),(.065,0,-.026),62)
camera('CAM_Profile',(.08,-2,-.035),(.08,0,-.035),ortho=1.22)
camera('CAM_Receiver',(-.20,-.62,.24),(-.006,0,.002),70)
camera('CAM_FirstPerson',(-.54,-.145,.238),(.30,.19,.085),35)
scene.camera=bpy.data.objects['CAM_Hero']
scene.render.engine='CYCLES';scene.cycles.samples=96;scene.cycles.use_denoising=True
scene.render.resolution_x=2400;scene.render.resolution_y=1350;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.view_settings.exposure=-.65
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
scene.render.film_transparent=False
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA'
ROOT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'M4_Precision_Study.blend'))
print('M4_SAMPLE_BUILD_PASS',len(mesh_objects),'mesh parts',flush=True)
