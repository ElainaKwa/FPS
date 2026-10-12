"""Second exterior-art pass. Executed by build_sample.py before UV generation.
Uses its scene helpers; all dimensions are visual proportions, not engineering data.
"""
def remove_prefixes(collection,prefixes):
    for obj in list(collection.objects):
        if any(obj.name.startswith(prefix) for prefix in prefixes):bpy.data.objects.remove(obj,do_unlink=True)

def smooth_sides(obj):
    for polygon in obj.data.polygons:polygon.use_smooth=len(polygon.vertices)==4
    return obj

def loft_x(name,sections,outline,mat,w=.001):
    # Sections: x, width multiplier, height multiplier, height offset.
    n=len(outline);verts=[(x,y*sy,z*sz+dz) for x,sy,sz,dz in sections for y,z in outline]
    faces=[tuple(reversed(range(n))),tuple(range((len(sections)-1)*n,len(sections)*n))]
    faces += [(r*n+i,r*n+(i+1)%n,(r+1)*n+(i+1)%n,(r+1)*n+i) for r in range(len(sections)-1) for i in range(n)]
    return mesh(name,verts,faces,mat,w)

def rounded_rect(width,height,r,cy=0,cz=0,steps=8):
    points=[]
    for y,z,a in [(width/2-r,height/2-r,0),(-width/2+r,height/2-r,90),(-width/2+r,-height/2+r,180),(width/2-r,-height/2+r,270)]:
        for i in range(steps+1):
            angle=math.radians(a+90*i/steps)
            points.append((cy+y+r*math.cos(angle),cz+z+r*math.sin(angle)))
    return points

def ring_x(name,x0,x1,outer,inner,mat,w=.0005):
    n=len(outer);verts=[(x,y,z) for x,outline in [(x0,outer),(x1,outer),(x0,inner),(x1,inner)] for y,z in outline]
    faces=[]
    for i in range(n):
        j=(i+1)%n
        faces += [(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),(i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)]
    return mesh(name,verts,faces,mat,w)

def loft_z(name,rings,mat,power=3,n=48,w=.001):
    verts=[]
    for z,cx,rx,ry in rings:
        for i in range(n):
            a=math.tau*i/n;c=math.cos(a);s=math.sin(a)
            verts.append((cx+rx*math.copysign(abs(c)**(2/power),c),ry*math.copysign(abs(s)**(2/power),s),z))
    faces=[tuple(reversed(range(n))),tuple(range((len(rings)-1)*n,len(rings)*n))]
    faces += [(r*n+i,r*n+(i+1)%n,(r+1)*n+(i+1)%n,(r+1)*n+i) for r in range(len(rings)-1) for i in range(n)]
    return smooth_sides(mesh(name,verts,faces,mat,w))

# Make coatings visibly different without turning the gun into polished silver.
for key,color,metal,rough in [
    ('body',(.014,.018,.021),.55,.43),('guard',(.010,.013,.015),.38,.49),
    ('steel',(.020,.024,.027),.72,.39),('poly',(.010,.012,.013),0,.60),
    ('rubber',(.006,.007,.008),0,.76),('edge',(.07,.075,.08),.85,.39)]:
    surface_material=mats[key];surface_material.diffuse_color=(*color,1)
    nodes=surface_material.node_tree.nodes;links=surface_material.node_tree.links;bs=nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Metallic'].default_value=metal
    bs.inputs['Specular IOR Level'].default_value=.22 if key in ['poly','rubber'] else .36
    bs.inputs['Roughness'].default_value=rough
    for node in nodes:
        if node.type=='MAP_RANGE':node.inputs['To Min'].default_value=rough-.045;node.inputs['To Max'].default_value=rough+.055
        if node.type=='BUMP':node.inputs['Strength'].default_value=.30
    # Subtle unevenness in the finish remains visible in a close-up.
    coord=nodes.new('ShaderNodeTexCoord');noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=72;noise.inputs['Detail'].default_value=3
    links.new(coord.outputs['Object'],noise.inputs['Vector'])
    variation=nodes.new('ShaderNodeValToRGB');variation.color_ramp.elements[0].position=.16;variation.color_ramp.elements[1].position=.84
    variation.color_ramp.elements[0].color=tuple(v*.84 for v in color)+(1,)
    variation.color_ramp.elements[1].color=tuple(v*1.12 for v in color)+(1,)
    links.new(noise.outputs['Fac'],variation.inputs[0]);links.new(variation.outputs['Color'],bs.inputs['Base Color'])
