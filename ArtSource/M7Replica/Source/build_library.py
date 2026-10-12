"""Reference-led M7 core and separately replaceable external art attachments.

The 2000x1000 official side view is traced in art coordinates. This is a visual
mesh library, not a mechanical design. Only the new library is written.
"""
import bpy, bmesh, math, json, sys
import numpy as np
from pathlib import Path
from mathutils import Vector

ROOT=Path(r'D:/Unreal Projects/MCP_Test/ArtSource/M7Replica')
sys.path.insert(0,str(ROOT/'Source'))
import replica_api as g

if bpy.data.collections.get('SM_M7_Core'):
    raise RuntimeError('M7 replica library already exists; use a fresh scene for rebuilding.')
edit=bpy.data.scenes.new('M7_Library_Edit')
bpy.context.window.scene=edit
g.enum_set(edit.unit_settings,'system','METRIC')
edit.unit_settings.scale_length=1

def p(px,py):return ((1250-px)*.0005,(380-py)*.0005)
MOUNTS={'Core':(0,0,0),'Stock':(-.179,0,-.0025),'Magazine':(.080,0,-.100),
        'RearGrip':(-.0625,0,-.065),'Optic':(0,0,.055),
        'Foregrip':(.3075,0,-.0435),'Muzzle':(.598,0,0),'Iron':(0,0,.055)}
registry={}

def lin(c):return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
TEX=ROOT/'Textures'
TEX.mkdir(exist_ok=True)
def tile(name,arr):
    im=bpy.data.images.new(name,width=arr.shape[1],height=arr.shape[0],alpha=True)
    im.colorspace_settings.name='Non-Color'
    im.pixels.foreach_set(arr.astype(np.float32).ravel())
    im.filepath_raw=str(TEX/(name+'.png'))
    im.file_format='PNG'
    im.save()
    im.pack()
    return im

def mat(key,label,color,metal,rough,grain=0):
    m=bpy.data.materials.new('M7R_'+label)
    m.use_nodes=True
    rgba=tuple(lin(c) for c in color)+(1,)
    m.diffuse_color=rgba
    n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    n.inputs['Base Color'].default_value=rgba
    n.inputs['Metallic'].default_value=metal
    n.inputs['Roughness'].default_value=rough
    if grain:
        rng=np.random.default_rng(117+len(label))
        a=np.ones((256,256,4),np.float32)
        noise=rng.normal(0,1,(256,256)).astype(np.float32)
        a[:,:,1]=np.clip(rough+noise*grain*.22,.08,.95)
        a[:,:,2]=metal
        orm=tile('M7R_'+key+'_ORM',a)
        nodes=m.node_tree.nodes
        links=m.node_tree.links
        tex=nodes.new('ShaderNodeTexImage');tex.image=orm
        sep=nodes.new('ShaderNodeSeparateColor')
        links.new(tex.outputs['Color'],sep.inputs['Color'])
        links.new(sep.outputs['Green'],n.inputs['Roughness'])
        links.new(sep.outputs['Blue'],n.inputs['Metallic'])
        norm=np.ones_like(a)
        nx=rng.normal(0,grain*.095,(256,256))
        ny=rng.normal(0,grain*.095,(256,256))
        norm[:,:,0]=.5+nx*.5;norm[:,:,1]=.5+ny*.5
        norm[:,:,2]=.5+.5*np.sqrt(np.clip(1-nx*nx-ny*ny,0,1))
        ni=tile('M7R_'+key+'_Normal',norm)
        tn=nodes.new('ShaderNodeTexImage');tn.image=ni
        nm=nodes.new('ShaderNodeNormalMap')
        nm.inputs['Strength'].default_value=.42
        links.new(tn.outputs['Color'],nm.inputs['Color'])
        links.new(nm.outputs['Normal'],n.inputs['Normal'])
    return m

