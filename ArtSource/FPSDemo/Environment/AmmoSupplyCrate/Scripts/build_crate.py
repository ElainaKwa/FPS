"""Build an original placeable industrial supply-case exterior and FBX.
Run in an isolated Blender process. Only owns this asset's source/output files.
"""
import bpy, bmesh, math, json, hashlib
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT.parents[3]
NAME='SM_AmmoSupplyCrate'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1.0
art=bpy.data.collections.new('AmmoSupplyCrate');scene.collection.children.link(art)
art.asset_mark();art.asset_data.description='Industrial ammo supply case; bottom-center pivot; closed exterior'
current=art
palette={
    'Graphite':dict(color=[.038,.051,.057],metallic=.48,roughness=.46),
    'Panel':dict(color=[.075,.095,.100],metallic=.30,roughness=.54),
    'Metal':dict(color=[.24,.27,.28],metallic=.85,roughness=.36),
    'Rubber':dict(color=[.013,.018,.021],metallic=0,roughness=.77),
    'Yellow':dict(color=[.95,.64,.035],metallic=.20,roughness=.43),
    'Markings':dict(color=[.79,.84,.81],metallic=.0,roughness=.55),
}
mats={}
for key,p in palette.items():
    m=bpy.data.materials.new('MI_AMC_'+key);m.diffuse_color=(*p['color'],1);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF')
    for name,val in [('Base Color',(*p['color'],1)),('Metallic',p['metallic']),('Roughness',p['roughness'])]:bs.inputs[name].default_value=val
    mats[key]=m

def finish(o,name,mat,w=0):
    o.name=name
    for col in list(o.users_collection):col.objects.unlink(o)
    current.objects.link(o);o.data.materials.append(mats[mat])
    if w:
        b=o.modifiers.new('Edge bevel','BEVEL');b.width=w;b.segments=3
        n=o.modifiers.new('Face normals','WEIGHTED_NORMAL');n.keep_sharp=True;n.weight=50
    return o

def box(name,loc,size,mat='Graphite',w=.002):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.dimensions=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return finish(o,name,mat,min(w,min(size)*.3))

def poly(name,pts,depth,mat='Graphite',w=.001,y=0):
    n=len(pts);v=[(x,yy,z) for yy in [y-depth/2,y+depth/2] for x,z in pts]
    f=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    data=bpy.data.meshes.new(name);data.from_pydata(v,[],f);data.update()
    bm=bmesh.new();bm.from_mesh(data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(data);bm.free()
    o=bpy.data.objects.new(name,data);current.objects.link(o)
    return finish(o,name,mat,w)

def shell(name,w,d,z0,z1,corner,mat,bevel=.003):
    xy=[(-w/2+corner,-d/2),(w/2-corner,-d/2),(w/2,-d/2+corner),(w/2,d/2-corner),(w/2-corner,d/2),(-w/2+corner,d/2),(-w/2,d/2-corner),(-w/2,-d/2+corner)]
    verts=[(x,y,z) for z in [z0,z1] for x,y in xy]
    faces=[tuple(reversed(range(8))),tuple(range(8,16))]+[(i,(i+1)%8,(i+1)%8+8,i+8) for i in range(8)]
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new(name,mesh);current.objects.link(o)
    return finish(o,name,mat,bevel)

def cyl(name,loc,r,depth,mat='Metal',axis='Y',verts=20):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=loc)
    o=bpy.context.object;o.rotation_euler=(math.pi/2,0,0) if axis=='Y' else ((0,math.pi/2,0) if axis=='X' else (0,0,0))
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    return finish(o,name,mat,min(.0008,r*.15))

def bolt(x,y,z,axis='Y'):
    cyl('Recessed_fastener',(x,y,z),.0045,.0024,'Metal',axis,6)
    if axis=='Y':box('Fastener_slot',(x,y+(-.0014 if y<0 else .0014),z),(.004,.0003,.0008),'Rubber',0)

