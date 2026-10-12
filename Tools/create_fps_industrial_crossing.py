"""Author Industrial Crossing through Unreal Editor APIs. Preserve existing levels by default.

The generator is authoritative until the map is handed over for manual editing.
Uses the shared industrial palette and the editable CrossingKit; no Valve assets.
"""
import json
import math
import random
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
MAP = '/Game/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing'
ART = '/Game/FPSDemo/Art/Environment/CrossingKit'
OUT = ROOT / 'Saved/IndustrialCrossing'
OUT.mkdir(parents=True, exist_ok=True)
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
rng = random.Random(131026)
if (ROOT/'Content/FPSDemo/Maps/Gameplay/IndustrialCrossing/LV_FPSIndustrialCrossing.umap').exists():
    # Explicit rebuilding is restricted to this generator's labelled output and receipt.
    assert '-IndustrialCrossingRebuild' in unreal.SystemLibrary.get_command_line(), 'Existing map preserved: ' + MAP
    receipt = json.loads((OUT/'Authoring.json').read_text(encoding='utf-8'))
    assert receipt['map'] == MAP and receipt['source'] == 'Tools/create_fps_industrial_crossing.py'
    assert levels.load_level(MAP)
    assert any(a.get_actor_label() == 'IC_Navigation_112x96m' for a in actors.get_all_level_actors())
    for old in actors.get_all_level_actors():
        if not isinstance(old,unreal.WorldSettings): actors.destroy_actor(old)
else:
    assert levels.new_level(MAP)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', unreal.FPSMatchMode)
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
cylinder = unreal.load_asset('/Engine/BasicShapes/Cylinder')
sphere = unreal.load_asset('/Engine/BasicShapes/Sphere')
count = 0


mats = {name:unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/MI_IND_'+name)
        for name in ('Concrete','Ceramic','Graphite','Steel','Yellow','Rubber','Blue','Red','Light')}
assert all(mats.values())
kit_meshes = {name:unreal.load_asset(ART+'/Meshes/SM_IC_'+name)
              for name in ('Portal600','Cargo180','Vent150','ServicePipe400','Bollard120')}
assert all(kit_meshes.values()), 'Import CrossingKit before generating the level'


def mesh(name, loc, size, mat='Concrete', solid=True, rot=None, shape=None, folder='Architecture'):
    global count
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), rot or unreal.Rotator())
    a.set_actor_label('IC_' + name)
    a.set_folder_path('IndustrialCrossing/' + folder)
    c = a.static_mesh_component
    c.set_static_mesh(shape or cube)
    c.set_material(0, mats[mat])
    c.set_collision_profile_name('BlockAll' if solid else 'NoCollision')
    c.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    count += 1
    return a


def sign(label, loc, yaw=180, size=65, color=(235,215,175,255)):
    a = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*loc), unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label('IC_Sign_' + label.replace('\n', '_'))
    a.set_folder_path('IndustrialCrossing/Wayfinding')
    c = a.text_render
    c.set_text(label)
    c.set_world_size(size)
    c.set_text_render_color(unreal.Color(*color))
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    return a


