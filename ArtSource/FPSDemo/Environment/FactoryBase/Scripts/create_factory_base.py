"""Author the independent 28 x 24 m walkable factory through Unreal Editor APIs.
The generator is authoritative for this initial layout. Existing output is preserved
unless -FactoryBaseRebuild is explicitly passed and a matching receipt is present.
"""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[5]
MAP = '/Game/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase'
SOURCE = 'ArtSource/FPSDemo/Environment/FactoryBase/Scripts/create_factory_base.py'
OUT = ROOT/'Saved/FactoryBase'
OUT.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset=unreal.EditorAssetLibrary
map_file=ROOT/'Content/FPSDemo/Maps/Gameplay/FactoryBase/LV_FPSFactoryBase.umap'
protected={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'Content/FPSDemo').rglob('*') if p.suffix in ('.umap','.uasset') and p!=map_file}
if map_file.exists():
    assert '-FactoryBaseRebuild' in unreal.SystemLibrary.get_command_line(),'Existing factory preserved'
    receipt=json.loads((OUT/'Authoring.json').read_text(encoding='utf-8'))
    assert receipt['map']==MAP and receipt['source']==SOURCE
    assert levels.load_level(MAP)
    assert any(a.get_actor_label()=='FB_Layout_28x24m' for a in actors.get_all_level_actors())
    # Only this generator's actors may be replaced; manual additions cause a stop.
    assert all(isinstance(a,unreal.WorldSettings) or a.get_actor_label().startswith('FB_') for a in actors.get_all_level_actors())
    for a in actors.get_all_level_actors():
        if not isinstance(a,unreal.WorldSettings):actors.destroy_actor(a)
else:assert levels.new_level(MAP)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.FPSFactoryMode)
cube=unreal.load_asset('/Engine/BasicShapes/Cube');cylinder=unreal.load_asset('/Engine/BasicShapes/Cylinder')
mats={n:unreal.load_asset('/Game/FPSDemo/Art/Industrial/Materials/MI_IND_'+n) for n in ('Concrete','Graphite','Steel','Yellow','Rubber','Light','Ceramic')}
assert all(mats.values())
meshes={n:unreal.load_asset('/Game/FPSDemo/Art/Industrial/Environment/SM_IND_'+n) for n in ('Floor400','Wall400','Pillar300','Cargo240','WeaponRack','WorkLight','BaseBeacon','Railing200')}
assert all(meshes.values())
count=0

def spawn(cls,name,loc=(0,0,0),rot=None,folder='Architecture'):
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rot or unreal.Rotator());a.set_actor_label('FB_'+name);a.set_folder_path('FactoryBase/'+folder);return a

def box(name,loc,size,mat='Graphite',solid=True,shape=None,rot=None,folder='Architecture'):
    global count
    a=spawn(unreal.StaticMeshActor,name,loc,rot,folder);c=a.static_mesh_component;c.set_static_mesh(shape or cube);c.set_material(0,mats[mat]);c.set_collision_profile_name('BlockAll' if solid else 'NoCollision');c.set_editor_property('mobility',unreal.ComponentMobility.STATIC);a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)));count+=1;return a

def prop(name,loc,scale=(1,1,1),yaw=0,solid=True):
    global count
    a=spawn(unreal.StaticMeshActor,name,loc,unreal.Rotator(yaw=yaw),'SharedKit');a.static_mesh_component.set_static_mesh(meshes[name]);a.static_mesh_component.set_collision_profile_name('BlockAll' if solid else 'NoCollision');a.static_mesh_component.set_editor_property('mobility',unreal.ComponentMobility.STATIC);a.set_actor_scale3d(unreal.Vector(*scale));count+=1;return a

def text(value,loc,yaw=0,size=35,color=(232,225,192,255)):
    a=spawn(unreal.TextRenderActor,'Sign_'+value.replace('\n','_'),loc,unreal.Rotator(yaw=yaw),'Wayfinding');c=a.text_render;c.set_text(value);c.set_world_size(size);c.set_text_render_color(unreal.Color(*color));c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER);return a

def lamp(name,loc,intensity=9000,radius=950,color=(232,243,255,255)):
    a=spawn(unreal.PointLight,name,loc,folder='Lighting');c=a.light_component;c.set_mobility(unreal.ComponentMobility.MOVABLE);c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS);c.set_intensity(intensity);c.set_attenuation_radius(radius);c.set_light_color(unreal.LinearColor(color[0]/255,color[1]/255,color[2]/255,1));c.set_cast_shadows(False);return a

# Flush tiled deck and simple blocking envelope. Clear central aisle is 12 m wide.
for x in range(-1200,1201,400):
 for y in range(-1000,1001,400):prop('Floor400',(x,y,0))