mats={
    'metal':mat('metal','01_AnodizedBlack',(.13,.135,.14),.70,.42,.22),
    'polymer':mat('polymer','02_PolymerBlack',(.115,.12,.125),0,.60,.34),
    'rubber':mat('rubber','03_RubberBlack',(.075,.079,.083),0,.78,.43),
    'edge':mat('edge','04_DarkHardware',(.19,.195,.20),.76,.35,.12),
    'steel':mat('steel','05_BurnishedSteel',(.26,.27,.28),.85,.30,.10),
    'dark':mat('dark','06_RecessBlack',(.025,.028,.030),.18,.67,0),
    'label':mat('label','07_EngravingGray',(.30,.31,.32),.30,.50,0),
    'glass':mat('glass','08_Glass',(.17,.20,.23),0,.12,0),
    'lens':mat('lens','09_CoatedLens',(.09,.12,.145),.05,.13,0),
}
for key in ('glass','lens'):
    n=next(n for n in mats[key].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    n.inputs['Transmission Weight'].default_value=.96
    n.inputs['IOR'].default_value=1.45

def begin_asset(name,category,mount_key,is_default=False):
    assert name not in registry,name
    c=bpy.data.collections.new(name)
    edit.collection.children.link(c)
    root=bpy.data.objects.new(name+'_Root',None)
    c.objects.link(root)
    root.empty_display_size=.014
    mount=MOUNTS[mount_key]
    root['mount_key']=mount_key
    root['mount_position_m']=mount
    root['assembly_coordinates']=True
    c.instance_offset=mount
    c.asset_mark()
    c.asset_data.author='MCP_Test'
    c.asset_data.description=name+' | M7 visual component | '+category+' | black realistic exterior'
    for t in ('M7','Weapon',category,'Modular','Realistic'):c.asset_data.tags.new(t)
    registry[name]={'collection':c,'root':root,'category':category,'mount_key':mount_key,
                    'mount':mount,'is_default':is_default}
    g.configure(c,root,mats)
    return root

def profile_px(name,pts,width,mat,bevel=.0005):
    return g.profile(name,[p(*pt) for pt in pts],width,mat,bevel)
api={k:getattr(g,k) for k in ('box','cyl','tube','mesh','profile','text')}
api.update(mats=mats,p=p,profile_px=profile_px,begin_asset=begin_asset)
box,cyl,tube,mesh,profile,text=[api[k] for k in ('box','cyl','tube','mesh','profile','text')]

def bolt_px(name,px,py,side=1,rpx=5):
    x,z=p(px,py);y=side*.0293
    cyl(name+'_Head',(x,y,z),rpx*.0005,.0012,'edge','Y',16)
    cyl(name+'_Socket',(x,y+side*.00075,z),rpx*.00024,.00045,'dark','Y',6)

def slab_px(name,pts,side,mat='metal',depth=.0015,bevel=.00045):
    o=profile_px(name,pts,depth,mat,bevel)
    o.location.y=side
    return o

def bool_cut(target,cutters):
    bpy.ops.object.select_all(action='DESELECT')
    for o in cutters:o.select_set(True)
    bpy.context.view_layer.objects.active=cutters[0]
    if len(cutters)>1:bpy.ops.object.join()
    cutter=bpy.context.object
    bpy.context.view_layer.objects.active=target
    m=target.modifiers.new('Actual reference openings','BOOLEAN')
    g.enum_set(m,'operation','DIFFERENCE');g.enum_set(m,'solver','EXACT')
    m.object=cutter
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(cutter,do_unlink=True)

def cutpx(pts):return profile_px('TEMP_ReferenceCut',pts,.16,'dark',.00055)
def capsule(px0,px1,py0,py1):
    r=min((py1-py0)/2,(px1-px0)/2)
    return [(px0+r,py0),(px1-r,py0),(px1,py0+r),(px1,py1-r),
            (px1-r,py1),(px0+r,py1),(px0,py1-r),(px0,py0+r)]

begin_asset('SM_M7_Core','Core','Core',True)
# The actual M7 receiver contour and diagonal magwell are traced, not boxes.
upper=profile_px('CORE_UpperForgedContour',[(936,309),(1469,309),(1520,315),(1555,318),
    (1573,332),(1584,350),(1583,381),(1564,411),(1518,427),(1460,435),(1400,432),
    (1350,439),(1200,430),(995,434),(939,440),(920,432),(920,334)],.058,'metal',0)
# A real shallow charging-channel recess, visible on the official left side.
x,z=p(1100,347)
cut=box('TEMP_ChargingChannel',(x,.0298,z),(.094,.012,.0098),'dark',.003)
g.apply_mods(cut);bool_cut(upper,[cut]);g.bevel_obj(upper,.00065,3)
lower=profile_px('CORE_LowerForgedContour',[(957,417),(1465,414),(1522,414),(1569,401),
    (1583,414),(1586,446),(1563,470),(1514,489),(1472,491),(1448,517),(1403,524),
    (1370,559),(1330,540),(1237,537),(1224,582),(1210,608),(998,553),(992,546),
    (990,505),(980,487),(957,469)],.046,'metal',0)
hole=cutpx([(1230,537),(1262,527),(1298,526),(1322,541),(1331,565),(1329,590),
            (1298,607),(1255,610),(1228,600)])
g.apply_mods(hole);bool_cut(lower,[hole]);g.bevel_obj(lower,.00070,3)

# Sculpted upper ridges, long side guides and offset handle, both shell halves.
for side in (-1,1):
    slab_px('CORE_UpperShoulder_'+str(side),[(943,306),(1435,307),(1464,340),
        (1458,344),(972,345),(953,333)],side*.0279,'metal',.003,.0008)
    slab_px('CORE_LongGuide_'+str(side),[(1043,364),(1543,363),(1567,372),
        (1557,388),(1049,387),(1040,380)],side*.0301,'edge',.002,.0006)
    slab_px('CORE_LowerCastingRib_'+str(side),[(1003,418),(1493,417),(1470,436),
        (1410,451),(1246,450),(1205,465),(1001,452)],side*.024,'metal',.002,.0008)
    for px,py in ((984,381),(1020,346),(1182,346),(931,429),(1438,460)):
        bolt_px('CORE_ReceiverFastener_'+str(side),px,py,side,5)
    # Selector plate and the stepped bolt catch match the reference silhouette.
    slab_px('CORE_SelectorMount_'+str(side),[(1327,478),(1394,470),(1422,482),
        (1420,511),(1399,531),(1344,526),(1328,510)],side*.0244,'metal',.0025,.0007)
    x,z=p(1383,492)
    cyl('CORE_SelectorHub_'+str(side),(x,side*.0278,z),.007,.0035,'edge','Y',32)
    slab_px('CORE_SelectorLever_'+str(side),[(1337,487),(1387,482),(1393,488),
        (1381,500),(1336,500)],side*.030,'metal',.0025,.0007)
    for px,py in ((1341,511),(1405,513)):
        bolt_px('CORE_SelectorPlateBolt_'+str(side),px,py,side,3)
    slab_px('CORE_BoltCatchBase_'+str(side),[(1183,473),(1218,468),(1238,485),
        (1234,518),(1210,529),(1182,516)],side*.0255,'metal',.003,.0007)
    slab_px('CORE_BoltCatchButton_'+str(side),[(1201,424),(1223,421),(1232,430),
        (1227,454),(1218,458),(1199,448)],side*.029,'polymer',.006,.0009)
    for j in range(10):
        x,z=p(1214,428+j*2.2)
        box('CORE_CatchTraction',(x,side*.0328,z),(.009,.00065,.00075),'edge',.00016)
    slab_px('CORE_HorizontalCatch_'+str(side),[(1143,485),(1189,482),(1208,488),
        (1203,499),(1145,501),(1138,496)],side*.027,'metal',.007,.0008)
    x,z=p(1544,451)
    tube('CORE_RearQDOpening_'+str(side),(x,side*.026,z),.0135,.0094,.003,'edge','Y',48)
    cyl('CORE_RearQDRecess_'+str(side),(x,side*.024,z),.0095,.001,'dark','Y',40)
    x,z=p(1440,464)
    tube('CORE_SmallQDSocket_'+str(side),(x,side*.026,z),.0046,.0032,.002,'dark','Y',32)

# Side charging channel has a narrow bezel and a separate, recognizable handle.
slab_px('CORE_ChargingSlotBezel',capsule(999,1201,332,362),.0312,'edge',.0012,.0006)
slab_px('CORE_ChargingSlotInterior',capsule(1012,1189,338,355),.0321,'dark',.001,.0004)
for px in (1020,1182):bolt_px('CORE_ChannelScrew',px,346,1,5)
slab_px('CORE_ChargingHandle',[(947,369),(1045,369),(1047,395),(1004,398),
        (1004,384),(975,384),(975,400),(943,400)],.037,'metal',.008,.0007)
for px,py in ((979,382),(1058,390)):bolt_px('CORE_ChargingHandleFastener',px,py,1,4)
# Opposite side recess is separately formed, rather than mirrored lettering.
slab_px('CORE_OppositeEjectionPort',capsule(1060,1260,334,373),-.0301,'dark',.001,.0006)
slab_px('CORE_OppositeCover',[(1060,370),(1257,370),(1260,383),(1062,383)],-.032,'metal',.002,.0005)
x,z=p(1160,385);cyl('CORE_OppositeCoverHinge',(x,-.033,z),.0019,.095,'edge','X',24)

# The magwell bottom follows the reference's pronounced diagonal edge.
for side in (-1,1):
    slab_px('CORE_MagwellFlaredLip_'+str(side),[(992,538),(1207,595),(1206,611),
        (985,552)],side*.0245,'edge',.003,.0008)
    # Small laser marking dots create understated etched lines, not a nameplate.
    for row in range(4):
        for col in range(30-row*3):
            x,z=p(1023+col*2.1,489+row*4.5)
            box('CORE_QuietEtching',(x,side*.0243,z),(.00036,.00018,.00036),'label',.00006)

# An annular guard keeps the curved reference opening truly hollow.
out=[(1210,520),(1263,512),(1313,516),(1341,535),(1344,588),(1330,608),
     (1270,628),(1230,619),(1208,600)]
ins=[(1229,537),(1265,528),(1302,530),(1324,545),(1324,583),(1317,595),
     (1271,612),(1240,604),(1227,590)]
n=len(out)
vs=[(p(px,py)[0],y,p(px,py)[1]) for y in (-.010,.010) for contour in (out,ins) for px,py in contour]
fs=[]
for i in range(n):
    j=(i+1)%n
    fs.extend([(i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),
        (i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)])
mesh('CORE_CurvedTriggerGuard',vs,fs,'metal',.0008)
profile_px('CORE_CurvedTrigger',[(1290,525),(1300,526),(1302,548),(1298,569),
    (1289,590),(1281,591),(1290,571),(1294,549)],.0045,'steel',.00045)

# Default handguard. Its specific front trapezoids/long side slots differ from
# the tidal long handguard used in the rejected version.
guard=profile_px('CORE_DefaultHandguardOutline',[(176,287),(917,289),(927,308),
    (936,329),(935,440),(914,463),(895,470),(819,469),(786,449),(186,449),
    (176,420),(173,341)],.055,'metal',0)
inner=box('TEMP_HollowInterior',(.350,0,0),(.420,.039,.060),'dark',.004)
g.apply_mods(inner)
cuts=[inner]
for pts in (
    [(212,319),(265,319),(259,350),(207,350)],
    [(280,318),(343,318),(360,347),(274,347)],
    [(374,330),(402,330),(412,346),(383,346)],
    [(426,330),(454,330),(466,346),(435,346)],
    [(477,330),(504,330),(515,346),(486,346)],
):
    o=cutpx(pts);g.apply_mods(o);cuts.append(o)
for px0,px1 in ((261,391),(468,599),(677,807)):
    o=cutpx(capsule(px0,px1,372,395));g.apply_mods(o);cuts.append(o)
for px in (219,429,636):
    x,z=p(px,383);o=cyl('TEMP_CircleVent',(x,0,z),.0047,.14,'dark','Y',32)
    g.apply_mods(o);cuts.append(o)
for start in (257,301,354,399,445,490):
    o=cutpx([(start+16,424),(start+39,424),(start+22,440),(start,440)])
    g.apply_mods(o);cuts.append(o)
for offset in (0,32,64):
    o=cutpx([(782+offset,416),(800+offset,416),(833+offset,460),(815+offset,460)])
    g.apply_mods(o);cuts.append(o)
for start in (606,650,694,738,782):
    o=cutpx([(start,305),(start+32,305),(start+37,313),(start+4,313)])
    g.apply_mods(o);cuts.append(o)
for px0,px1 in ((800,849),(860,907)):
    o=cutpx(capsule(px0,px1,336,349));g.apply_mods(o);cuts.append(o)
bool_cut(guard,cuts);g.bevel_obj(guard,.00072,3)
for side in (-1,1):
    slab_px('CORE_HandguardShoulder_'+str(side),[(598,317),(927,318),(921,329),
        (605,329)],side*.027,'edge',.0011,.00035)
    slab_px('CORE_HandguardMidRib_'+str(side),[(178,403),(774,405),(792,418),
        (788,425),(186,420)],side*.0268,'metal',.001,.00035)
    x,z=p(839,383)
    tube('CORE_HandguardQD_'+str(side),(x,side*.0288,z),.0101,.0067,.0028,'edge','Y',48)
    cyl('CORE_HandguardQDInner_'+str(side),(x,side*.0274,z),.0067,.001,'dark','Y',40)
    for px,py in ((221,382),(927,431)):
        bolt_px('CORE_HandguardPin_'+str(side),px,py,side,4)

# Short default exposed barrel and actual small rail profile.
cyl('CORE_BarrelVisible',(.414,0,0),.0090,.372,'metal','X',64)
cyl('CORE_BarrelShoulder',(.537,0,0),.0125,.013,'edge','X',48)
cyl('CORE_BarrelShortFront',(.563,0,0),.0089,.072,'metal','X',64)
box('CORE_TopRailBase',(.180,0,.0472),(.684,.024,.0090),'metal',.00055)
for i in range(52):
    x=-.156+i*.0133
    profile('CORE_RailTooth_%02d'%i,[(x-.0045,.0485),(x-.0045,.0520),
        (x-.0032,.0545),(x+.0032,.0545),(x+.0045,.0520),(x+.0045,.0485)],.032,'metal',.0004)
box('CORE_LowerRailBase',(.323,0,-.0440),(.215,.026,.005),'metal',.00045)
for i in range(16):
    box('CORE_LowerRailTooth_%02d'%i,(.224+i*.013,0,-.0475),(.008,.032,.0035),'metal',.00035)
# Rear stock receiver face and its exposed folding socket.
slab_px('CORE_RearSocketFace',[(1536,316),(1598,326),(1610,350),(1608,427),
        (1588,471),(1560,471),(1572,425)],0,'metal',.047,.00075)
slab_px('CORE_RearSocketWeb',[(1567,321),(1596,329),(1595,344),(1569,339)],0,'edge',.030,.0005)
for side in (-1,1):
    bolt_px('CORE_RearSocketBolt_'+str(side),1585,380,side,4)

from furniture_pack import build_furniture_pack
from accessory_pack import build_accessory_pack
build_furniture_pack(api)
build_accessory_pack(api)
from refine_library import refine_geometry
refine_geometry()

# Explicit attachment transforms remain in the editable core asset.
core=registry['SM_M7_Core']
for key,position in MOUNTS.items():
    if key=='Core':continue
    socket=bpy.data.objects.new('M7_Mount_'+key,None)
    core['collection'].objects.link(socket)
    socket.parent=core['root']
    socket.location=position
    socket.empty_display_size=.012
    socket['attachment_category']=key

# Unwrap editable source pieces once; image maps travel with the exports.
bpy.ops.object.select_all(action='DESELECT')
meshes=[o for rec in registry.values() for o in rec['collection'].objects if o.type=='MESH']
for o in meshes:o.select_set(True)
bpy.context.view_layer.objects.active=meshes[0]
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.012,scale_to_bounds=True)
bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT')