def prop(name,loc,scale=(1,1,1),yaw=0,solid=True):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc),unreal.Rotator(yaw=yaw))
    a.set_actor_label('IC_'+name)
    a.set_folder_path('IndustrialCrossing/Kit')
    a.static_mesh_component.set_static_mesh(kit_meshes[name])
    a.static_mesh_component.set_collision_profile_name('BlockAll' if solid else 'NoCollision')
    a.static_mesh_component.set_editor_property('mobility',unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a


def crate(x,y,z=0,size=160):
    prop('Cargo180',(x,y,z),(size/180,)*3)


def portal(x,y,width=650,height=550,yaw=0,door=False):
    """Steel service portal; origin on the floor, aperture points along local X."""
    prop('Portal600',(x,y,0),(1,width/600,height/566),yaw)
    if door:
        for side in (-1,1):
            mesh('FixedOpenGate',(x+100,y+side*(width/2-10),200),(220,24,400),'Graphite')
            mesh('GateWarning',(x+100,y+side*(width/2-23),125),(215,3,16),'Yellow',False)


# Rectangles form the clear ground plane. Remaining 2 m cells become solid buildings.
# Coordinates are centimetres; the 112 x 96 m footprint is seven times Foundry's area.
streets = [
    (-5400,-3600,-1400,1400), (-4800,-3800,-4000,4000),
    (-4600,4400,3000,4000), (2400,4600,1600,4000),
    (4000,5400,-1800,1800), (4000,5000,-4000,4000),
    (1600,4400,-4200,-2200), (-4400,2200,-3800,-2800),
    (-4200,4400,-600,600), (-800,0,0,2200), (-800,3000,1600,2400),
    (-2000,-1200,-3200,-400), (1800,2600,-2800,0),
]


def open_cell(x,y):
    return any(a <= x < b and c <= y < d for a,b,c,d in streets)


mesh('Ground', (0,0,-40), (11200,9600,80), 'Concrete', folder='Ground')
mesh('DistantApron', (0,0,-180), (150000,150000,100), 'Concrete', False, folder='Skyline')
for side in (-1,1):
    for offset in (-9000,-3000,3000,9000):
        mesh('DistantWarehouse',(side*15000,offset,1200),(4400,2200,2600),'Graphite',False,folder='Skyline')
# Flush ground detailing is visual only, preserving the Z=0 navigation surface.
for a,b,c,d in streets:
    mesh('StreetPaving', ((a+b)/2,(c+d)/2,.12), (b-a,d-c,.2), 'Concrete', False, folder='Ground')
for side in (-1,1):
    mesh('Perimeter', (side*5600,0,650), (160,9760,1300), 'Graphite')
    mesh('Perimeter', (0,side*4800,650), (11200,160,1300), 'Graphite')

occupied = {(i,j) for i in range(56) for j in range(48) if not open_cell(-5500+i*200,-4700+j*200)}
blocks = []
while occupied:
    i,j = min(occupied)
    ni = i+1
    # Modular facility buildings with varied roof heights and readable major silhouettes.
    while ni < i+7 and (ni,j) in occupied:
        ni += 1
    nj = j+1
    while nj < j+7 and all((u,nj) in occupied for u in range(i,ni)):
        nj += 1
    for u in range(i,ni):
        for v in range(j,nj): occupied.remove((u,v))
    x0,x1,y0,y1 = -5600+i*200,-5600+ni*200,-4800+j*200,-4800+nj*200
    x,y,w,d = (x0+x1)/2,(y0+y1)/2,x1-x0,y1-y0
    h = rng.choice((640,760,880,1040))
    tone = rng.choice(('Concrete','Concrete','Concrete','Ceramic'))
    mesh('FacilityBlock', (x,y,h/2), (w,d,h), tone)
    mesh('ImpactSkirt',(x,y,45),(w+4,d+4,90),'Graphite',False,folder='Facades')
    mesh('RoofFascia', (x,y,h-50), (w+10,d+10,35), 'Steel', False)
    mesh('RoofCap', (x,y,h+10), (w,d,20), 'Graphite')
    blocks.append((x,y,w,d,h))
    # Vents and structural columns remain outside gameplay collision.
    for axis,edge,other0,other1,out in [('X',x0,y0,y1,-1),('X',x1,y0,y1,1),('Y',y0,x0,x1,-1),('Y',y1,x0,x1,1)]:
        for q in range(int(other0+180),int(other1-70),500):
            px,py = (edge+out*35,q) if axis=='X' else (q,edge+out*35)
            if not open_cell(px,py): continue
            wx,wy = (edge+out*3,q) if axis=='X' else (q,edge+out*3)
            yaw = (0 if out<0 else 180) if axis=='X' else (90 if out<0 else -90)
            prop('Vent150',(wx,wy,420),yaw=yaw,solid=False)
            dims = (12,20,h) if axis=='X' else (20,12,h)
            loc = (wx,wy-170,h/2) if axis=='X' else (wx-170,wy,h/2)
            mesh('FacadeColumn',loc,dims,'Steel',False,folder='Facades')
    if w >= 800 and d >= 800 and rng.random() < .55:
        mesh('HVAC', (x,y,h+110), (240,320,200), 'Graphite', True, folder='Skyline')
        prop('ServicePipe400',(x,y+240,h+25),solid=False)

# Plant exhaust stacks are outside player circulation; yellow only marks service access.
for x,y in [(1000,-4400),(3000,4400),(-3000,4400)]:
    mesh('ExhaustBase', (x,y,500), (420,420,1000), 'Concrete', folder='Skyline')
    mesh('ExhaustStack',(x,y,1250),(240,240,1600),'Steel',True,shape=cylinder,folder='Skyline')
    mesh('StackBand',(x,y,1850),(248,248,90),'Yellow',False,shape=cylinder,folder='Skyline')

# Mid gate has a generous central opening; the original flow test's -400 to +400 firing lane stays clear.
portal(1000,0,560,590,door=True)
mesh('MidGateWingN',(1000,470,285),(160,260,570),'Graphite')
mesh('MidGateWingS',(1000,-470,285),(160,260,570),'Graphite')
portal(-3000,3500,520,550,door=True)
mesh('LongGateWingN',(-3000,3910,275),(160,180,550),'Concrete')
mesh('LongGateWingS',(-3000,3090,275),(160,180,550),'Concrete')
portal(1600,-3300,620,580)
portal(-3000,-3300,620,580)

# B service passage: steel frames and alternating roof panels admit daylight.
for x in range(-2800,1600,600):
    portal(x,-3300,680,560)
    mesh('TunnelRoof',(x,-3300,660),(340,1000,100),'Graphite')
    mesh('ServiceLight',(x,-3660,335),(90,24,20),'Light',False,folder='Details')
    lamp = actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(x,-3300,440))
    lamp.set_folder_path('IndustrialCrossing/Lighting')
    lamp.light_component.set_editor_property('intensity',45.0)
    lamp.light_component.set_editor_property('attenuation_radius',750.0)
    lamp.light_component.set_editor_property('light_color',unreal.Color(235,243,255,255))