for x in range(-1200,1201,400):
 prop('Wall400',(x,-1200,0),(1,1,2.2))
 if abs(x)>=600:prop('Wall400',(x,1200,0),(1,1,2.2))
for x in (-1400,1400):
 for y in range(-1000,1001,400):prop('Wall400',(x,y,0),(1,1,2.2),90)
box('Ceiling',(0,0,680),(2840,2440,40),'Graphite')
for y in (-1000,-400,400,1000):
 box('RoofBeam',(0,y,610),(2800,32,60),'Steel')
 for x in (-1350,1350):prop('Pillar300',(x,y,0),(1.5,1.5,2.1))
 for x in (-620,620):
  box('OverheadStrip',(x,y,573),(420,24,8),'Light',False,folder='Lighting');lamp('Overhead',(x,y,515),12000,1250)
# Overhead service pipes and cable trunk run outside the open walking lane.
for x in (-1150,1150):
 box('CableTray',(x,0,535),(90,2200,24),'Graphite',False)
 for dx in (-20,20):box('ServicePipe',(x+dx,0,566),(11,11,2200),'Steel',False,cylinder,unreal.Rotator(pitch=90))
# North departure door, shutters, lit header and a control kiosk.
box('DepartureDoor',(0,1200,240),(790,65,480),'Steel')
for z in range(35,480,40):box('ShutterSlat',(0,1159,z),(750,12,3),'Graphite',False)
for x in (-415,415):box('DoorJamb',(x,1180,255),(45,110,510),'Yellow')
box('DoorHeader',(0,1170,525),(880,100,48),'Graphite');text('04  /  DEPLOY',(0,1108,500),-90,58)
box('DoorLamp',(0,1110,548),(600,15,12),'Light',False);lamp('DoorFill',(0,850,380),4500,700)
box('DispatchPedestal',(0,950,52),(110,90,104),'Graphite');box('DispatchScreen',(0,933,124),(105,65,8),'Steel',True,rot=unreal.Rotator(pitch=18));box('DispatchDisplay',(0,918,133),(86,42,3),'Light',False,rot=unreal.Rotator(pitch=18))
# West warehouse, shelves and actual shared cargo meshes.
for y in (250,730):
 for z in (12,170,335):box('StorageShelf',(-1220,y,z),(260,410,14),'Steel')
 for dy in (-195,195):box('StorageUpright',(-1340,y+dy,195),(14,14,390),'Yellow')
 for z in (20,180):prop('Cargo240',(-1210,y,z),(.75,.75,.75),90)
box('StorageHeader',(-1307,500,460),(12,600,78),'Graphite',False);text('01  /  STORAGE',(-1297,500,444),0,39)
box('WarehouseTerminal',(-990,430,60),(140,100,120),'Graphite')
box('WarehouseDisplay',(-908,430,139),(8,72,54),'Light',False)

# West gunsmith bench: tool wall, shelf, installed shared model and inspection lights.
box('GunBench',(-1110,-610,82),(360,310,22),'Steel')
for x in (-1255,-965):
 for y in (-735,-485):box('BenchLeg',(x,y,38),(18,18,76),'Graphite')
