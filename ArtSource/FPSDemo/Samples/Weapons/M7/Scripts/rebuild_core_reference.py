"""Reconstruct the visual receiver from the user's cropped side reference.

Only SM_M7_Core is edited. Fixed sockets and top rail stay aligned with the
existing attachment library. Image proportions are art observations, not
manufacturing measurements or a mechanically functioning weapon design.
"""
import bpy
import json
import math
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
ASSET='SM_M7_Core'
REAR_RAIL_START_X=-.136
REAR_RAIL_TRIMMED_TEETH={'CORE_RailTooth_00','CORE_RailTooth_01'}
REFERENCE_SHA256='b002e3b186d529b526a7e1c42ad948f9e727610deea6eafc34fc6126faccbb46'
sys.path.insert(0,str(ROOT/'Scripts'))
from rebuild_tidal_reference import Builder,rounded_rect,stable_triangulation
from build_materials import create_material,CONFIG


def p(px,py):
    return .165-(px-10)*.00067,(140-py)*.00066


def pix(points):return [p(*v) for v in points]


def round_px(x0,x1,y0,y1,r=2):
    a=p(x1,y1);b=p(x0,y0)
    return rounded_rect(a[0],b[0],a[1],b[1],r*.00066,12)


def circle(x,z,r,n=64):
    return [(x+r*math.cos(i*math.tau/n),z+r*math.sin(i*math.tau/n)) for i in range(n)]


def bezier(a,b,c,d,n=18):
    return [((1-t)**3*a[0]+3*(1-t)**2*t*b[0]+3*(1-t)*t*t*c[0]+t**3*d[0],
             (1-t)**3*a[1]+3*(1-t)**2*t*b[1]+3*(1-t)*t*t*c[1]+t**3*d[1])
            for t in [i/n for i in range(n+1)]]


class ReceiverBuilder(Builder):
    def difference(self,target,outline,y0,y1,label):
        cutter=self.prism('TEMP_CORE_'+label,outline,y0,y1,'coat')
        bpy.context.view_layer.objects.active=target
        mod=target.modifiers.new('Machined recess '+label,'BOOLEAN')
        mod.operation='DIFFERENCE';mod.object=cutter
        # Cut the raw solid before editable bevel/normal modifiers.
        while list(target.modifiers).index(mod)>0:
            bpy.ops.object.modifier_move_up(modifier=mod.name)
        solvers={v.identifier for v in mod.bl_rna.properties['solver'].enum_items}
        mod.solver='MANIFOLD' if 'MANIFOLD' in solvers else 'EXACT'
        if hasattr(mod,'use_self'):mod.use_self=False
        bpy.ops.object.modifier_apply(modifier=mod.name)
        self.boolean_count+=1
        assert len(target.data.polygons)>4,('Empty receiver after cut',label)
        self.objects.remove(cutter);data=cutter.data
        bpy.data.objects.remove(cutter,do_unlink=True)
        if not data.users:bpy.data.meshes.remove(data)

    def side(self,name,outline,side,base=.0254,depth=.0007,role='coat',bevel=.00018):
        a,b=sorted((side*base,side*(base+depth)))
        return self.prism(name,outline,a,b,role,bevel)

    def pocket(self,target,name,outline,side,base=.0245,depth=.0017):
        a,b=sorted((side*(base-depth),side*(base+.008)))
        self.difference(target,outline,a,b,name)
        self.side(name+'_InsetFloor',outline,side,base-depth+.000015,.00003,'dark',.00004)

    def screw(self,name,px,py,side,base=.026,r=.0030):
        x,z=p(px,py)
        a,b=sorted((side*base,side*(base+.0007)))
        self.ring_y(name+'_Seat',x,z,a,b,r*1.15,r*.79,'hardware')
        head=self.side(name+'_Head',circle(x,z,r*.81),side,base+.0003,.0009,'hardware',.00012)
        # The drive is actually recessed, so reflections describe its depth.
        self.difference(head,circle(x,z,r*.33,6),*sorted((side*(base+.00065),side*(base+.006))),name+'_HexDrive')
        self.side(name+'_DriveBack',circle(x,z,r*.31,6),side,base+.00066,.000015,'dark',.00002)
        return head