# Cover is distributed to break long sightlines without plugging a route.
for x,y,s in [(-2300,3500,180),(-400,3740,150),(1400,3230,150),
              (3200,2800,220),(3470,2800,160),(2800,1950,160),(4280,3650,190),
              (2400,-3600,200),(2650,-3600,150),(3700,-2500,220),(1800,-2500,160),
              (-2700,-390,160),(-400,440,140),(2900,400,170),
              (-4370,2550,150),(-4200,-2000,170),(4720,2600,160)]:
    crate(x,y,size=s)
mesh('BlueSpawnShield',(-4050,0,170),(100,820,340),'Graphite',folder='Cover')
mesh('RedSpawnShield',(4360,0,170),(100,820,340),'Graphite',folder='Cover')

# A loading bay canopy. Narrow supports stay outside route centres.
for x in (2850,3450,4050):
    mesh('LoadingCanopy',(x,3950,420),(560,320,20),'Graphite',False,folder='Details')
    mesh('CanopyEdge',(x,3790,420),(560,12,20),'Yellow',False,folder='Details')
    mesh('CanopyPost',(x-270,3820,210),(12,12,420),'Steel',folder='Details')
    prop('Bollard120',(x-255,3720,0))
for y in (-4100,-3900):
    mesh('LoadingPallet',(3100,y,15),(220,110,30),'Graphite',folder='Details')
for x,y in [(3000,3400),(3000,-3000)]:
    for dx,dy,w,d in [(0,-300,600,10),(0,300,600,10),(-300,0,10,600),(300,0,10,600)]:
        mesh('SiteOutline',(x+dx,y+dy,.6),(w,d,.3),'Yellow',False,folder='Wayfinding')
sign('A  /  LOADING BAY',(2410,2700,370),0,70,(220,190,75,255))
sign('B  /  SERVICE YARD',(1650,-4190,340),90,85,(220,190,75,255))
sign('MID  /  DOUBLE GATE',(1050,0,557),0,32)
sign('A  >  LONG',(-3550,3010,255),90,55)
sign('B  >  SERVICE',(-3810,-2600,260),180,55)
sign('A  /  SHORT',(-790,1300,280),0,55)
sign('LOGISTICS / CROSSING',(-5350,0,530),0,85)
for y in (3160,3840,-2980,-3620):
    for x in range(-1800,1700,400):
        mesh('RouteDash',(x,y,.7),(170,7,.3),'Yellow',False,folder='Wayfinding')

# Explicit native gameplay setup and per-map navigation. No changes to existing maps or shared rules.
setup = actors.spawn_actor_from_class(unreal.FPSModeSetup,unreal.Vector(0,0,1200))
setup.set_editor_property('rules',unreal.load_asset('/Game/FPSDemo/Modes/DA_TeamMatch'))
setup.set_folder_path('IndustrialCrossing/Gameplay')
nav = actors.spawn_actor_from_class(unreal.FPSNavigationRegion,unreal.Vector())
nav.set_editor_property('grid_origin',unreal.Vector2D(-5500,-4700))
nav.set_editor_property('grid_size',unreal.IntPoint(111,95))
# Return to shared combat junctions between flank visits. A single clockwise loop on
# this larger footprint can leave equally fast opposing bots perpetually chasing.
patrol = [(0,0,90),(3200,2200,90),(-500,0,90),(3200,-3200,90),(0,0,90),
          (-1800,3500,90),(-500,0,90),(-800,-3300,90),(-400,2000,90),(0,0,90),(-1600,-1400,90)]