box('ToolBoard',(-1335,-610,180),(22,360,190),'Graphite');prop('WeaponRack',(-1230,-600,94),(.85,.85,.85),90)
box('BenchLamp',(-1210,-610,310),(90,260,12),'Light',False);lamp('BenchTask',(-1040,-610,285),2400,500,(255,239,212,255));box('GunsmithHeader',(-1307,-610,440),(12,600,78),'Graphite',False);text('02  /  GUNSMITH',(-1297,-610,424),0,39)
# A shared, fixed M4 identifies the station; the actual loadout is shown in its live UI.
gun=spawn(unreal.StaticMeshActor,'WorkbenchM4',(-1160,-600,118),unreal.Rotator(yaw=90),'SharedKit')
gun.static_mesh_component.set_static_mesh(unreal.load_asset('/Game/FPSDemo/Art/Industrial/Weapons/SM_IND_M4'))
gun.static_mesh_component.set_collision_profile_name('NoCollision');count+=1
box('ToolBox',(-1050,-705,113),(70,48,38),'Graphite')
box('ToolBoxLid',(-1050,-705,134),(72,50,5),'Yellow',False)
box('BenchTablet',(-1050,-510,106),(90,63,8),'Steel',False,rot=unreal.Rotator(pitch=20))
# East processing cell: enclosed recycler, conveyors, motors and service cabinet.
box('Processor',(1140,330,135),(350,430,270),'Graphite')
box('ProcessorFace',(947,330,160),(32,340,180),'Steel');box('ProcessorWindow',(925,330,185),(8,240,85),'Rubber',False)
for y in (170,490):box('SafetyFrame',(911,y,160),(12,14,180),'Yellow',False)
box('ProcessorBase',(1120,330,20),(410,480,40),'Steel')
for y in (240,420):box('DriveMotor',(1090,y,305),(90,90,100),'Steel',True,cylinder,unreal.Rotator(pitch=90))
for y in range(-100,151,45):box('ConveyorRoller',(1090,y,75),(34,34,275),'Steel',True,cylinder,unreal.Rotator(pitch=90))
box('ConveyorBase',(1090,15,45),(310,330,35),'Graphite');box('ConveyorTrim',(921,15,72),(12,330,18),'Yellow',False)
box('FabricationHeader',(1307,350,460),(12,700,78),'Graphite',False);text('03  /  FABRICATION',(1297,350,444),180,39);lamp('ProcessorTask',(800,300,340),3800,650,(255,237,210,255))
# Maintenance/service corner, central flush docking outline, and clear floor guidance.
box('ServiceCabinet',(1210,-760,105),(160,290,210),'Steel');box('CabinetDoor',(1118,-760,110),(12,240,165),'Graphite')
prop('WorkLight',(1080,-1010,0));prop('Cargo240',(720,-930,0),(.8,.8,.8))
text('MAINTENANCE',(1380,-740,340),180,30)
for x in (-560,560):
 for y in range(-1000,950,200):box('AisleDash',(x,y,1.3),(7,95,.4),'Yellow',False,folder='Wayfinding')
for x,y,w,d in ((0,-500,560,6),(0,100,560,6),(-280,-200,6,600),(280,-200,6,600)):box('DockOutline',(x,y,1.4),(w,d,.4),'Steel',False,folder='Wayfinding')
for x in (-120,120):box('DockGuide',(x,-200,1.5),(18,230,.4),'Yellow',False,folder='Wayfinding')
text('PERSONAL FACTORY',(-0,-1170,455),90,62)
# Four independent facility markers, positioned in free space above work surfaces.
for name,page,loc,yaw in [('仓库与整备','Warehouse',(-850,430,145),180),('枪械改装','Modification',(-850,-610,145),180),('加工制造','Production',(850,330,145),0),('出征整备','Overview',(0,850,145),90)]:
 a=spawn(unreal.FPSFactoryStation,'Station_'+page,loc,unreal.Rotator(yaw=yaw),'Gameplay');a.set_editor_property('page',page);a.set_editor_property('display_name',unreal.Text(name))
start=spawn(unreal.PlayerStart,'Layout_28x24m',(0,-720,100),unreal.Rotator(yaw=90),'Gameplay')
# Indoor ambient fill; no combat actors, nav regions or loot spawners.
sky=spawn(unreal.SkyLight,'Ambient',(0,0,600),folder='Lighting');sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sky.light_component.set_intensity(.18)
post=spawn(unreal.PostProcessVolume,'Exposure',folder='Lighting');post.set_editor_property('unbound',True);s=post.get_editor_property('settings')
s.set_editor_property('override_auto_exposure_min_brightness',True);s.set_editor_property('override_auto_exposure_max_brightness',True);s.set_editor_property('auto_exposure_min_brightness',5.3);s.set_editor_property('auto_exposure_max_brightness',5.3);post.set_editor_property('settings',s)
for i,(loc,target) in enumerate([((0,-1040,335),(0,240,190)),((-420,-740,260),(-1120,-540,155)),((320,-540,280),(1010,250,160)),((0,-500,210),(-1140,560,190))]):
 a=spawn(unreal.CameraActor,'Camera_'+str(i),loc,unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)),'Cameras');a.camera_component.set_field_of_view(90 if i==0 else 80);a.set_editor_property('tags',['FactoryCamera'+str(i)])
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(0,-1040,335),unreal.Rotator(pitch=-7,yaw=90))
assert levels.save_current_level()
changed=[p for p,h in protected.items() if hashlib.sha256((ROOT/p).read_bytes()).hexdigest()!=h]
assert not changed,changed
report={'map':MAP,'source':SOURCE,'source_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'footprint_cm':[2800,2400],'ceiling_cm':660,'facility_pages':['Warehouse','Modification','Production','Overview'],'static_meshes':count,'protected_packages':len(protected),'protected_packages_unchanged':not changed,'status':'saved; runtime verification separate'}
(OUT/'Authoring.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
(ROOT/'ArtSource/FPSDemo/Environment/FactoryBase/Manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('FPS_FACTORY AUTHORING PASS '+MAP)