# Definitions remain editable. The edit view initially shows the default parts.
for rec in registry.values():
    lc=edit.view_layers[0].layer_collection.children.get(rec['collection'].name)
    if lc:lc.exclude=not rec['is_default']

def instance(scene,name,pos):
    rec=registry[name]
    o=bpy.data.objects.new(name+'_Instance',None)
    scene.collection.objects.link(o)
    g.enum_set(o,'instance_type','COLLECTION')
    o.instance_collection=rec['collection']
    o.location=pos
    return o

assembly=bpy.data.scenes.new('M7_Default_Assembly')
g.enum_set(assembly.unit_settings,'system','METRIC')
assembly.unit_settings.scale_length=1
defaults=[name for name,r in registry.items() if r['is_default']]
for name in defaults:instance(assembly,name,registry[name]['mount'])

def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
def studio(scene,prefix,scale=1.10):
    world=bpy.data.worlds.new(prefix+'_World');world.use_nodes=True
    wn=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND')
    wn.inputs['Color'].default_value=(.105,.11,.115,1);wn.inputs['Strength'].default_value=.27
    scene.world=world
    for name,loc,power,size in [('Key',(.15,1.0,1.3),150,1.4),('Fill',(-.4,-.6,.5),80,1.0),('Rim',(.7,-.4,.5),105,.8)]:
        data=bpy.data.lights.new(prefix+'_'+name,'AREA');data.energy=power;data.size=size
        o=bpy.data.objects.new(data.name,data);scene.collection.objects.link(o);o.location=loc;aim(o,(.10,0,-.045))
    cd=bpy.data.cameras.new(prefix+'_Camera');cam=bpy.data.objects.new(cd.name,cd)
    scene.collection.objects.link(cam);cam.location=(-.64,1.9,.55);aim(cam,(.12,0,-.035))
    g.enum_set(cd,'type','ORTHO');cd.ortho_scale=scale;scene.camera=cam
    try:scene.render.engine='CYCLES'
    except TypeError as e:print(e)
    scene.cycles.samples=64;scene.cycles.use_denoising=True
    scene.render.resolution_x=2200;scene.render.resolution_y=1050;scene.render.resolution_percentage=100
    g.enum_set(scene.render.image_settings,'file_format','PNG')
    g.enum_set(scene.view_settings,'view_transform','AgX')
    try:scene.view_settings.look='AgX - Medium High Contrast'
    except TypeError:pass
    scene.view_settings.exposure=-.15
    return cam