nav.set_editor_property('patrol_points',[unreal.Vector(*p) for p in patrol])
nav.set_actor_label('IC_Navigation_112x96m')
nav.set_folder_path('IndustrialCrossing/Gameplay')
for team,x in [(0,-4900),(1,4920)]:
    starts=[]
    prefix = 'FPSBlue' if team==0 else 'FPSRed'
    for i in range(5):
        p = actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(x,-600+i*300,100),unreal.Rotator(yaw=0 if team==0 else 180))
        p.set_editor_property('tags',[prefix+str(i)])
        p.set_actor_label(prefix+str(i))
        p.set_folder_path('IndustrialCrossing/Gameplay')
        starts.append(p)
    base = actors.spawn_actor_from_class(unreal.FPSBaseRegion,unreal.Vector(x,0,150))
    base.set_editor_property('team_id',team)
    base.bounds.set_box_extent(unreal.Vector(320,1000,150))
    base.set_editor_property('starts',starts)
    base.set_editor_property('fallback_start',starts[2])
    base.set_folder_path('IndustrialCrossing/Gameplay')
    sign('BLUE  /  SAFE BASE' if team==0 else 'RED  /  SAFE BASE',(x+(-400 if team==0 else 400),0,310),0 if team==0 else 180,55,(70,160,235,255) if team==0 else (235,90,60,255))
    for i,weapon in enumerate(('AK','M4','MP5','UMP45','M9','HeavyPistol','BR3')):
        wx,wy = x+(-470 if team==0 else 410),-1050+i*350
        mesh('WeaponPlinth',(wx,wy,18),(90,100,36),'Graphite',folder='Gameplay')
        mesh('WeaponTeamStripe',(wx,wy,37),(86,94,2),'Blue' if team==0 else 'Red',False,folder='Gameplay')
        stand = actors.spawn_actor_from_class(unreal.FPSWeaponStand,unreal.Vector(wx,wy,80))
        stand.set_editor_property('definition',unreal.load_asset('/Game/FPSDemo/Weapons/Definitions/DA_'+weapon))
        stand.set_editor_property('display_order',team*7+i)
        stand.set_folder_path('IndustrialCrossing/Gameplay')

sun = actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-48,yaw=-32,roll=0))
sun.light_component.set_editor_property('intensity',5.0)
sun.light_component.set_editor_property('atmosphere_sun_light',True)
sun.light_component.set_editor_property('light_color',unreal.Color(255,248,233,255))
sun.light_component.set_editor_property('light_source_angle',2.5)
sky = actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,2000))
sky.light_component.set_editor_property('intensity',1.1)
sky.light_component.set_editor_property('real_time_capture',True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector())
post = actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector())
post.set_editor_property('unbound',True)
settings = post.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_min_brightness',True)
settings.set_editor_property('override_auto_exposure_max_brightness',True)
settings.set_editor_property('auto_exposure_min_brightness',1.0)
settings.set_editor_property('auto_exposure_max_brightness',1.0)
post.set_editor_property('settings',settings)
for i,(loc,target) in enumerate([
    ((-9600,-11000,13500),(0,0,0)), ((-2000,3500,175),(3500,3400,180)),
    ((3300,1750,200),(3400,3600,350)), ((-2500,-3320,170),(1000,-3300,200)),
    ((-1300,100,180),(1600,0,250)),
]):
    a = actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(*loc),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)))
    a.set_actor_label('IC_Camera_'+str(i))
    a.set_editor_property('tags',['CrossingCapture'+str(i)])
    a.camera_component.set_field_of_view(70 if i==0 else 85)
    a.set_folder_path('IndustrialCrossing/Cameras')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-8500,-9500,12000),unreal.Rotator(pitch=-48,yaw=48,roll=0))
assert levels.save_current_level()
assets.save_directory(ART,only_if_is_dirty=True,recursive=True)
report={'map':MAP,'footprint_cm':[11200,9600],'primitive_actors':count,'buildings':len(blocks),'navigation_cells':[111,95],
        'player_starts':10,'weapon_stands':14,'patrol_points':patrol,'source':'Tools/create_fps_industrial_crossing.py','status':'authored; runtime verification separate'}
(OUT/'Authoring.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('INDUSTRIAL_CROSSING AUTHORING PASS '+MAP+' actors='+str(count))
