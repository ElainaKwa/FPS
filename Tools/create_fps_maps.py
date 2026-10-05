"""Author two separate playable maps using Unreal Editor APIs. Keeps the lesson map intact."""
import unreal

ROOT='/Game/FPSDemo/Maps'
assets=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')

def material(name,color):
    path='/Game/FPSDemo/MapMaterials/'+name
    if assets.does_asset_exist(path): return unreal.load_asset(path)
    mat=tools.create_asset(name,'/Game/FPSDemo/MapMaterials',unreal.Material,unreal.MaterialFactoryNew())
    node=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property('constant',unreal.LinearColor(*color,1))
    unreal.MaterialEditingLibrary.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant)
    rough.set_editor_property('r',.72)
    unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assets.save_asset(path)
    return mat

mats={n:material('M_Map_'+n,c) for n,c in {
    'Floor':(.10,.13,.16),'Wall':(.23,.29,.34),'Dark':(.025,.04,.06),
    'Blue':(.025,.30,.65),'Red':(.65,.07,.035),'Gold':(.75,.42,.07),
    'White':(.7,.78,.8),'Cargo':(.12,.28,.28)}.items()}

def box(name,loc,size,mat='Wall',solid=True):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*loc))
    a.set_actor_label(name)
    comp=a.static_mesh_component
    comp.set_static_mesh(cube)
    comp.set_material(0,mats[mat])
    comp.set_collision_profile_name('BlockAll' if solid else 'NoCollision')
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
    return a

def text(label,loc,yaw=180,size=45,color=(180,220,240,255)):
    a=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*loc),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label('Sign_'+label)
    c=a.text_render
    c.set_text(label)
    c.set_world_size(size)
    c.set_text_render_color(unreal.Color(*color))
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)

def marker(tag,loc,yaw=0,cls=unreal.PlayerStart):
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),unreal.Rotator(pitch=0,yaw=yaw,roll=0))
    a.set_actor_label(tag)
    a.set_editor_property('tags',[tag])
    return a

def environment(length,width):
    box('Floor',(0,0,-25),(length,width,50),'Floor')
    for side in (-1,1):
        box('SideWall',(0,side*(width/2+20),190),(length+80,40,430))
        box('EndWall',(side*(length/2+20),0,190),(40,width,430))
        for x in range(-int(length/2)+100,int(length/2),500):
            box('WallPillar',(x,side*(width/2-12),170),(30,35,340),'Dark')
            box('WallStripe',(x,side*(width/2-32),200),(240,4,12),'Gold',False)
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,600),unreal.Rotator(pitch=-55,yaw=-35,roll=0))
    light.light_component.set_editor_property('intensity',4.0)
    sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,700))
    sky.light_component.set_editor_property('intensity',1.0)
    sky.light_component.set_editor_property('real_time_capture',True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,0))

def rack(x,y,model,yaw=0):
    box('WeaponPlinth',(x,y,18),(115,100,36),'Dark')
    box('WeaponStripe',(x,y,37),(110,90,2),'Gold',False)
    # Explicit source configuration, independent of enum ordinals.
    definitions = {
        1: '/Game/FPSDemo/Weapons/Definitions/DA_AK',
        2: '/Game/FPSDemo/Weapons/Definitions/DA_M4',
        3: '/Game/FPSDemo/Weapons/Definitions/DA_MP5',
        4: '/Game/FPSDemo/Weapons/Definitions/DA_UMP45',
        5: '/Game/FPSDemo/Weapons/Definitions/DA_M9',
        6: '/Game/FPSDemo/Weapons/Definitions/DA_HeavyPistol',
    }
    definition = unreal.load_asset(definitions[model])
    assert definition, definitions[model]
    stand = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.FPSWeaponStand, unreal.Vector(x,y,75), unreal.Rotator(0,yaw,0))
    stand.set_actor_label('WeaponStand_' + str(model))
    stand.set_editor_property('definition', definition)
    stand.set_editor_property('display_order', model-1)

def create(name,training):
    path=ROOT+'/'+name
    # Never replace a user's existing level on a repeat invocation.
    if assets.does_asset_exist(path):
        raise RuntimeError('Map already exists, edit deliberately instead: '+path)
    assert levels.new_level(path)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/MCP_Test.FPSMatchMode'))
    if training:
        environment(6000,2400)
        marker('FPSTrainingPlayer',(-1900,0,100))
        text('RANGE / SOLO',(-2800,0,270),0,75)
        text('F : PICK UP    R : RELOAD',(-2400,0,190),0,35)
        for i in range(3):
            y=(i-1)*650
            x=-900+i*1000
            marker('FPSTarget'+str(i),(x,y,100),180)
            box('TargetPad',(x,y,2),(180,200,4),'Red',False)
            box('TargetBackstop',(2600,y,170),(100,500,340),'Dark')
            text(str((i+1)*10)+' m',(x+90,y,180),180,55)
            for markx in range(-1400,2400,500):
                box('LaneGuide',(markx,y,1),(220,8,2),'Gold',False)
        for i in range(6): rack(-2650,-875+i*350,i+1)
        for y in (-1000,1000):
            box('SafetyDivider',(300,y,40),(4600,30,80),'Wall')
        box('FiringLine',(-1900,0,1),(15,2100,2),'Blue',False)
    else:
        environment(4800,3200)
        for side in (-1,1):
            color='Blue' if side<0 else 'Red'
            for i in range(5): marker('FPS'+color+str(i),(side*2050,-1000+i*500,100),0 if side<0 else 180)
            # Spawn shield; routes leave through the two side openings.
            box('SpawnShield',(side*1610,0,145),(70,720,290),color)
            text('BLUE BASE' if side<0 else 'RED BASE',(side*2320,0,260),0 if side<0 else 180,65)
            for i in range(6): rack(side*2280,-1050+i*420,i+1,0 if side<0 else 180)
            for y in (-800,800):
                box('Cargo',(side*750,y,110),(520,300,220),'Cargo')
                for offset in range(-200,250,90):
                    box('CargoRib',(side*750+offset,y,112),(12,310,225),'Dark')
                box('LowCover',(side*250,y*1.45,50),(250,100,100),'Gold')
            box('CenterCover',(side*540,360*side,55),(240,100,110),'Wall')
        for y in (-1460,1460):
            box('FlankRoute',(0,y,1),(4400,12,2),'Gold',False)
        box('CenterMark',(0,0,1),(180,180,2),'Gold',False)
        text('FOUNDRY / 5 v 5',(0,1540,300),-90,70)
        text('05:00 / ELIMINATION',(0,-1540,300),90,60)
    assert levels.save_current_level()
    unreal.log('FPS_MAP_CREATED '+path)

create('LV_FPSFoundry',False)
create('LV_FPSTraining',True)
unreal.log('FPS_MAPS COMPLETE')