font_dir=PROJECT/'ArtSource/FPSDemo/Industrial/UI/Fonts'
english=bpy.data.fonts.load(str(font_dir/'Rajdhani-Bold.ttf'))
chinese=bpy.data.fonts.load(str(font_dir/'MiSans-Bold.ttf'))
def label(name,body,loc,size,mat='Markings',font=None,rotation=(math.pi/2,0,0),align='LEFT'):
    c=bpy.data.curves.new(name,'FONT');c.body=body;c.size=size;c.resolution_u=2;c.extrude=0;c.align_x=align
    c.font=font or english
    o=bpy.data.objects.new(name,c);current.objects.link(o);o.location=loc;o.rotation_euler=rotation
    return finish(o,name,mat)

# Closed molded housing with continuous gasket and layered lid.
shell('Lower_case',.720,.460,.045,.337,.045,'Graphite',.004)
shell('Bottom_armored_rim',.726,.466,.037,.067,.044,'Rubber',.004)
shell('Lid_gasket',.722,.462,.335,.345,.044,'Rubber',.0015)
shell('Lid_shell',.728,.468,.345,.405,.043,'Graphite',.004)
shell('Lid_top_inset',.628,.375,.403,.413,.030,'Panel',.002)
for x in [-.277,.277]:
    box('Floor_skid',(x,0,.018),(.092,.398,.036),'Rubber',.006)
    for y in [-.174,.174]:box('Skid_end_cap',(x,y,.040),(.096,.059,.047),'Graphite',.004)
    box('Stacking_rail',(x,0,.414),(.046,.383,.024),'Rubber',.004)
    for y in [-.167,.167]:box('Rail_protector',(x,y,.425),(.046,.048,.009),'Graphite',.002)

# Upright corner pads and molded side seams.
for x in [-.326,.326]:
    for y in [-.204,.204]:
        box('Corner_upright',(x,y,.191),(.060,.051,.260),'Rubber',.007)
        box('Corner_upper_cap',(x,y,.363),(.064,.056,.086),'Rubber',.007)
        for z in [.079,.306]:bolt(x,y+(-.027 if y<0 else .027),z)
for y in [-.234,.234]:
    # Lower front panel leaves a continuous protective frame around the face.
    panel=[(-.282,.080),(.282,.080),(.298,.097),(.298,.280),(.280,.296),(-.280,.296),(-.298,.280),(-.298,.097)]
    poly('Inset_face_panel',panel,.006,'Panel',.002,y=y)
    for x in [-.265,.265]:
        box('Face_reinforcement',(x,y+(-.005 if y<0 else .005),.178),(.016,.008,.161),'Graphite',.002)
    for x in [-.241,.241]:
        for z in [.092,.281]:bolt(x,y+(-.004 if y<0 else .004),z)

# Two contrasting lid latches. Exterior shapes only, no animated mechanism.
for x in [-.228,.228]:
    box('Latch_recess',(x,-.243,.330),(.061,.017,.110),'Rubber',.004)
    box('Latch_bridge',(x,-.254,.346),(.035,.013,.075),'Metal',.002)
    poly('Yellow_latch_cover',[(x-.024,.283),(x+.024,.283),(x+.027,.293),(x+.024,.340),(x-.024,.340),(x-.027,.293)],.015,'Yellow',.002,y=-.265)
    cyl('Latch_hinge',(x,-.269,.336),.0045,.054,'Metal','X')
    box('Latch_grip',(x,-.275,.300),(.030,.003,.006),'Graphite',.001)
    bolt(x,-.274,.321)
    box('Lid_identity_tab',(x,-.237,.385),(.058,.005,.012),'Yellow',.001)

# Rear hinge barrels and a narrow maintenance panel.
for x in [-.227,.227]:
    for dx in [-.025,.0,.025]:cyl('Lid_rear_hinge',(x+dx,.241,.344),.008,.021,'Metal','X')
    box('Hinge_strap',(x,.239,.333),(.081,.008,.064),'Graphite',.002)
    for dx in [-.026,.026]:bolt(x+dx,.245,.310)
