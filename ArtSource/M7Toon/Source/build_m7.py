"""Black M7-inspired stylized battle-rifle visual prop, external geometry only.

Execute in Blender. The earlier TIDELINE scene/file is not modified.
Reference: official Delta Force weapon assembly M7 artwork, rob_img6.png.
"""
import bpy
import math
import json
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(r'D:/Unreal Projects/MCP_Test/ArtSource/M7Toon')
ROOT.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(ROOT/'Source'))
import model_api as geo

scene=bpy.data.scenes.get('M7_BLACK_Studio')
if scene is None:
    scene=bpy.data.scenes.new('M7_BLACK_Studio')
bpy.context.window.scene=scene
asset=bpy.data.collections.get('ASSET_M7_BLACK')
if asset is None:
    asset=bpy.data.collections.new('ASSET_M7_BLACK')
    scene.collection.children.link(asset)
root=bpy.data.objects.get('M7_Root')
if root is None:
    root=bpy.data.objects.new('M7_Root',None)
    asset.objects.link(root)
if len(asset.objects)>1:
    raise RuntimeError('The M7 source already contains editable geometry. Rebuild in a fresh scene.')
geo.enum_set(scene.unit_settings,'system','METRIC')
scene.unit_settings.scale_length=1

def lin(c):return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def material(name,color,metal=0,rough=.4):
    m=bpy.data.materials.new('M7_'+name)
    m.use_nodes=True
    rgba=tuple(lin(c) for c in color)+(1,)
    m.diffuse_color=rgba
    n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
    n.inputs['Base Color'].default_value=rgba
    n.inputs['Metallic'].default_value=metal
    n.inputs['Roughness'].default_value=rough
    return m

