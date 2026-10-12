"""True open guard walls, sculpted receiver channels and clearer depth."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Source'))
import replica_api as g

def refine_geometry():
    edit=bpy.data.scenes['M7_Library_Edit'];previous=bpy.context.window.scene
    bpy.context.window.scene=edit
    layers=list(edit.view_layers[0].layer_collection.children)
    old_excludes=[(l,l.exclude) for l in layers]
    for l in layers:l.exclude=False
    edit.view_layers[0].update()
    mats={key:bpy.data.materials[name] for key,name in {
      'metal':'M7R_01_AnodizedBlack','polymer':'M7R_02_PolymerBlack','rubber':'M7R_03_RubberBlack',
      'edge':'M7R_04_DarkHardware','steel':'M7R_05_BurnishedSteel','dark':'M7R_06_RecessBlack','label':'M7R_07_EngravingGray'}.items()}
    matte=bpy.data.materials.get('M7R_10_MatteInnerPolymer')
    if matte is None:
        matte=bpy.data.materials.new('M7R_10_MatteInnerPolymer');matte.use_nodes=True
        shader=next(n for n in matte.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
        shader.inputs['Base Color'].default_value=(.006,.007,.008,1)
        shader.inputs['Metallic'].default_value=0
        shader.inputs['Roughness'].default_value=.86
        shader.inputs['Specular IOR Level'].default_value=.14
    mats['matte']=matte
    added=[]
    def configure(name):g.configure(bpy.data.collections[name],bpy.data.objects[name+'_Root'],mats)
    def remove(name):
        obj=bpy.data.objects.get(name)
        if obj:bpy.data.objects.remove(obj,do_unlink=True)
    def profile(name,points,width,mat='metal',bevel=.0005):
        obj=g.profile(name,points,width,mat,bevel);added.append(obj);return obj
    def box(*args,**kwargs):
        obj=g.box(*args,**kwargs);added.append(obj);return obj
    def cyl(*args,**kwargs):
        obj=g.cyl(*args,**kwargs);added.append(obj);return obj
    def cut(target,cutters):
        # Each cutter is a simple closed solid; overlapping cutters are
        # applied separately so every requested aperture is genuinely open.
        for cutter in cutters:
            g.apply_mods(cutter)
            if cutter in added:added.remove(cutter)
            bpy.context.view_layer.objects.active=target
            mod=target.modifiers.new('Real through opening','BOOLEAN')
            g.enum_set(mod,'operation','DIFFERENCE');g.enum_set(mod,'solver','EXACT');mod.object=cutter
            if hasattr(mod,'use_self'):mod.use_self=True
            bpy.ops.object.modifier_apply(modifier=mod.name)
            bpy.data.objects.remove(cutter,do_unlink=True)
    def capsule(x0,x1,z0,z1):
        radius=(z1-z0)/2;middle=(z0+z1)/2
        points=[]
        for centre,start in ((x1-radius,-math.pi/2),(x0+radius,math.pi/2)):
            for i in range(13):
                t=start+i*math.pi/12
                points.append((centre+radius*math.cos(t),middle+radius*math.sin(t)))
        return points
    def p(px,py):return ((1250-px)*.0005,(380-py)*.0005)
    def pix(points):return [p(*q) for q in points]
    def pixelcut(points):return profile('TEMP_DefaultAperture',pix(points),.14,'dark',.00025)
    def cap_px(x0,x1,y0,y1):
        # Convert a true rounded slot to traced side-view coordinates.
        xa,za=p(x1,y1);xb,zb=p(x0,y0)
        return capsule(xa,xb,za,zb)
    report={}
    try:
        configure('SM_M7_Barrel_Default')
        remove('CORE_DefaultHandguardOutline')
        standard=profile('CORE_DefaultHandguardOutline',pix([(176,287),(917,289),(927,308),
          (936,329),(935,440),(914,463),(895,470),(819,469),(786,449),(186,449),
          (176,420),(173,341)]),.055,'metal',0)
        cutters=[box('TEMP_DefaultHollow',(.350,0,0),(.420,.049,.062),'dark',.002)]
        for points in (
          [(212,319),(265,319),(259,350),(207,350)],
          [(280,318),(343,318),(360,347),(274,347)],
          [(374,330),(402,330),(412,346),(383,346)],
          [(426,330),(454,330),(466,346),(435,346)],
          [(477,330),(504,330),(515,346),(486,346)]):cutters.append(pixelcut(points))
        for x0,x1 in ((261,391),(468,599),(677,807)):
            cutters.append(profile('TEMP_DefaultRoundSlot',cap_px(x0,x1,372,395),.14,'dark',.00025))
        for px in (219,429,636):
            x,z=p(px,383);cutters.append(cyl('TEMP_DefaultRoundVent',(x,0,z),.0047,.14,'dark','Y',40))
        for start in (257,301,354,399,445,490):
            cutters.append(pixelcut([(start+16,424),(start+39,424),(start+22,440),(start,440)]))
        for offset in (0,32,64):
            cutters.append(pixelcut([(782+offset,416),(800+offset,416),(833+offset,460),(815+offset,460)]))
        for start in (606,650,694,738,782):
            cutters.append(pixelcut([(start,305),(start+32,305),(start+37,313),(start+4,313)]))
        for x0,x1 in ((800,849),(860,907)):
            cutters.append(profile('TEMP_DefaultRearSlot',cap_px(x0,x1,336,349),.14,'dark',.00025))
        cut(standard,cutters);g.bevel_obj(standard,.00065,4)

        configure('SM_M7_Barrel_Tidal')
        remove('TIDAL_HandguardShell')
        tidal=profile('TIDAL_HandguardShell',[(.156,.038),(.567,.038),(.586,.033),(.602,.011),
          (.604,.001),(.568,-.036),(.546,-.042),(.230,-.042),(.207,-.035),(.156,-.034)],.055,'metal',0)
        cutters=[box('TEMP_TidalHollow',(.378,0,-.001),(.418,.049,.063),'dark',.0015)]
        for i in range(11):
            x=.287+i*.026
            cutters.append(profile('TEMP_TidalUpperVent',[(x-.010,.016),(x+.008,.016),
              (x+.012,.029),(x-.006,.029)],.14,'dark',.00035))
        cutters.append(cyl('TEMP_TidalUpperPort',(.263,0,.023),.0042,.14,'dark','Y',40))
        for x in (.267,.358,.449):
            cutters.append(profile('TEMP_TidalLowerSlot',capsule(x-.039,x+.039,-.029,-.018),.14,'dark',.00035))
        for i in range(3):
            x=.552-i*.023
            cutters.append(profile('TEMP_TidalFrontVent',[(x+.022,0),(x+.012,0),(x-.023,-.032),(x-.012,-.032)],.14,'dark',.00035))
        for x in (.506,.529,.552):
            cutters.append(cyl('TEMP_TidalKeyCircle',(x,0,.001),.0030,.14,'dark','Y',40))
            cutters.append(box('TEMP_TidalKeyStem',(x+.004,0,.001),(.008,.14,.0038),'dark',.00025))
        cut(tidal,cutters);g.bevel_obj(tidal,.00065,4)
        remove('TIDAL_BarrelMain')
        barrel=cyl('TIDAL_BarrelMain',(.472,0,0),.0092,.636,'metal','X',80)
        # Cosmetic external rods remain visible through the real windows.
        # Their presence does not cap the shell apertures.
        configure('SM_M7_Core')
        for name in ('CORE_ChargingSlotBezel','CORE_ChargingSlotInterior'):
            remove(name)
        opening=cap_px(1012,1189,338,355)
        for name in ('CORE_UpperForgedContour','CORE_UpperShoulder_1'):
            target=bpy.data.objects[name]
            if not target.get('v2_deep_channel'):
                g.apply_mods(target)
                cutter=profile('TEMP_ReceiverDeepChannel',opening,.035,'dark',.00035)
                cutter.location.y=.029
                cut(target,[cutter]);g.bevel_obj(target,.00085,4)
                target['v2_deep_channel']=True
        for name in ('RFN_ReceiverChannelRim','RFN_ReceiverChannelBack','RFN_ReceiverSidePocket'):
            remove(name)
        rim=profile('RFN_ReceiverChannelRim',cap_px(999,1201,332,362),.0026,'edge',0)
        rim.location.y=.0310
        cutter=profile('TEMP_ReceiverRimOpening',opening,.10,'dark',.00045)
        cutter.location.y=.0310
        cut(rim,[cutter]);g.bevel_obj(rim,.00045,4)
        back=profile('RFN_ReceiverChannelBack',opening,.0006,'matte',.00025)
        back.location.y=.0118
        # Opposite side retains the observed/inferred port position but uses
        # an actual pocket with its black back wall inside the receiver.
        remove('CORE_OppositeEjectionPort')
        opposite=cap_px(1060,1260,334,373)
        upper=bpy.data.objects['CORE_UpperForgedContour']
        if not upper.get('v2_opposite_pocket'):
            g.apply_mods(upper)
            cutter=profile('TEMP_OppositePocket',opposite,.035,'dark',.00035)
            cutter.location.y=-.029
            cut(upper,[cutter]);g.bevel_obj(upper,.00085,4)
            upper['v2_opposite_pocket']=True
        back=profile('RFN_ReceiverSidePocket',opposite,.0006,'matte',.00025)
        back.location.y=-.0118
        # Existing forged ribs receive a more visible cross-section, keeping
        # their traced side silhouette and all attachment positions unchanged.
        for name in ('CORE_LongGuide_1','CORE_LongGuide_-1'):
            obj=bpy.data.objects.get(name)
            if obj and not obj.get('v2_raised_relief'):
                sign=1 if obj.location.y>0 else -1
                obj.location.y=sign*.0332
                obj['v2_raised_relief']=True
        for obj in bpy.data.collections['SM_M7_Core'].objects:
            if obj.type=='MESH' and 'Selector' in obj.name:
                for mod in obj.modifiers:
                    if mod.type=='BEVEL':mod.segments=4
        # Keep the new foregrip's curved window dark and smooth, matching its
        # polymer surface rather than an excessively bright inner reflection.
        frame=bpy.data.objects['GEO_M7_ResonanceGen1_CurvedGripAndOpenRearFrame']
        if matte.name not in frame.data.materials:frame.data.materials.append(matte)
        material_index=list(frame.data.materials).index(matte)
        for polygon in frame.data.polygons:
            if polygon.index%4==1:
                polygon.material_index=material_index;polygon.use_smooth=True
        for mod in list(frame.modifiers):
            if mod.type=='WEIGHTED_NORMAL':frame.modifiers.remove(mod)
        # UV only the new source surfaces. Existing source UVs remain intact.
        bpy.ops.object.select_all(action='DESELECT')
        for obj in added:obj.select_set(True)
        bpy.context.view_layer.objects.active=added[0]
        bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.012,scale_to_bounds=True)
        bpy.ops.object.mode_set(mode='OBJECT');bpy.ops.object.select_all(action='DESELECT')
        edit.view_layers[0].update()
        dg=bpy.context.evaluated_depsgraph_get()
        report['through_openings']={}
        for name,points in [('CORE_DefaultHandguardOutline',[p(303,333),p(326,383),p(318,432)]),
          ('TIDAL_HandguardShell',[(.358,-.0235),(.449,-.0235),(.495,.024),(.529,.001)])]:
            obj=bpy.data.objects[name].evaluated_get(dg)
            rows=[]
            for x,z in points:
                hit,loc,normal,face=obj.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
                rows.append({'x':x,'z':z,'ray_passes_through':not hit})
            report['through_openings'][name]=rows
            assert all(row['ray_passes_through'] for row in rows),'A guard aperture remains capped: '+name
        bounds=[barrel.matrix_world @ Vector(c) for c in barrel.bound_box]
        report['barrel_front_x']=max(p.x for p in bounds)
        report['crown_overlap_m']=report['barrel_front_x']-.7855
        assert report['crown_overlap_m']>0,'Tidal crown must meet the barrel continuously.'
        report['new_objects']=[obj.name for obj in added]
    finally:
        for l,value in old_excludes:l.exclude=value
        bpy.context.window.scene=previous
    return report

def main():
    report=refine_geometry()
    path=ROOT/'AssetManifest.json';manifest=json.loads(path.read_text(encoding='utf-8'))
    for row in manifest['assets']:row['editable_objects']=len(bpy.data.collections[row['collection']].objects)
    manifest['refinements']='True thin-wall guard apertures, deep 3D receiver side channels, raised cast ribs, matte inner grip surfaces and reshaped magazine surfaces.'
    path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    (ROOT/'GeometryReview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))

if __name__=='__main__':main()