label('Back_identifier','LOGISTICS  /  FIELD EQUIPMENT',(.195,.239,.205),.017,rotation=(math.pi/2,0,math.pi))
label('Back_smallprint','RETURN CASE AFTER USE',(.140,.239,.176),.012,rotation=(math.pi/2,0,math.pi))

# Side recessed handle wells and folded U handles.
for side in [-1,1]:
    x=side*.364
    box('Handle_well',(x,0,.245),(.008,.270,.105),'Rubber',.004)
    box('Handle_well_inset',(x+side*.004,0,.243),(.003,.227,.069),'Graphite',.002)
    for y in [-.106,.106]:
        cyl('Handle_pivot',(x+side*.010,y,.264),.011,.017,'Metal','X')
        box('Handle_arm',(x+side*.015,y,.238),(.013,.014,.052),'Metal',.003)
    box('Carry_handle',(x+side*.016,0,.216),(.025,.224,.023),'Rubber',.005)
    for y in [-.073,-.049,-.025,0,.025,.049,.073]:
        box('Handle_grip_ridge',(x+side*.029,y,.216),(.002,.002,.017),'Graphite',.0005)
    box('Side_identification_stripe',(side*.363,0,.118),(.004,.296,.022),'Yellow',.001)
    for y in [-.129,.129]:bolt(side*.369,y,.145,axis='X')

# Large readable stencil and an abstract ammunition pictogram.
box('Front_label_field',(-.024,-.240,.193),(.353,.002,.135),'Graphite',.001)
label('Main_stencil','AMMO',(-.125,-.2415,.200),.064)
label('Chinese_supply','弹药补给',(-.120,-.2416,.157),.025,font=chinese)
box('Identity_rule',(-.029,-.242,.146),(.194,.0005,.002),'Yellow',0)
for i in range(3):
    x=.126+i*.022
    poly('Ammo_pictogram',[(x-.006,.164),(x+.006,.164),(x+.006,.211),(x+.004,.220),(x,.229),(x-.004,.220),(x-.006,.211)],.0005,'Yellow',0,y=-.2415)
    box('Pictogram_base',(x,-.2418,.166),(.014,.0006,.003),'Yellow',0)
label('Front_subtitle','FIELD SUPPLY  /  04',(-.124,-.2415,.116),.012)
label('Front_batch','CASE  017',(-.238,-.242,.109),.009)
# Flat top label and minimal transport markings.
label('Lid_stencil','SUPPLY  /  04',(0,-.095,.414),.036,rotation=(0,0,0),align='CENTER')
label('Lid_smallprint','AMMUNITION',(0,-.128,.414),.013,rotation=(0,0,0),align='CENTER')
for x in [-.190,.190]:
    box('Lid_yellow_locator',(x,0,.414),(.012,.247,.0012),'Yellow',.0002)
for i,width in enumerate([.002,.001,.003,.001,.0015,.0025,.001,.003,.002,.001]):
    box('Tracking_barcode',(-.079+i*.006,-.2417,.260),(width,.0006,.017),'Markings',0)
label('Tracking_number','04-017',(.010,-.2418,.253),.012)

# Preserve editable authoring meshes, with a first UV set for every mesh.
bpy.ops.object.select_all(action='DESELECT')
meshes=[o for o in art.objects if o.type=='MESH']
for o in meshes:o.select_set(True)
bpy.context.view_layer.objects.active=meshes[0]
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.1519,island_margin=.02)
bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT')

# Evaluate and join a single game mesh, preserving six material slots.
deps=bpy.context.evaluated_depsgraph_get();exports=[]
for src in list(art.objects):
    data=bpy.data.meshes.new_from_object(src.evaluated_get(deps),preserve_all_data_layers=True,depsgraph=deps)
    if not data.uv_layers:data.uv_layers.new(name='UVMap')
    obj=bpy.data.objects.new('ExportPart',data);scene.collection.objects.link(obj);obj.matrix_world=src.matrix_world
    obj.select_set(True);exports.append(obj)