glass.inputs['Base Color'].default_value=(.82,.94,.90,1)
glass.inputs['Roughness'].default_value=.025

# The receiver has a rounded crown, flat working faces, and tapered end shoulders.
current=groups['Receiver']
remove_prefixes(current,['Upper_receiver_forging','Receiver_forged_shoulder','Magwell_side_panel','Magazine_well_flare','Forward_assist','Assist_cap_rib','Lower_forging_edge'])
outline=[(-.021,-.007),(-.026,.001)]+[(-.026*math.cos(math.pi*i/32),.032+.033*math.sin(math.pi*i/32)) for i in range(33)]+[(.026,.001),(.021,-.007)]
upper=loft_x('Upper_receiver_sculpted',[(-.144,.82,.88,.003),(-.131,1,1,0),(.118,1,1,0),(.141,.88,.91,.002)],outline,'body',0)
cut(upper,[box('cut_port_v2',(.054,-.028,.029),(.078,.026,.025),'dark',.005)])
bevel(upper,.0013,4)
for poly in upper.data.polygons:
    # Keep the broad side panels flat; smooth only the crown strips.
    poly.use_smooth=poly.center.z>.043
for side in [-1,1]:
    profile('Receiver_lower_rim',[(-.128,-.007),(.120,-.007),(.132,-.003),(.129,-.010),(-.127,-.012)],.003,'body',.0008,y=side*.023)
    profile('Magazine_well_beveled_cheek',[(.025,-.035),(.135,-.034),(.142,-.043),(.138,-.078),(.034,-.089),(.022,-.079)],.004,'body',.0014,y=side*.025)
    # Shallow machining recess with softened corners.
    profile('Magazine_well_inset',[(.039,-.045),(.126,-.043),(.130,-.049),(.125,-.072),(.042,-.082),(.035,-.074)],.0007,'guard',.0003,y=side*.0273)
profile('Magwell_bottom_lip',[(.028,-.079),(.138,-.068),(.143,-.079),(.032,-.092)],.057,'body',.0015)
# Exterior assist boss with a natural diagonal silhouette.
start=Vector((-.051,-.023,.032));end=Vector((-.077,-.046,.032));axis=(end-start).normalized()
boss=cyl('Angled_assist_boss',(start+end)/2,.0088,(end-start).length,'body','Z',64,.001)
boss.rotation_euler=axis.to_track_quat('Z','Y').to_euler()
cap=cyl('Assist_button_cap',end+axis*.003,.0081,.007,'poly','Z',64,.001)
cap.rotation_euler=boss.rotation_euler
for i in range(4):
    rib=box('Assist_button_grip',end+axis*.0065+Vector((0,0,(i-1.5)*.003)),(.010,.001,.0007),'rubber',.00025)
    rib.rotation_euler.z=math.atan2(axis.y,axis.x)+math.pi/2
bpy.data.objects['Receiver_serial'].location=(.073,-.0278,-.068)
bpy.data.objects['Receiver_serial'].data.size=.0034
remove_prefixes(groups['Markings'],['Receiver_contact_scuff'])

# Stock: a rounded sleeve, a thin rear frame and an open support strut.
current=groups['Stock'];remove_prefixes(current,[''])
cyl('Receiver_extension',(-.247,0,.029),.015,.211,'steel','X',64,.0005)
nut=tube('Castle_nut',(-.153,0,.029),.020,.015,.011,'steel',N=64,w=0)
cut(nut,[box('cut_nut_notch',(-.153,sy*.019,.029),(.006,.005,.007),'dark',.0005) for sy in [-1,1]])
bevel(nut,.0006)
sleeve_outline=rounded_rect(.050,.047,.014,cz=.027,steps=6)
stock=loft_x('Stock_cheek_sleeve',[(-.421,.9,1,0),(-.404,1.13,1.07,0),(-.28,1.08,1.0,0),(-.235,.85,.91,.003)],sleeve_outline,'poly',.002)
for p in stock.data.polygons:p.use_smooth=len(p.vertices)==4
profile('Stock_rear_frame',[(-.424,.059),(-.408,.052),(-.399,-.070),(-.416,-.094),(-.431,-.088),(-.438,.037)],.046,'poly',.003)
profile('Stock_diagonal_strut',[(-.417,-.085),(-.404,-.060),(-.278,-.011),(-.254,-.006),(-.267,-.024),(-.401,-.087)],.025,'poly',.0025)
pad=profile('Buttpad_rubber',[(-.429,.061),(-.439,.059),(-.447,.041),(-.440,-.076),(-.429,-.100),(-.416,-.098),(-.413,-.082),(-.423,.041)],.053,'rubber',.0035)
for i in range(15):
    z=-.083+i*.009
    x=-.440-(z+.083)*.042
    box('Buttpad_traction',(x,0,z),(.003,.047,.0028),'rubber',.0009)