mats={
    'metal':material('01_BlackAnodized',(.105,.111,.118),.64,.36),
    'polymer':material('02_CharcoalPolymer',(.125,.130,.136),.0,.55),
    'rubber':material('03_BlackRubber',(.060,.065,.069),.0,.76),
    'edge':material('04_GraphiteHardware',(.175,.180,.187),.72,.34),
    'steel':material('05_DarkSatinSteel',(.25,.255,.265),.82,.32),
    'dark':material('06_DeepRecess',(.032,.035,.039),.15,.64),
    'label':material('07_MutedEngraving',(.34,.35,.36),.32,.56),
    'glass':material('08_OpticGlass',(.13,.16,.20),.0,.10),
    'lens':material('09_LensCoating',(.085,.115,.145),.05,.12),
    'reticle':material('10_DimReticle',(.46,.12,.08),.0,.55),
}
gn=next(n for n in mats['glass'].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
gn.inputs['Transmission Weight'].default_value=.98
gn.inputs['IOR'].default_value=1.45
ln=next(n for n in mats['lens'].node_tree.nodes if n.type=='BSDF_PRINCIPLED')
ln.inputs['Transmission Weight'].default_value=.92
ln.inputs['IOR'].default_value=1.45
geo.configure(asset,root,mats)
api={k:getattr(geo,k) for k in ('box','cyl','tube','mesh','profile','text')}
api['mats']=mats
box,cyl,tube,mesh,profile,text=[api[k] for k in ('box','cyl','tube','mesh','profile','text')]

def bolt(name,loc,r=.0028,axis='Y',mat='edge'):
    o=cyl(name+'_Head',loc,r,.0016,mat,axis,12)
    if axis=='Y':
        y=loc[1]+(-.001 if loc[1]<0 else .001)
        box(name+'_HexSlot',(loc[0],y,loc[2]),(r*.8,.0005,r*.22),'dark',.0001)
    return o

def cut_union(target,cutters):
    bpy.ops.object.select_all(action='DESELECT')
    for o in cutters:o.select_set(True)
    bpy.context.view_layer.objects.active=cutters[0]
    bpy.ops.object.join()
    cutter=bpy.context.object
    bpy.context.view_layer.objects.active=target
    m=target.modifiers.new('Real external cutouts','BOOLEAN')
    geo.enum_set(m,'operation','DIFFERENCE')
    geo.enum_set(m,'solver','EXACT')
    m.object=cutter
    bpy.ops.object.modifier_apply(modifier=m.name)
    bpy.data.objects.remove(cutter,do_unlink=True)

def cutter_box(loc,dims,angle=0):
    o=box('M7_TEMP_Cutter',loc,dims,'dark',.004)
    o.rotation_euler[1]=angle
    geo.apply_mods(o)
    return o

# Long, stepped MCX-family upper with a continuous, very low top rail.
profile('Receiver_Upper',[(-.180,-.029),(-.180,.030),(-.161,.051),(-.122,.057),
        (.154,.057),(.175,.044),(.175,-.024),(.129,-.036),(-.135,-.036)],.065,'metal',.0024)
profile('Receiver_Lower',[(-.176,-.026),(.137,-.026),(.144,-.046),(.125,-.080),
        (.018,-.086),(.001,-.060),(-.055,-.060),(-.070,-.078),(-.117,-.060),(-.164,-.054)],.058,'metal',.0018)
box('Receiver_Magwell',(.076,0,-.075),(.114,.063,.033),'metal',.0023)
profile('Receiver_RearHingeTransition',[(-.182,.021),(-.171,.038),(-.156,.039),
        (-.156,-.054),(-.178,-.059),(-.191,-.039)],.074,'edge',.0016)
cyl('Stock_HingePin',(-.184,0,-.008),.010,.082,'edge','Z',32)
for z in (-.044,.025):
    cyl('Stock_HingePinCap',(-.184,0,z),.011,.004,'metal','Z',32)
box('Stock_HingeFoldLatch',(-.19,-.036,-.005),(.017,.006,.031),'steel',.0011)
for s in (-1,1):
    # Thin, stepped side rails follow the receiver casting instead of a plate.
    p=profile('Receiver_UpperSideRib_'+str(s),[(-.159,.029),(-.151,.046),(.124,.046),
        (.144,.038),(.141,.031),(-.133,.032)],.0024,'edge',.0008)
    p.location.y=s*.0329
    box('Receiver_UpperSeam_'+str(s),(-.015,s*.0332,.004),(.280,.0015,.004),'dark',.0004)
    box('Receiver_LowerCastingLine_'+str(s),(-.006,s*.030,-.028),(.259,.002,.003),'edge',.0005)
    for x,z in [(-.146,-.033),(.113,-.042),(-.17,.012)]:
        bolt('Receiver_CastingPin_'+str(s),(x,s*.033,z),.0030)
    cyl('Receiver_SelectorDisc_'+str(s),(-.082,s*.032,-.041),.008,.004,'metal','Y',32)
    lever=box('Receiver_SelectorLever_'+str(s),(-.090,s*.035,-.044),(.023,.0055,.0055),'edge',.001)
    lever.rotation_euler[1]=-.36
    for a in (-.7,0,.7):
        box('Receiver_SelectorEngraving',(-.082+math.sin(a)*.012,s*.033,-.041+math.cos(a)*.012),(.0011,.0005,.002),'label',.0002)

# Distinct offset charging-handle recess, long upper slot, separate hardware.
box('Receiver_LeftChargingChannel',(.001,-.0332,.016),(.203,.003,.013),'dark',.0017)
box('Receiver_ChargingRail',(.002,-.0351,.020),(.199,.003,.0028),'edge',.0005)
cyl('Receiver_SideChargeBoss',(.108,-.039,.014),.0048,.016,'edge','Y',24)
profile('Receiver_SideChargeHandle',[(.095,.018),(.103,.025),(.134,.025),(.144,.018),(.142,.004),(.128,.003),(.122,.012)],.014,'metal',.0014).location.y=-.047
for x in (.131,.136,.141):
    box('Receiver_HandleSerration',(x,-.055,.011),(.001,.001,.013),'dark',.0002)
box('Receiver_RightPort',(-.032,.034,.013),(.119,.003,.027),'dark',.002)
box('Receiver_PortInner',(-.024,.036,.013),(.095,.002,.018),'steel',.0012)
box('Receiver_PortDustLid',(-.029,.039,-.004),(.115,.005,.008),'metal',.0011)
cyl('Receiver_DustHinge',(-.027,.039,-.008),.0026,.108,'edge','X',20)
box('Receiver_RearChargingBase',(-.145,0,.051),(.033,.071,.008),'metal',.0013)
for s in (-1,1):
    box('Receiver_RearChargingEar',(-.157,s*.035,.047),(.020,.009,.008),'rubber',.0013)
    box('Receiver_BoltCatch',(-.008,s*.032,-.034),(.014,.007,.020),'metal',.0014)
    for j in range(4):
        box('Receiver_ControlRib',(-.008,s*.036,-.040+j*.004),(.010,.0009,.001),'edge',.0002)
text('Receiver_ModelMark','M7',(-.153,-.0343,-.019),.0065,'label',(math.pi/2,0,0))
text('Receiver_SubtleSerial','BATTLE / 07  •  0186',(-.147,-.0343,-.026),.0027,'label',(math.pi/2,0,0))

# Visible opening around the trigger, with a slim curved trigger blade.
profile('Trigger_Guard',[(-.054,-.057),(-.048,-.057),(-.041,-.091),(.006,-.091),
        (.014,-.060),(.021,-.060),(.016,-.096),(.006,-.099),(-.041,-.099),(-.048,-.094)],.012,'metal',.0014)
profile('Trigger_Blade',[(-.014,-.055),(-.008,-.055),(-.010,-.074),(-.019,-.086),
        (-.024,-.085),(-.017,-.074)],.006,'steel',.0009)

# Extended faceted handguard with the game's long, open lower silhouette.
outer=[(-.025,.055),(.025,.055),(.039,.040),(.039,-.021),(.024,-.039),(-.024,-.039),(-.039,-.021),(-.039,.040)]
inner=[(-.023,.048),(.023,.048),(.032,.036),(.032,-.017),(.021,-.031),(-.021,-.031),(-.032,-.017),(-.032,.036)]
vertices=[(x,y,z) for x,ring in [(.164,outer),(.518,outer),(.164,inner),(.518,inner)] for y,z in ring]
faces=[]
for i in range(8):
    j=(i+1)%8
    faces.extend([(i,j,8+j,8+i),(16+i,24+i,24+j,16+j),(i,16+i,16+j,j),(8+i,8+j,24+j,24+i)])
guard=mesh('Handguard_ExtendedSlenderShell',vertices,faces,'metal',0)
cutters=[]
# Small upper ventilation diamonds, long lower slots, slanted nose vents.
for i in range(10):
    x=.190+i*.027
    cutters.append(cutter_box((x,0,.035),(.020,.12,.012),-.17))
for x in (.216,.288,.360,.432):
    cutters.append(cutter_box((x,0,-.024),(.058,.12,.011)))
for x in (.469,.491,.508):
    cutters.append(cutter_box((x,0,.001),(.012,.12,.040),-.58))
cut_union(guard,cutters)
geo.bevel_obj(guard,.0012,3)
cyl('Barrel_InternalVisual',(.369,0,.012),.0105,.398,'dark','X',48)

for s in (-1,1):
    p=profile('Handguard_TransitionInset_'+str(s),[(.175,.018),(.271,.018),(.276,.010),(.267,-.008),(.175,-.008)],.002,'polymer',.0007)
    p.location.y=s*.039
    box('Handguard_SideCoverBase_'+str(s),(.365,s*.041,.006),(.187,.006,.031),'rubber',.0015)
    for i in range(23):
        x=.277+i*.0079
        # Alternating chevron facets create understated molded traction.
        for sign in (-1,1):
            rib=box('Handguard_CoverChevron',(x,s*.0455,.006+sign*.006),(.0018,.0022,.016),'polymer',.0006)
            rib.rotation_euler[1]=sign*.55
    for x in (.183,.250,.472):
        bolt('Handguard_Hardware_'+str(s),(x,s*.0405,.007),.0027)
    cyl('Handguard_QDSocket_'+str(s),(.237,s*.041,-.004),.0045,.0025,'dark','Y',24)
    tube('Handguard_QDSocketLip_'+str(s),(.237,s*.042,-.004),.005,.0035,.0015,'edge','Y',32)
    text('Handguard_QuietMark_'+str(s),'M7 / 07',(.189,s*.041,.020),.0032,'label',(math.pi/2 if s<0 else -math.pi/2,0,0))

# Low machined top rail; teeth are deliberately restrained and continuous.
box('Rail_MainSpine',(.166,0,.0575),(.676,.027,.007),'metal',.001)
for i in range(48):
    x=-.164+i*.0142
    profile('Rail_TopTooth_%02d'%i,[(x-.0048,.060),(x-.0048,.064),
        (x-.0035,.067),(x+.0035,.067),(x+.0048,.064),(x+.0048,.060)],.038,'metal',.0005)
box('Rail_Underside',(.328,0,-.041),(.173,.029,.005),'metal',.0007)
for i in range(12):
    box('Rail_UnderTooth_%02d'%i,(.251+i*.014,0,-.045),(.008,.036,.005),'metal',.0006)

# Long exterior barrel and compact muzzle, clearly visible forward of guard.
cyl('Barrel_LongExposed',(.626,0,.012),.0095,.229,'metal','X',48)
for x,r,depth in [(.520,.014,.012),(.536,.0115,.012),(.726,.0125,.008)]:
    cyl('Barrel_Shoulder',(x,0,.012),r,depth,'edge','X',32)
tube('Muzzle_CompactHollowBody',(.753,0,.012),.0145,.008,.048,'metal','X',48)
tube('Muzzle_RearRing',(.731,0,.012),.015,.008,.006,'edge','X',40)
tube('Muzzle_FrontLip',(.779,0,.012),.0145,.008,.0035,'edge','X',48)
for i in range(6):
    a=i*math.tau/6
    o=box('Muzzle_VentInset',( .754,.0141*math.sin(a),.012+.0141*math.cos(a)),(.028,.004,.0014),'dark',.0004)
    o.rotation_euler[0]=-a

# Small swept foregrip with the real reference's triangular empty opening.
profile('Foregrip_OpenTriangle',[(.269,-.048),(.376,-.048),(.383,-.059),(.375,-.079),
        (.350,-.084),(.329,-.140),(.302,-.140),(.278,-.104),(.269,-.076),
        (.282,-.067),(.294,-.097),(.313,-.124),(.324,-.125),(.344,-.078),(.365,-.071),(.365,-.063),(.280,-.063)],.026,'polymer',.0018)
box('Foregrip_RailClamp',(.321,0,-.050),(.116,.035,.008),'metal',.0012)
for s in (-1,1):
    for x,z in [(.28,-.063),(.366,-.065),(.313,-.134),(.29,-.097)]:
        bolt('Foregrip_MountBolt',(x,s*.014,z),.0023)
    for i in range(8):
        z=-.088-i*.0045
        x=.282+i*.0025
        o=box('Foregrip_BackTraction',(x,s*.0138,z),(.007,.0014,.0019),'rubber',.0005)
        o.rotation_euler[1]=.4

from furniture_m7 import build_furniture
from optic_m7 import build_optic
build_furniture(api)
build_optic(api)

# Named component parents, evaluated before preserving child world transforms.
prefixes=[('Stock_','Stock',(-.18,0,.0)),('Grip_','Grip',(-.08,0,-.06)),
    ('Magazine_','Magazine',(.07,0,-.075)),('Optic_','Optic',(0,0,.07)),
    ('SightRear_','RearSight',(-.15,0,.067)),('SightFront_','FrontSight',(.48,0,.067)),
    ('Foregrip_','Foregrip',(.32,0,-.04)),('Muzzle_','Muzzle',(.73,0,.012)),('Trigger_','Trigger',(-.01,0,-.055))]
parents={}
for prefix,name,pivot in prefixes+[('','ReceiverHandguardBarrel',(0,0,0))]:
    p=bpy.data.objects.new('M7_PART_'+name,None)
    asset.objects.link(p)
    p.parent=root
    p.location=pivot
    p.empty_display_size=.017
    parents[name]=p
bpy.context.view_layer.update()
for o in list(asset.objects):
    if o.type not in ('MESH','FONT'):continue
    name=next((name for prefix,name,pivot in prefixes if o.name.startswith(prefix)),'ReceiverHandguardBarrel')
    world=o.matrix_world.copy()
    o.parent=parents[name]
    o.matrix_world=world
bpy.context.view_layer.update()
asset.asset_mark()
asset.asset_data.description='M7 battle rifle inspired stylized visual prop. Realistic silhouette, black/charcoal materials, editable modular parts.'
asset.asset_data.author='MCP_Test'
for tag in ('M7','Battle Rifle','Stylized','Black','Modular'):asset.asset_data.tags.new(tag)
root['asset_id']='M7_BLACK'
root['axis']='metres; +X muzzle / +Z up'
root['reference_url']='https://www.playdeltaforce.com/events/moddedfirearms/en/'
root['scope']='Static visual prop; external details only; no animation rig.'

# Neutral, warm-free studio: black materials remain black while edges read.
studio=bpy.data.collections.new('M7_STUDIO_PreviewOnly')
scene.collection.children.link(studio)
world=bpy.data.worlds.new('M7_NeutralWorld')
world.use_nodes=True
wn=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND')
wn.inputs['Color'].default_value=(.12,.12,.12,1)
wn.inputs['Strength'].default_value=.28
scene.world=world
def aim(o,target):o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
def light(name,loc,power,size,color):
    d=bpy.data.lights.new(name,'AREA')
    d.energy=power
    d.size=size
    d.color=color
    o=bpy.data.objects.new(name,d)
    studio.objects.link(o)
    o.location=loc
    aim(o,(.13,0,-.025))
light('M7_Key',(.25,-.85,1.15),130,1.3,(1,1,1))
light('M7_Fill',(.55,.85,.50),95,1.1,(.90,.94,1))
light('M7_Rim',(-.7,.20,.60),95,.9,(1,1,1))
backdrop=material('Studio_Backdrop',(.31,.33,.35),0,.86)
data=bpy.data.meshes.new('M7_StudioGroundMesh')
data.from_pydata([(-200,-200,-.235),(200,-200,-.235),(200,200,-.235),(-200,200,-.235)],[],[(0,1,2,3)])
data.materials.append(backdrop)
ground=bpy.data.objects.new('M7_StudioGround',data)
studio.objects.link(ground)
camdata=bpy.data.cameras.new('M7_Camera')
cam=bpy.data.objects.new('M7_Camera',camdata)
studio.objects.link(cam)
cam.location=(.95,-2.1,.72)
aim(cam,(.13,0,-.025))
geo.enum_set(camdata,'type','ORTHO')
camdata.ortho_scale=1.40
scene.camera=cam
try:scene.render.engine='CYCLES'
except TypeError as e:print(e)
scene.cycles.samples=64
scene.cycles.use_denoising=True
scene.render.resolution_x=2000
scene.render.resolution_y=1100
scene.render.resolution_percentage=100
geo.enum_set(scene.render.image_settings,'file_format','PNG')
geo.enum_set(scene.view_settings,'view_transform','AgX')
try:scene.view_settings.look='AgX - Medium High Contrast'
except TypeError:pass
scene.view_settings.exposure=-.10
scene.render.filepath=str(ROOT/'Preview_Hero.png')
scene['asset_collection']='ASSET_M7_BLACK'
scene['preview_only_collection']='M7_STUDIO_PreviewOnly'
bpy.ops.object.select_all(action='DESELECT')
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':
        area.spaces.active.region_3d.view_perspective='CAMERA'
        geo.enum_set(area.spaces.active.shading,'type','MATERIAL')
# Library writing isolates this asset scene from the preserved earlier version.
bpy.data.libraries.write(str(ROOT/'M7_BLACK_Stylized.blend'),{scene},fake_user=True,compress=True)
print(json.dumps({'stage':'M7_model_built','editable_objects':len(asset.all_objects),'file':str(ROOT/'M7_BLACK_Stylized.blend')},ensure_ascii=False))
