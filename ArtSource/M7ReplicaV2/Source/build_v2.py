"""Split the M7 prop's front assembly and build Tidal/reference grip variants."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OLD=ROOT.parent/'M7Replica'
sys.path.insert(0,str(ROOT/'Source'))
import replica_api as g

MOUNTS={'Core':(0,0,0),'Stock':(-.179,0,-.0025),'Magazine':(.080,0,-.100),
 'RearGrip':(-.0625,0,-.065),'Optic':(0,0,.055),'Barrel':(.158,0,0),
 'Foregrip':(.3075,0,-.0435),'Muzzle':(.598,0,0),'Iron':(0,0,.055)}
DEFAULT_LOCAL={'Foregrip':(.1495,0,-.0435),'Muzzle':(.440,0,0),'FrontIron':(.3595,0,.055)}
TIDAL_LOCAL={'Foregrip':(.189,0,-.041),'Muzzle':(.636,0,0),'FrontIron':(.394,0,.055)}

def main():
    if bpy.data.collections.get('SM_M7_Barrel_Default'):
        raise RuntimeError('V2 assets already exist; refuse an overlapping rebuild.')
    if not bpy.data.collections.get('SM_M7_Core'):
        raise RuntimeError('Open the original M7 asset library before running V2.')
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'M7_Modular_Replica_V2.blend'),compress=True)
    old=json.loads((OLD/'AssetManifest.json').read_text(encoding='utf-8'))
    edit=bpy.data.scenes['M7_Library_Edit']
    bpy.context.window.scene=edit
    for layer in edit.view_layers[0].layer_collection.children:layer.exclude=False
    edit.view_layers[0].update()
    mats={key:bpy.data.materials[name] for key,name in {
      'metal':'M7R_01_AnodizedBlack','polymer':'M7R_02_PolymerBlack',
      'rubber':'M7R_03_RubberBlack','edge':'M7R_04_DarkHardware',
      'steel':'M7R_05_BurnishedSteel','dark':'M7R_06_RecessBlack',
      'label':'M7R_07_EngravingGray','glass':'M7R_08_Glass','lens':'M7R_09_CoatedLens'}.items()}
    registry={row['name']:{'collection':bpy.data.collections[row['collection']],
      'root':bpy.data.objects[row['name']+'_Root'],'row':{
       key:row[key] for key in ('name','category','mount_key','mount_position_m','default','collection')}}
      for row in old['assets'] if row['name']!='SM_M7_Foregrip_K1'}
    new_objects=[]
    def begin_asset(name,category,mount_key,is_default=False):
        c=bpy.data.collections.new(name);edit.collection.children.link(c)
        root=bpy.data.objects.new(name+'_Root',None);c.objects.link(root)
        root.empty_display_size=.014
        root['mount_key']=mount_key;root['mount_position_m']=MOUNTS[mount_key]
        c.instance_offset=MOUNTS[mount_key]
        c.asset_mark();c.asset_data.author='MCP_Test'
        c.asset_data.description=name+' | independently replaceable M7 exterior art asset'
        for tag in ('M7',category,'Modular','Black'):c.asset_data.tags.new(tag)
        registry[name]={'collection':c,'root':root,'row':{'name':name,'category':category,
          'mount_key':mount_key,'mount_position_m':list(MOUNTS[mount_key]),
          'default':is_default,'collection':name}}
        g.configure(c,root,mats)
        return root
    def box(*args,**kwargs):
        o=g.box(*args,**kwargs);new_objects.append(o);return o
    def profile(*args,**kwargs):
        o=g.profile(*args,**kwargs);new_objects.append(o);return o
    def cyl(*args,**kwargs):
        o=g.cyl(*args,**kwargs);new_objects.append(o);return o
    def tube(*args,**kwargs):
        o=g.tube(*args,**kwargs);new_objects.append(o);return o
    def cut(target,cutters):
        # Sequential solids avoid intersecting cutters being treated as
        # disconnected self-intersections, which can leave extruded caps.
        for cutter in cutters:
            g.apply_mods(cutter)
            if cutter in new_objects:new_objects.remove(cutter)
            bpy.context.view_layer.objects.active=target
            mod=target.modifiers.new('Reference exterior openings','BOOLEAN')
            g.enum_set(mod,'operation','DIFFERENCE');g.enum_set(mod,'solver','EXACT');mod.object=cutter
            if hasattr(mod,'use_self'):mod.use_self=True
            bpy.ops.object.modifier_apply(modifier=mod.name)
            bpy.data.objects.remove(cutter,do_unlink=True)
    def capsule(x0,x1,z0,z1):
        r=min((x1-x0)/2,(z1-z0)/2)
        return [(x0+r,z0),(x1-r,z0),(x1,z0+r),(x1,z1-r),
                (x1-r,z1),(x0+r,z1),(x0,z1-r),(x0,z0+r)]
    def move(o,collection,root):
        for owner in list(o.users_collection):owner.objects.unlink(o)
        collection.objects.link(o);o.parent=root
    def socket(collection,root,name,position,key):
        o=bpy.data.objects.new(name,None);collection.objects.link(o);o.parent=root
        o.location=position;o.empty_display_size=.012;o['mount_key']=key
        return o

    # Preserve the original standard front as a complete, removable component.
    begin_asset('SM_M7_Barrel_Default','Barrel','Barrel',True)
    standard=registry['SM_M7_Barrel_Default'];core=registry['SM_M7_Core']
    for o in list(core['collection'].objects):
        if (o.name=='CORE_DefaultHandguardOutline' or o.name.startswith('CORE_Handguard')
           or o.name.startswith('CORE_Barrel') or o.name.startswith('CORE_LowerRail')
           or (o.name.startswith('CORE_RailTooth_') and sum(v.co.x for v in o.data.vertices)/len(o.data.vertices)>.158)):
            move(o,standard['collection'],standard['root'])
    old_base=bpy.data.objects.get('CORE_TopRailBase')
    if old_base:bpy.data.objects.remove(old_base,do_unlink=True)
    g.configure(core['collection'],core['root'],mats)
    box('CORE_ReceiverTopRailBase',(-.002,0,.0472),(.320,.024,.009),'metal',.00055)
    g.configure(standard['collection'],standard['root'],mats)
    box('BARREL_Default_FrontTopRailBase',(.340,0,.0472),(.364,.024,.009),'metal',.00055)
    for key in ('Foregrip','Muzzle'):
        obj=bpy.data.objects.get('M7_Mount_'+key)
        if obj:
            move(obj,standard['collection'],standard['root'])
            obj.name='M7_BarrelDefault_Mount_'+key
    socket(core['collection'],core['root'],'M7_Mount_Barrel',MOUNTS['Barrel'],'Barrel')
    socket(standard['collection'],standard['root'],'M7_BarrelDefault_Mount_FrontIron',(.5175,0,.055),'FrontIron')
    standard['row']['local_mounts_m']={k:list(v) for k,v in DEFAULT_LOCAL.items()}
    standard['row']['display_name']='标准枪管与护木组合'

    # Tidal: the long exposed cylinder and compact slanted front guard are
    # traced from the official combo/detail, without copying a game mesh.
    begin_asset('SM_M7_Barrel_Tidal','Barrel','Barrel',False)
    tidal=registry['SM_M7_Barrel_Tidal']
    tidal['row']['display_name']='M7 堤风超长枪管组合'
    tidal['row']['local_mounts_m']={k:list(v) for k,v in TIDAL_LOCAL.items()}
    outline=[(.156,.038),(.567,.038),(.586,.033),(.602,.011),
      (.604,.001),(.568,-.036),(.546,-.042),(.230,-.042),
      (.207,-.035),(.156,-.034)]
    guard=profile('TIDAL_HandguardShell',outline,.055,'metal',0)
    cutters=[box('TEMP_TidalHollow',(.378,0,-.001),(.418,.039,.067),'dark',.002)]
    # Eleven repeated slanted upper apertures, plus the rear circular port.
    for i in range(11):
        x=.287+i*.026
        cutters.append(profile('TEMP_TidalUpperVent',[(x-.010,.016),(x+.008,.016),
          (x+.012,.029),(x-.006,.029)],.14,'dark',.00065))
    cutters.append(cyl('TEMP_TidalUpperCircularPort',(.263,0,.023),.0042,.14,'dark','Y',40))
    for x in (.267,.358,.449):
        cutters.append(profile('TEMP_TidalLowerSlot',capsule(x-.039,x+.039,-.029,-.018),.14,'dark',.001))
    for i in range(3):
        x=.552-i*.023
        cutters.append(profile('TEMP_TidalFrontDiagonal',[(x+.022,.000),(x+.012,.000),
          (x-.023,-.032),(x-.012,-.032)],.14,'dark',.0006))
    # Three small keyhole-shaped side openings ahead of the broad side web.
    for x in (.506,.529,.552):
        cutters.append(cyl('TEMP_TidalKeyCircle',(x,0,.001),.0030,.14,'dark','Y',32))
        cutters.append(box('TEMP_TidalKeyStem',(x+.004,0,.001),(.008,.14,.0038),'dark',.0005))
    cut(guard,cutters);g.bevel_obj(guard,.00075,4)
    for side in (-1,1):
        # Long quiet shoulder and lower reinforcement, with hardware in black.
        o=profile('TIDAL_UpperSideShoulder',[(.175,.014),(.492,.014),(.512,.007),(.175,.004)],.0014,'metal',.00045)
        o.location.y=side*.0272
        o=profile('TIDAL_LowerReinforcement',[(.175,-.009),(.512,-.009),(.537,-.014),(.501,-.017),(.175,-.017)],.0012,'metal',.0004)
        o.location.y=side*.0268
        tube('TIDAL_QDSocket',(.225,side*.029,0),.0070,.0046,.0028,'edge','Y',48)
        cyl('TIDAL_QDDarkInterior',(.225,side*.0276,0),.0046,.001,'dark','Y',40)
        for x,z in ((.171,-.027),(.170,.017),(.579,.006)):
            cyl('TIDAL_ShellFastener',(x,side*.0289,z),.0032,.0012,'edge','Y',24)
            cyl('TIDAL_FastenerSocket',(x,side*.0296,z),.0014,.00025,'dark','Y',6)
    # Visible exterior barrel and cosmetic gas tube, matching the reference's
    # long smooth projection. These are visual solids with no internal action.
    cyl('TIDAL_BarrelMain',(.472,0,0),.0092,.636,'metal','X',80)
    tube('TIDAL_ExternalFrontCrown',(.790,0,0),.0093,.0036,.009,'steel','X',64)
    cyl('TIDAL_RearBarrelCollar',(.205,0,0),.0154,.024,'edge','X',64)
    cyl('TIDAL_VisibleGasTube',(.381,0,.026),.0038,.372,'metal','X',48)
    for x in (.224,.564):
        profile('TIDAL_ExteriorGasSupport',[(x-.012,-.010),(x+.012,-.010),
          (x+.012,.030),(x+.008,.038),(x-.008,.038),(x-.012,.030)],.025,'metal',.0010)
    cyl('TIDAL_FrontSteppedCollar',(.765,0,0),.0105,.011,'edge','X',64)
    box('TIDAL_UpperRailBase',(.359,0,.0472),(.402,.024,.009),'metal',.00055)
    for i in range(30):
        x=.166+i*.0133
        profile('TIDAL_UpperRailTooth_%02d'%i,[(x-.0045,.0485),(x-.0045,.052),
          (x-.0032,.0545),(x+.0032,.0545),(x+.0045,.052),(x+.0045,.0485)],.032,'metal',.0004)
    box('TIDAL_LowerRailBase',(.376,0,-.043),(.274,.026,.005),'metal',.00045)
    for i in range(21):
        box('TIDAL_LowerRailTooth_%02d'%i,(.247+i*.013,0,-.0465),(.008,.032,.0035),'metal',.00035)
    for key,loc in TIDAL_LOCAL.items():
        position=tuple(MOUNTS['Barrel'][j]+loc[j] for j in range(3))
        socket(tidal['collection'],tidal['root'],'M7_BarrelTidal_Mount_'+key,position,key)

    # Retire the incorrect K1 geometry only in the new file.
    k1=bpy.data.collections.get('SM_M7_Foregrip_K1')
    if k1:
        for obj in list(bpy.data.objects):
            if obj.instance_collection==k1:bpy.data.objects.remove(obj,do_unlink=True)
        for obj in list(k1.objects):bpy.data.objects.remove(obj,do_unlink=True)
        bpy.data.collections.remove(k1,do_unlink=True)
    from resonance_grip import build_resonance_grip
    api={key:getattr(g,key) for key in ('box','cyl','tube','mesh','profile','text')}
    api['begin_asset']=begin_asset;api['mats']=mats
    result=build_resonance_grip(api)
    grip=registry['SM_M7_Foregrip_ResonanceGen1']
    grip['row']['display_name']='共振一代人体工程握把'
    grip['row']['visual_reference']='Reference/ResonanceGen1_Icon.png + Game_ResonanceGen1_K416.jpg'
    new_objects.extend(o for o in grip['collection'].objects if o.type=='MESH')
    registry['SM_M7_Sights_Iron']['row']['compatible_barrels']=['SM_M7_Barrel_Default']

    # Only new source meshes need UV creation; moved standard pieces keep theirs.
    bpy.ops.object.select_all(action='DESELECT')
    unwrapped=[o for o in new_objects if o.type=='MESH' and not o.data.uv_layers]
    for o in unwrapped:o.select_set(True)
    if unwrapped:
        bpy.context.view_layer.objects.active=unwrapped[0]
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.012,scale_to_bounds=True)
        bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT')
    for image in bpy.data.images:
        if image.name.startswith('M7R_') and image.packed_file:
            image.filepath_raw=str(ROOT/'Textures'/Path(image.filepath_raw).name)
    from refine_v2 import refine_geometry
    from refine_magazines import refine_magazines
    refine_geometry()
    refine_magazines()
    from sculpt_shell_volume import sculpt_shells
    sculpt_shells()
    defaults=['SM_M7_Core','SM_M7_Barrel_Default','SM_M7_Stock_Default','SM_M7_Mag_20',
       'SM_M7_Grip_Default','SM_M7_Muzzle_Standard','SM_M7_Sights_Iron']
    equipped=['SM_M7_Core','SM_M7_Barrel_Tidal','SM_M7_Stock_Default','SM_M7_Mag_Extended',
       'SM_M7_Grip_D2','SM_M7_Optic_LPVO','SM_M7_Foregrip_ResonanceGen1','SM_M7_Muzzle_Suppressor']
    short=[('SM_M7_Barrel_Default' if name=='SM_M7_Barrel_Tidal' else name) for name in equipped]
    presets=[{'name':'SM_M7_Default_Assembled','parts':defaults,'scene':'M7_Default_Assembly'},
       {'name':'SM_M7_Equipped_Tidal','parts':equipped,'scene':'M7_Equipped_Tidal'},
       {'name':'SM_M7_Equipped_Standard','parts':short,'scene':'M7_Equipped_Standard'}]
    # Existing studio presentation is reused, while the exporter resolves the
    # exact scene instances and dependent sockets for all three presets.
    old_equipped=bpy.data.scenes.get('M7_Suppressed_Assembly')
    if old_equipped:old_equipped.name='M7_Equipped_Tidal'
    for preset in presets:
        scene=bpy.data.scenes.get(preset['scene'])
        if scene:scene['asset_library']='M7_Modular_Replica_V2'

    # Asset catalog: two barrel combos in the top row and fourteen compact
    # attachments below, centered on their bounds rather than on mount pivots.
    catalog=bpy.data.scenes['M7_Attachment_Catalog']
    for o in list(catalog.objects):
        if o.type not in ('CAMERA','LIGHT'):bpy.data.objects.remove(o,do_unlink=True)
    dg=bpy.context.evaluated_depsgraph_get()
    names=['SM_M7_Barrel_Default','SM_M7_Barrel_Tidal']+[name for name in registry
      if name not in ('SM_M7_Core','SM_M7_Barrel_Default','SM_M7_Barrel_Tidal')]
    def label(name,body,position):
        curve=bpy.data.curves.new('Label_'+name,'FONT');curve.body=body
        curve.size=.016;curve.align_x='CENTER';curve.materials.append(mats['label'])
        obj=bpy.data.objects.new(curve.name,curve);catalog.collection.objects.link(obj)
        obj.location=position;obj.rotation_euler=(math.pi/2,0,math.pi)
    for i,name in enumerate(names):
        rec=registry[name];mount=Vector(rec['row']['mount_position_m'])
        points=[o.matrix_world @ Vector(c) - mount for o in rec['collection'].all_objects
          if o.type in ('MESH','FONT') for c in o.evaluated_get(dg).bound_box]
        centre=Vector([(min(p[j] for p in points)+max(p[j] for p in points))/2 for j in range(3)])
        if i<2:
            centre_target=Vector((-.47+i*.94,0,.65));text_z=.535
        else:
            j=i-2;centre_target=Vector(((j%4-1.5)*.45,0,.30-(j//4)*.31))
            text_z=centre_target.z-.135
        instance=bpy.data.objects.new(name+'_Catalog_Instance',None);catalog.collection.objects.link(instance)
        g.enum_set(instance,'instance_type','COLLECTION');instance.instance_collection=rec['collection']
        instance.location=centre_target-centre
        label(name,name.removeprefix('SM_M7_').replace('_',' / '),(centre_target.x,.08,text_z))
    catalog.camera.location=(0,2.6,.10)
    catalog.camera.rotation_euler=(Vector((0,0,.05))-catalog.camera.location).to_track_quat('-Z','Y').to_euler()
    catalog.camera.data.ortho_scale=2.12
    catalog.render.resolution_x,catalog.render.resolution_y=2400,1850
    rows=[]
    for name,rec in registry.items():
        row=rec['row'];row['default']=name in defaults
        row['editable_objects']=len(rec['collection'].objects);rows.append(row)
    manifest={'library':'M7_Modular_Replica_V2','source_file':'M7_Modular_Replica_V2.blend',
      'expected_asset_count':17,'axis':'+X muzzle, +Z up, metres',
      'assets':rows,'mounts':{k:list(v) for k,v in MOUNTS.items()},'default_parts':defaults,
      'equipped_parts':equipped,'presets':presets,
      'scenes':{'edit':'M7_Library_Edit','default':'M7_Default_Assembly','catalog':'M7_Attachment_Catalog'},
      'rigged':False,'reference':'Reference/ReferenceManifest.json',
      'appearance':'Black realistic palette; reference-built Resonance Gen1 and Tidal external geometry',
      'barrel_replacement':'Select one Barrel combo; its Foregrip, Muzzle and FrontIron sockets move dependent attachments.',
      'verification_limits':'External reference reconstruction, not an original game mesh. Opposite-side detail and exact game scale remain inferred.',
      'unreal_import_status':'not imported or tested in Unreal Editor'}
    (ROOT/'AssetManifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    for layer in edit.view_layers[0].layer_collection.children:
        rec=registry.get(layer.name)
        if rec:layer.exclude=rec['row']['name'] not in defaults
    bpy.context.window.scene=bpy.data.scenes['M7_Equipped_Tidal']
    print(json.dumps({'built_assets':len(rows),'new_assets':['SM_M7_Barrel_Default','SM_M7_Barrel_Tidal','SM_M7_Foregrip_ResonanceGen1'],
       'grip_result':str(result)[:600],'source_file':str(ROOT/'M7_Modular_Replica_V2.blend')},ensure_ascii=False))

if __name__=='__main__':main()