for side in [-1,1]:
    line('Stock_parting_seam',[(-.416,side*.026,.007),(-.380,side*.028,.003),(-.278,side*.027,.006),(-.242,side*.022,.011)],.0003,'rubber')
    for i in range(5):
        line('Stock_cheek_texture',[(-.388+i*.013,side*.0278,.025),(-.38+i*.013,side*.0283,.035)],.00032,'rubber')
    tube('Stock_sling_socket',(-.408,side*.025,-.031),.0073,.0048,.004,'steel',axis='Y',N=64,w=.0004)
    screw('Stock_frame_pin',-.416,side*.025,.005,.0026,side)
profile('Stock_adjustment_paddle',[(-.278,-.008),(-.367,-.008),(-.374,-.022),(-.341,-.032),(-.273,-.024)],.025,'poly',.003)
for i in range(6):box('Stock_paddle_rib',(-.29-i*.011,0,-.025),(.003,.027,.002),'rubber',.00065)

# Rounded ergonomic grip, with continuous checkered bands following its contour.
current=groups['Grip'];remove_prefixes(current,[''])
# Broader, shorter proportions guided by the user's Delta Force M4A1 reference.
# These are artistic mesh proportions, not a physical component specification.
grip_rings=[(-.038,-.087,.020,.018),(-.048,-.087,.023,.021),(-.068,-.092,.026,.023),(-.090,-.100,.030,.024),(-.118,-.112,.033,.024),(-.141,-.121,.034,.023),(-.150,-.122,.032,.022)]
grip=loft_z('Ergonomic_grip_shell',grip_rings,'poly',power=3.0,w=.0015)
profile('Grip_upper_saddle',[(-.128,-.030),(-.111,-.032),(-.068,-.044),(-.070,-.060),(-.110,-.065),(-.124,-.052)],.038,'poly',.004)
def grip_surface(z,angle,offset=0):
    for upper,lower in zip(grip_rings,grip_rings[1:]):
        if lower[0]<=z<=upper[0]:
            t=(upper[0]-z)/(upper[0]-lower[0]);cx,rx,ry=[upper[i]*(1-t)+lower[i]*t for i in range(1,4)];break
    else:raise ValueError('Grip surface height outside rings')
    c=math.cos(angle);s=math.sin(angle)
    return (cx+(rx+offset)*math.copysign(abs(c)**(2/3),c),(ry+offset)*math.copysign(abs(s)**(2/3),s),z)