def build_core(b):
    # Upper body: thin horizontal receiver and small angular shoulder steps.
    upper=b.prism('CORE_UpperForgedContour',pix([(10,87),(23,78),(53,82),(375,82),
      (397,100),(456,100),(466,84),(491,86),(506,102),(515,137),(504,161),
      (21,161),(12,148)]),-.0245,.0245,'coat')
    # A narrow roof bridges the fixed rail without making a large bulbous slab.
    roof=b.prism('CORE_UpperRoofBridge',pix([(16,77),(38,72),(456,72),(478,81),
      (491,90),(401,96),(378,81),(21,88)]),-.017,.017,'coat',.00045)
    # Lower forging incorporates the magwell and the curved rear shoulder.
    lower_outline=[(18,165),(448,165),(473,170),(489,181),(482,197),(464,204),
      (442,205),(422,198),(402,201),(384,208),(370,222),(359,234),(337,238),
      (316,242),(307,238),(213,235),(210,289),(204,293),(60,253),(62,211),
      (45,195),(21,190)]
    lower=b.prism('CORE_LowerForgedContour',pix(lower_outline),-.0235,.0235,'coat')
    # Real magazine cavity: retains the existing independent magazine neck.
    b.difference(lower,[(.028,-.048),(.129,-.048),(.129,-.15),(.022,-.15)],-.0214,.0214,'MagazineInsertionCavity')
    # Actual hollow sling socket through the lower forging.
    sx,sz=p(450,185)
    b.difference(lower,circle(sx,sz,.0080),-.10,.10,'RearSlingThroughHole')
    checks=[]
    for s in (-1,1):
        tag='L' if s<0 else 'R'
        # Long restrained upper land, terminating in a distinct angled edge.
        b.side('CORE_UpperShoulder_'+str(s),pix([(38,88),(374,88),(391,106),
          (389,109),(76,109),(67,118),(55,116),(43,102)]),s,.0245,.0009,'coat',.00028)
        b.pocket(upper,'CORE_UpperLongRecess_'+tag,round_px(77,203,105,121,3.8),s,depth=.0020)
        # Continuous thin charging track extends almost to the rear seam.
        b.pocket(upper,'CORE_LongTrack_'+tag,round_px(101,474,131,153,2.5),s,depth=.00155)
        b.side('CORE_TrackLowerLip_'+tag,pix([(105,148),(475,148),(475,153),(104,154)]),s,.0246,.00065,'coat',.00015)
        b.side('CORE_TrackSlider_'+tag,round_px(111,242,133,143,1.8),s,.0231,.00065,'hardware',.00013)
        b.side('CORE_TrackSliderSeam_'+tag,round_px(248,445,142,145,.7),s,.0231,.00018,'hardware',.00007)
        for px in (81,194):b.screw('CORE_UpperSlotFastener_'+tag+str(px),px,112,s,.02555,.00345)
        # Reference front housing: stepped metal shape with three real pockets.
        block=b.side('CORE_FrontMechanicalHousing_'+tag,pix([(12,111),(28,111),
          (39,124),(104,124),(104,153),(89,158),(13,157)]),s,.0246,.0018,'hardware',0)
        for i,(x0,x1,y0,y1) in enumerate(((18,36,132,145),(63,91,133,145))):
            b.pocket(block,'CORE_FrontHousingPocket_'+tag+str(i),round_px(x0,x1,y0,y1,1.8),s,.0264,.0012)
        b.screw('CORE_FrontHousingFastener_'+tag,51,139,s,.0265,.00255)
        b.screw('CORE_TrackFrontFastener_'+tag,102,139,s,.0265,.00265)
        Builder.bevel(block,.00025)
        # Thin upper/lower separation land and genuine shallow machined band.
        b.side('CORE_LowerCastingRib_'+str(s),pix([(31,166),(431,166),(422,180),
          (234,184),(221,180),(37,179)]),s,.0235,.00065,'coat',.00027)
        b.pocket(lower,'CORE_LowerHorizontalChannel_'+tag,round_px(40,205,173,181,1.2),s,.0235,.0009)
        # Cross-layer bolt catch: vertical stem, top paddle and lower crossarm.
        b.side('CORE_CatchStem_'+tag,pix([(211,154),(224,153),(225,218),
          (240,218),(240,238),(217,238),(216,226),(209,224)]),s,.0261,.00145,'hardware',.0003)
        b.side('CORE_CatchUpperPaddle_'+tag,round_px(211,228,178,198,1.8),s,.0275,.0012,'coat',.00023)
        b.side('CORE_CatchCrossarm_'+tag,pix([(175,206),(221,205),(235,214),
          (220,222),(164,220),(165,214)]),s,.0261,.00135,'hardware',.0003)
        b.side('CORE_CatchInset_'+tag,round_px(181,218,210,218,1),s,.0276,.0002,'dark',.00008)
        for j in range(5):
            b.side('CORE_CatchGripGroove_'+tag+str(j),round_px(213,226,181+j*2.8,182+j*2.8,.35),s,.0287,.00012,'hardware',.00004)
        b.side('CORE_CatchLowerPad_'+tag,round_px(220,239,225,237,1.2),s,.0276,.0008,'coat',.00018)
        b.screw('CORE_CatchPivot_'+tag,214,216,s,.0283,.00225)
        # Magwell side relief follows the sloped skirt rather than a huge panel.
        b.pocket(lower,'CORE_MagwellInset_'+tag,pix([(67,196),(196,197),(203,204),
          (202,265),(68,232)]),s,.0235,.00055)
        # Matching coating is used on this shallow broad inset, never a black void.
        inset=b.objects[-1];inset.data.materials[0]=b.materials['coat']
        b.side('CORE_MagwellRolledLip_'+tag,pix([(60,246),(211,285),(210,292),
          (59,253)]),s,.0235,.00125,'coat',.00035)
        b.screw('CORE_FrontTakedown_'+tag,20,171,s,.0245,.00305)
        b.screw('CORE_RearTakedown_'+tag,369,191,s,.0245,.0031)
        # Smaller selector and a genuine open rear sling ring.
        cx,cz=p(330,210)
        b.side('CORE_SelectorHub_'+tag,circle(cx,cz,.0055),s,.0237,.00115,'hardware',.00022)
        b.side('CORE_SelectorLever_'+tag,pix([(330,203),(306,206),(302,212),
          (312,215),(334,214),(341,209)]),s,.025,.0017,'hardware',.00022)
        b.screw('CORE_SelectorPivot_'+tag,331,209,s,.0266,.0026)
        for j,(px,py) in enumerate(((312,228),(324,231),(338,230))):
            x,z=p(px,py)
            b.side('CORE_SelectorWitness_'+tag+str(j),circle(x,z,.00065,24),s,.0236,.00012,'hardware',.00003)
        for px,py in ((280,215),(349,214)):
            b.screw('CORE_SmallLowerPin_'+tag+str(px),px,py,s,.0237,.0015)
        a,d=sorted((s*.0234,s*.0250))
        b.ring_y('CORE_RearSlingMachinedRing_'+tag,sx,sz,a,d,.0115,.0080,'coat')
        a,d=sorted((s*.0250,s*.02555))
        b.ring_y('CORE_RearSlingInnerCollar_'+tag,sx,sz,a,d,.00865,.0080,'hardware')
        # Rear end has an independent slim seam below the stock attachment.
        b.side('CORE_RearEndSeam_'+tag,pix([(488,98),(491,100),(499,153),
          (496,162),(493,155)]),s,.0246,.00045,'hardware',.00008)
        b.side('CORE_RearLatch_'+tag,round_px(461,476,88,114,1.0),s,.0245,.0010,'hardware',.00015)
        b.screw('CORE_RearLatchPin_'+tag,466,110,s,.0255,.0016)
        checks.append({'side':s,'upper_slot_depth_m':.0020,'long_track_depth_m':.00155,
          'lower_separation_recess_m':.0009,'real_recesses':True})
    # Rear stock interface remains exactly at the original socket.
    b.box('CORE_StockMountBlock',(-.172,0,-.0025),(.015,.041,.046),'coat',.0006)
    # Slender curved trigger guard, modeled as a real frame with open center.
    outer=[(209,231),(215,232),(217,274)]
    outer+=bezier((217,274),(219,293),(254,302),(281,300))[1:]
    outer+=bezier((281,300),(302,299),(314,296),(317,281))[1:]
    outer += [(317,238),(309,238),(309,277)]
    outer+=bezier((309,277),(306,290),(296,293),(281,293))[1:]
    outer+=bezier((281,293),(257,296),(224,288),(224,273))[1:]
    outer += [(223,233)]
    b.prism('CORE_CurvedTriggerGuard',pix(outer),-.010,.010,'coat',.00038)
    trigger=bezier((270,235),(276,250),(285,266),(276,281),24)
    trigger+=bezier((276,281),(267,282),(272,273),(272,266),10)[1:]
    trigger+=bezier((272,266),(273,254),(267,247),(265,238),15)[1:]
    b.prism('CORE_CurvedTrigger',pix(trigger),-.0026,.0026,'hardware',.00025)
    # Joining shoulder supports the existing removable rear grip.
    shoulder=[(400,195),(435,201),(430,215)]
    shoulder+=bezier((430,215),(418,215),(415,232),(408,242),18)[1:]
    shoulder+=bezier((408,242),(394,252),(371,255),(351,259),18)[1:]
    shoulder+=bezier((351,259),(340,261),(327,263),(320,258),14)[1:]
    shoulder += [(315,242),(359,224)]
    b.prism('CORE_GripUpperShoulder',pix(shoulder),-.0235,.0235,'coat',.00055)
    Builder.bevel(upper,.00042);Builder.bevel(lower,.0006)
    stable_triangulation(b.objects)
    bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get()
    hit,*_=lower.evaluated_get(dg).ray_cast(Vector((sx,.10,sz)),Vector((0,-1,0)),distance=.20)
    assert not hit,'Rear sling socket must remain genuinely open'
    return checks