bpy.context.view_layer.objects.active=exports[0];bpy.ops.object.join();joined=bpy.context.object;joined.name=NAME
scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
# Pack a separate whole-mesh lightmap UV, including converted lettering.
joined.data.uv_layers.new(name='LightmapUV');joined.data.uv_layers.active_index=1
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.1519,island_margin=.008,scale_to_bounds=True)
bpy.ops.object.mode_set(mode='OBJECT');joined.data.uv_layers.active_index=0
joined.data.calc_loop_triangles()
points=[joined.matrix_world@v.co for v in joined.data.vertices]
bounds=[[min(v[i] for v in points) for i in range(3)],[max(v[i] for v in points) for i in range(3)]]
triangles=len(joined.data.loop_triangles)
assert triangles<55000,triangles
assert abs(bounds[0][2])<.00001,bounds
path=ROOT/'Exports'/(NAME+'.fbx')
bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},axis_forward='X',axis_up='Z',use_space_transform=False,add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE')
bpy.data.objects.remove(joined,do_unlink=True)

# Separate studio. Render-only contents never enter the FBX.
studio=bpy.data.collections.new('STUDIO_NOT_FOR_EXPORT');scene.collection.children.link(studio);current=studio
box('Studio_floor',(0,0,-.025),(200,200,.035),'Panel',0)
def aim(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
for name,pos,power,size,col in [('Key',(.2,-1.4,2),160,1.7,(.9,.95,1)),('Fill',(-1.2,-.4,1),80,1.5,(.72,.84,1)),('Rim',(.7,1.1,1.7),220,1.1,(1,.86,.65))]:
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=col
    o=bpy.data.objects.new(name,d);studio.objects.link(o);o.location=pos;aim(o,(0,0,.2))
world=bpy.data.worlds.new('Studio');world.use_nodes=True;scene.world=world
world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.16,.20,1);world.node_tree.nodes['Background'].inputs[1].default_value=.25
for name,loc,target,lens in [('Hero',(1.18,-1.55,1.02),(0,0,.21),58),('Front',(0,-1.55,.42),(0,0,.21),62),('Rear',(-1.10,1.50,.91),(0,0,.21),58)]:
    d=bpy.data.cameras.new('CAM_'+name);o=bpy.data.objects.new('CAM_'+name,d);studio.objects.link(o);o.location=loc;aim(o,target);d.lens=lens;d.clip_start=.01
scene.camera=bpy.data.objects['CAM_Hero'];scene.render.engine='CYCLES';scene.cycles.samples=64;scene.cycles.use_denoising=True
scene.render.resolution_x=1920;scene.render.resolution_y=1440;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
scene.view_settings.view_transform='AgX';scene.view_settings.exposure=-.5
scene.render.filepath=str(ROOT/'Previews/Hero.png')
scene['asset_scope']='Placeable closed exterior; no resupply gameplay';scene['source_authority']='Scripts/build_crate.py'
bpy.ops.file.pack_all()
blend=ROOT/'Source/AmmoSupplyCrate.blend';bpy.ops.wm.save_as_mainfile(filepath=str(blend))
manifest=dict(version='AmmoSupplyCrate-20261012-v1',generated_by='Scripts/build_crate.py',blender=bpy.app.version_string,
    triangles=triangles,source_parts=len(art.objects),bounds_m=bounds,materials=palette,
    source='Source/AmmoSupplyCrate.blend',fbx='Exports/'+NAME+'.fbx',pivot='bottom center',front='-Y',
    font_sources=['ArtSource/FPSDemo/Industrial/UI/Fonts/Rajdhani-Bold.ttf','ArtSource/FPSDemo/Industrial/UI/Fonts/MiSans-Bold.ttf'],
    sha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [blend,path]},
    scope='Static exterior asset; no gameplay interaction, no opening animation')
(ROOT/'AssetManifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
print('AMMO_CRATE_BUILD_PASS',triangles,'triangles',len(art.objects),'source parts',flush=True)