for side in [-1,1]:
    # Conformal inlay follows both the side thickness and the palm swell.
    verts=[];rows=24;cols=18
    for row in range(rows+1):
        z=-.066-row*.078/rows
        half_angle=math.radians(38)*(0.82+0.18*math.sin(math.pi*row/rows))
        for col in range(cols+1):verts.append(grip_surface(z,side*math.pi/2+(col/cols*2-1)*half_angle,.00020))
    faces=[(r*(cols+1)+c,r*(cols+1)+c+1,(r+1)*(cols+1)+c+1,(r+1)*(cols+1)+c) for r in range(rows) for c in range(cols)]
    panel=mesh('Grip_conformal_rubber',verts,faces,'rubber',0)
    if sum(p.normal.y for p in panel.data.polygons)*side<0:
        bm=bmesh.new();bm.from_mesh(panel.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(panel.data);bm.free()
    for polygon in panel.data.polygons:polygon.use_smooth=True
    for row in range(22):
        z=-.072-row*.0032
        for col in range(7):
            angle=side*math.pi/2+(col-3)*.11
            point=grip_surface(z,angle,.00045)
            o=box('Grip_fine_stipple',point,(.0010,.0006,.0010),'rubber',.00024)
            o.rotation_euler.z=angle-side*math.pi/2
# A low finger ledge and narrow front ribs reinforce the visual scale.
profile('Grip_front_finger_ledge',[(-.072,-.087),(-.066,-.089),(-.064,-.094),(-.073,-.098),(-.079,-.094)],.030,'poly',.0017)
for z in [-.103,-.110,-.117,-.124,-.131]:
    points=[grip_surface(z,math.radians(a),.0002) for a in range(-43,44,5)]
    line('Grip_front_traction',points,.00045,'rubber')
loft_z('Grip_bottom_plug',[(-.146,-.122,.033,.023),(-.153,-.122,.034,.023)],'rubber',power=3,w=.0015)

# Curved magazine with actual shallow pressed channels instead of thin added rods.
current=groups['Magazine'];remove_prefixes(current,[''])
mag_rings=[(-.061,.077,.041,.015),(-.09,.078,.041,.015),(-.118,.081,.041,.015),(-.15,.088,.041,.015),(-.185,.101,.041,.015),(-.219,.119,.041,.015),(-.231,.127,.041,.015)]
mag=loft_z('Magazine_pressed_shell',mag_rings,'steel',power=5,n=48,w=0)
channels=[]
for side in [-1,1]:
    for j in range(4):
        x=.052+j*.016
        path=[(x,-.091),(x+.002,-.119),(x+.009,-.15),(x+.022,-.185),(x+.039,-.214)]
        shape=[(xx-.0022,z) for xx,z in path]+[(xx+.0022,z) for xx,z in reversed(path)]
        channels.append(profile('cut_pressed_channel',shape,.0026,'dark',.0012,y=side*.0154))
cut(mag,channels);bevel(mag,.0008,3)
loft_z('Magazine_floorplate',[(-.224,.125,.044,.018),(-.234,.130,.044,.018)],'body',power=5,w=.001)
for side in [-1,1]:
    line('Magazine_rear_fold',[(.036,side*.011,-.091),(.041,side*.011,-.118),(.048,side*.011,-.15),(.061,side*.011,-.185),(.079,side*.011,-.216)],.0006,'body')
text('Magazine_lot','04 / 17',(.105,-.0153,-.204),.0032)

# Holographic-style exterior: a rounded hood and a matching inset glass outline.
current=groups['Optic'];remove_prefixes(current,[''])
box('Optic_mount_foot',(-.035,0,.086),(.105,.035,.014),'body',.002)
for side in [-1,1]:
    profile('Optic_clamp_jaw',[(-.087,.083),(.02,.083),(.018,.093),(-.08,.096)],.007,'body',.001,y=side*.018)
for x in [-.07,.003]:screw('Optic_crossbolt_head',x,-.024,.087,.0038,-1)
profile('Optic_lower_housing',[(-.091,.095),(-.080,.107),(-.038,.111),(.029,.105),(.041,.098),(.039,.089),(-.083,.088)],.047,'poly',.0025)
outer=rounded_rect(.065,.061,.012,cz=.131,steps=10)
inner=rounded_rect(.053,.050,.009,cz=.132,steps=10)
hood=ring_x('Optic_rounded_hood',-.082,-.047,outer,inner,'guard',.0009)
for i,p in enumerate(hood.data.polygons):p.use_smooth=i%4<2
gasket_outer=rounded_rect(.054,.051,.0095,cz=.132,steps=10)
gasket_inner=rounded_rect(.0515,.0485,.0085,cz=.132,steps=10)
ring_x('Optic_lens_gasket',-.063,-.060,gasket_outer,gasket_inner,'rubber',.00025)
loft_x('Optic_contoured_glass',[(-.0615,1,1,0),(-.0605,1,1,0)],gasket_inner,'glass',.0001)
profile('Optic_battery_pack',[(-.044,.107),(-.034,.115),(.027,.115),(.038,.109),(.038,.098),(-.042,.098)],.041,'guard',.002)
box('Battery_end_cap',(.039,0,.106),(.004,.042,.017),'poly',.0015)
for z in [.102,.107,.112]:box('Battery_cap_rib',(.0415,0,z),(.001,.027,.001),'rubber',.0003)
for side in [-1,1]:
    screw('Optic_hood_fastener',-.069,side*.0325,.117,.0026,side)
    cyl('Optic_recessed_dial',(-.028,side*.024,.101),.006,.003,'steel','Y',64,.0005)
    box('Optic_dial_slot',(-.028,side*.0257,.101),(.006,.0003,.0008),'dark',.0002)
    text('Optic_laser_index','H-01',(-.073,side*.0327,.126),.0023,side=side)
for y in [-.011,.011]:
    button=cyl('Optic_rear_button',(-.092,y,.100),.0042,.002,'rubber','X',48,.0006)
    line('Button_symbol',[(-.0932,y-.0015,.100),(-.0932,y+.0015,.100)],.00023,'marks')

# Compact sight silhouettes. The rear unit is folded below the optic window.
current=groups['Sights'];remove_prefixes(current,[''])
box('Rear_folded_sight_base',(-.123,0,.083),(.029,.027,.012),'body',.002)
cyl('Rear_folded_sight_axle',(-.126,0,.091),.0045,.031,'steel','Y',48,.0005)
box('Rear_folded_sight_leaf',(-.110,0,.094),(.024,.017,.006),'steel',.0015)
box('Front_sight_base',(.424,0,.082),(.024,.030,.012),'body',.0018)
for side in [-1,1]:
    profile('Front_sight_hood',[(.412,.086),(.415,.106),(.419,.116),(.427,.116),(.432,.106),(.435,.086)],.0037,'steel',.0015,y=side*.010)
    screw('Front_sight_pivot',.423,side*.017,.085,.003,side)
cyl('Front_sight_post',(.423,0,.101),.0013,.020,'steel','Z',32,.0003)

# Witness marks sit on the corresponding fasteners, not in empty space.
current=groups['Markings'];remove_prefixes(current,['Inspection_witness'])
box('Inspection_witness',(.151,-.036,.009),(.003,.00025,.0007),'yellow',.0001)

# Follow-up: compact receiver proportions and pressed-metal magazine character,
# observed in the user-requested Delta Force M4A1 screenshot.
current=groups['Receiver']
remove_prefixes(current,['Lower_receiver_forging','Magazine_well_beveled_cheek','Magazine_well_inset','Magwell_bottom_lip'])
lower=profile('Lower_receiver_contoured',[(-.138,-.009),(.134,-.009),(.141,-.025),(.137,-.081),(.033,-.091),(.019,-.050),(-.076,-.050),(-.091,-.033),(-.135,-.030)],.047,'body',0)
pockets=[]
for side in [-1,1]:
    pocket=profile('cut_lower_recess',[(.044,-.043),(.124,-.039),(.126,-.071),(.043,-.079),(.033,-.070)],.0024,'dark',.0012,y=side*.0240)
    pockets.append(pocket)
cut(lower,pockets);bevel(lower,.0020,4)
profile('Magwell_flared_rim',[(.031,-.081),(.136,-.070),(.141,-.081),(.033,-.093)],.055,'body',.0015)
# Raised control fence follows the receiver casting, with small stamped identifiers.
line('Magazine_button_fence',[(.010,-.025,-.012),(.044,-.025,-.012),(.048,-.025,-.036),(.013,-.025,-.038)],.0014,'body')
text('Receiver_model_mark','M4  /  FIELD',(.053,-.0235,-.051),.0042)
text('Receiver_batch_mark','SERIES 04',(.053,-.0235,-.060),.0030)
bpy.data.objects['Receiver_serial'].location=(.053,-.0235,-.070)
bpy.data.objects['Receiver_serial'].data.size=.0028
for i in range(13):
    box('Receiver_microcode',(.108+i*.0009,-.0236,-.059),(.0004,.0001,.007 if i%3 else .006),'marks',0)
for obj in current.objects:
    if obj.name.startswith(('Selector_safe','Selector_fire')):obj.location.z=-.014;obj.data.size=.0032

current=groups['Magazine']
mag_finish=material('11_Magazine_Satin',(.040,.047,.049),.65,.51,.000045)
bpy.data.objects['Magazine_pressed_shell'].data.materials.clear()
bpy.data.objects['Magazine_pressed_shell'].data.materials.append(mag_finish)
for side in [-1,1]:
    for z in [-.095,-.112]:
        # A narrow seam across the upper body, kept below the receiver insertion.
        line('Magazine_transverse_press',[(.044,side*.0147,z),(.051,side*.0153,z),(.107,side*.0153,z),(.114,side*.0147,z)],.00038,'steel')

# Align the surrounding assemblies after reducing the receiver's slab-like bulk.
# This changes only the art study; no runtime sockets or gameplay values exist here.
from mathutils import Matrix
receiver_fit=Matrix.Translation((.143,0,.073)) @ Matrix.Diagonal((.84,1,.82,1)) @ Matrix.Translation((-.143,0,-.073))
for obj in groups['Receiver'].objects:obj.matrix_world=receiver_fit @ obj.matrix_world
for group,offset in [('Stock',(.046,0,0)),('Grip',(.040,0,.024)),('Magazine',(.018,0,.029)),('Optic',(.029,0,0))]:
    move=Matrix.Translation(offset)
    for obj in groups[group].objects:obj.matrix_world=move @ obj.matrix_world
for obj in groups['Sights'].objects:
    if obj.name.startswith('Rear_'):obj.matrix_world=Matrix.Translation((.043,0,0)) @ obj.matrix_world