def main():
    manifest_path=ROOT/'AssetManifest.json'
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    row=next(r for r in manifest['assets'] if r['name']==ASSET)
    collection=bpy.data.collections[row['collection']]
    root=bpy.data.objects[ASSET+'_Root']
    previous=bpy.context.window.scene;edit=bpy.data.scenes['M7_Library_Edit']
    bpy.context.window.scene=edit
    excluded=[(l,l.exclude) for l in edit.view_layers[0].layer_collection.children]
    for l,_ in excluded:l.exclude=False
    edit.view_layers[0].update()
    sockets={o.name:list(o.location) for o in collection.objects if o.type=='EMPTY'}
    # Freeze guard compares evaluated geometry, materials, UVs and interfaces.
    from freeze_tidal_attachment import guard_source
    guard_source(ROOT,manifest,edit)
    frozen_before={'passed':True,'registry':'AttachmentRegistry.json'}
    try:
        coat_masks=[m for m in CONFIG['wear_masks'] if m['id'].startswith('receiver_')]
        mats={role:create_material('M7M_Core_Reference_'+role,material_role,coat_masks if role=='coat' else [])
              for role,material_role in [('coat','coated_receiver'),('hardware','dark_steel_hardware'),('dark','recess_black')]}
        keep=[];removed=[]
        for o in list(collection.objects):
            if o.type not in ('MESH','FONT'):continue
            if o.name in REAR_RAIL_TRIMMED_TEETH:
                removed.append(o.name);data=o.data;bpy.data.objects.remove(o,do_unlink=True)
                if not data.users:bpy.data.meshes.remove(data)
                continue
            if o.name=='CORE_ReceiverTopRailBase' or o.name.startswith('CORE_RailTooth_'):
                if o.name=='CORE_ReceiverTopRailBase':
                    inverse=o.matrix_world.inverted()
                    for vertex in o.data.vertices:
                        world=o.matrix_world@vertex.co
                        if world.x<REAR_RAIL_START_X:
                            world.x=REAR_RAIL_START_X;vertex.co=inverse@world
                    o.data.update()
                o.data.materials.clear();o.data.materials.append(mats['coat']);keep.append(o);continue
            removed.append(o.name);data=o.data;bpy.data.objects.remove(o,do_unlink=True)
            if not data.users:
                if isinstance(data,bpy.types.Mesh):bpy.data.meshes.remove(data)
                else:bpy.data.curves.remove(data)
        b=ReceiverBuilder(collection,root,mats)
        recess_checks=build_core(b);stable_triangulation(keep)
        bpy.context.view_layer.update()
        graph=bpy.context.evaluated_depsgraph_get();graph.update()
        rail=bpy.data.objects['CORE_ReceiverTopRailBase']
        bounds=[rail.matrix_world@Vector(v) for v in rail.bound_box]
        assert abs(min(v.x for v in bounds)-REAR_RAIL_START_X)<1e-6
        assert abs(max(v.x for v in bounds)-.158)<1e-6,'Forward barrel interface moved'
        roof=bpy.data.objects['CORE_UpperRoofBridge'].evaluated_get(graph)
        samples=[]
        for x in (-.1335,-.126):
            hit,point,*_=roof.ray_cast(Vector((x,0,.08)),Vector((0,0,-1)),distance=.07)
            assert hit,'Rear rail must sit above the receiver roof'
            overlap=point.z-min(v.z for v in bounds)
            assert overlap>0,'Rear rail has a gap below its support'
            samples.append({'x_m':x,'receiver_roof_z_m':point.z,'support_overlap_m':overlap})
        for name,loc in sockets.items():assert (bpy.data.objects[name].location-Vector(loc)).length<1e-8
        for key in ('texture_sets','baked_uv_layer','atlas_resolution'):row.pop(key,None)
        row['preserved_constant_materials']=[];row['editable_objects']=len(collection.objects)
        row['geometry_review']='CoreReferenceReview.json'
        report={'success':True,'asset':ASSET,'reference_image':'Reference/Core_Receiver_User.png',
          'reference_sha256':REFERENCE_SHA256,
          'scope':'Art reconstruction of upper/lower receiver only; existing removable furniture, barrel and fixed sockets retained.',
          'mapping':{'x':'.165-(px-10)*.00067','z':'(140-py)*.00066','width_inferred':True},
          'changes':['Thin upper shoulder with stepped roof','Two long real recessed channels and slim longitudinal slider',
          'Front mechanical housing with real pockets and recessed hex drives','Cross-layer catch stem and lower crossarm',
          'Sloped hollow magazine well and thin rolled lip','Thin curved open trigger guard',
          'Smaller selector and through-hole rear sling seat'],
          'boolean_operations':b.boolean_count,'machined_recess_checks':recess_checks,
          'retained_top_rail_objects':[o.name for o in keep],'fixed_sockets_unchanged':True,
          'rear_rail_trim':{'rear_start_x_m':REAR_RAIL_START_X,'original_rear_start_x_m':-.162,
            'trimmed_tooth_names':sorted(REAR_RAIL_TRIMMED_TEETH),'forward_end_x_m':.158,
            'receiver_support_samples':samples,'forward_barrel_interface_unchanged':True},
          'frozen_attachment_check':frozen_before,'removed_object_count':len(removed),
          'new_object_count':len(b.objects),'bevel_segments':8,'circle_segments':64,
          'unreal_tested':False,'visual_acceptance':'pending user review','atlas_status':'Core only requires fresh bake'}
        material_path=ROOT/'MaterialReport.json';materials=json.loads(material_path.read_text(encoding='utf-8'))
        materials['atlases']=[v for v in materials.get('atlases',[]) if v.get('asset')!=ASSET]
        materials['assignments']=[v for v in materials.get('assignments',[]) if v.get('asset')!=ASSET]
        materials['assignments'] += [{'asset':ASSET,'object':o.name,'material':o.data.materials[0].name,'role':o.data.materials[0]['m7_role']} for o in b.objects+keep]
        materials['geometry_unchanged']=False;materials['maps_baked']=False
        materials['status']='Reference-driven Core receiver revision, Core atlas/export update pending'
        manifest['core_reference_review']='CoreReferenceReview.json'
        manifest['material_stage']='Reference-driven Core receiver revision; Core atlas/export update pending'
        manifest['visual_acceptance']='pending user review'
        (ROOT/'CoreReferenceReview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        material_path.write_text(json.dumps(materials,ensure_ascii=False,indent=2),encoding='utf-8')
        manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        guard_source(ROOT,manifest,edit)
    finally:
        for l,value in excluded:l.exclude=value
        bpy.context.window.scene=previous
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/manifest['source_file']),compress=True)
    print('CORE_REFERENCE_DONE',json.dumps({'objects':row['editable_objects'],'boolean_operations':b.boolean_count,'frozen_attachment_preserved':True}),flush=True)


if __name__=='__main__':main()