studio(assembly,'M7_Default',1.13)
catalog=bpy.data.scenes.new('M7_Attachment_Catalog')
catalog.unit_settings.scale_length=1
for i,name in enumerate(registry):
    if name=='SM_M7_Core':continue
    j=i-1;col=j%4;row=j//4
    instance(catalog,name,((col-1.5)*.45,0,(1.5-row)*.31))
studio(catalog,'M7_Catalog',2.10)
catalog.camera.location=(0,2.6,.10);aim(catalog.camera,(0,0,.02))
catalog.render.resolution_x=2000;catalog.render.resolution_y=1400
assembly['default_parts']=json.dumps(defaults)
assembly['asset_library']='M7_Modular_Replica'
assembly['visual_reference']='Official_M7_Universe_Side.webp + Official_Garena_M7.jpg'
edit['scope']='Editable reference-led external visual geometry; individual collections are Blender assets.'
edit['pixel_reference_mapping']='2000x1000; X=(1250-px)*0.0005, Z=(380-py)*0.0005; art coordinates only'
rows=[]
for name,rec in registry.items():
    rows.append({'name':name,'category':rec['category'],'mount_key':rec['mount_key'],
        'mount_position_m':list(rec['mount']),'default':rec['is_default'],'collection':name,
        'editable_objects':len(rec['collection'].objects)})
manifest={'library':'M7_Modular_Replica','source_file':'M7_Modular_Replica.blend','axis':'+X muzzle, +Z up, metres',
    'default_parts':defaults,'assets':rows,'mounts':MOUNTS,'rigged':False,
    'reference':'Reference/ReferenceManifest.json','appearance':'Black realistic palette and externally traced M7 default contours',
    'verification_limits':'No official right-side/front/rear orthographic reference; hidden-side exterior is reconstructed from available views.',
    'unreal_import_status':'not imported or tested in Unreal Editor'}
(ROOT/'AssetManifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
bpy.context.window.scene=assembly
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_perspective='CAMERA'
        area.spaces.active.shading.type='MATERIAL'
data={edit,assembly,catalog}|{r['collection'] for r in registry.values()}
bpy.data.libraries.write(str(ROOT/'M7_Modular_Replica.blend'),data,fake_user=True,compress=True)
print(json.dumps({'built_assets':len(registry),'default_components':defaults,'file':str(ROOT/'M7_Modular_Replica.blend')},ensure_ascii=False))
