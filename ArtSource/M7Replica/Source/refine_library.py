"""Apply visible-reference corrections to the modular exterior meshes."""
import bpy, json, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Source'))
import replica_api as g

def refine_geometry():
    scene = bpy.data.scenes['M7_Library_Edit']
    previous_scene = bpy.context.window.scene
    bpy.context.window.scene = scene
    layers = list(scene.view_layers[0].layer_collection.children)
    excluded = [(layer,layer.exclude) for layer in layers]
    for layer in layers: layer.exclude = False
    mats = {key:bpy.data.materials[name] for key,name in {
        'polymer':'M7R_02_PolymerBlack', 'metal':'M7R_01_AnodizedBlack',
        'edge':'M7R_04_DarkHardware', 'dark':'M7R_06_RecessBlack',
    }.items()}
    added=[]
    def configure(name):
        c=bpy.data.collections[name]
        g.configure(c,bpy.data.objects[name+'_Root'],mats)
    def p(px,py):return ((1250-px)*.0005,(380-py)*.0005)
    def profile(name,points,width,mat,bevel=.0005):
        old=bpy.data.objects.get(name)
        if old is not None:bpy.data.objects.remove(old,do_unlink=True)
        obj=g.profile(name,[p(*pt) for pt in points],width,mat,bevel)
        added.append(obj)
        return obj
    def subtract(target,cutter):
        g.apply_mods(cutter)
        bpy.context.view_layer.objects.active=target
        mod=target.modifiers.new('Reference opening','BOOLEAN')
        g.enum_set(mod,'operation','DIFFERENCE')
        g.enum_set(mod,'solver','EXACT')
        mod.object=cutter
        bpy.ops.object.modifier_apply(modifier=mod.name)
        bpy.data.objects.remove(cutter,do_unlink=True)
    try:
        # Visible black insertion segment closes the gap above both magazines.
        for name in ('SM_M7_Mag_20','SM_M7_Mag_Extended'):
            configure(name)
            profile('RFN_'+name+'_InsertionNeck',
                [(999,533),(1201,590),(1201,652),(998,599)],.041,'dark')
        configure('SM_M7_Stock_Default')
        profile('RFN_Stock_CheekLowerBeam',
            [(1678,410),(1946,410),(1942,433),(1680,432)],.040,'polymer',.00065)
        web=profile('RFN_Stock_UpperSolidWeb',
            [(1681,429),(1944,431),(1937,474),(1915,483),
             (1690,479),(1680,460)],.033,'polymer',0)
        cutter=profile('TEMP_StockDiagonalSlot',
            [(1816,438),(1898,460),(1898,470),(1890,477),
             (1810,453),(1808,447),(1810,441)],.10,'dark',.0015)
        added.remove(cutter)
        subtract(web,cutter)
        g.bevel_obj(web,.00065,3)
        # The reference has a hollow sling socket in the lower frame, rather
        # than a solid bolt and a second loop floating in the upper window.
        for side in ('L','R'):
            for suffix in ('_Rim','_Head','_Socket'):
                obj=bpy.data.objects.get('Stock_Default_LengthLock_'+side+suffix)
                if obj is not None:bpy.data.objects.remove(obj,do_unlink=True)
            obj=bpy.data.objects.get('Stock_Default_SlingLoop_'+side)
            if obj is not None:bpy.data.objects.remove(obj,do_unlink=True)
        boss=profile('RFN_Stock_LowerSlingBoss',
            [(1848,487),(1878,487),(1886,503),(1876,520),
             (1845,520),(1837,506)],.032,'polymer',0)
        x,z=p(1860,505)
        cutter=g.cyl('TEMP_StockSlingHole',(x,0,z),.0055,.10,'dark','Y',48)
        subtract(boss,cutter)
        g.bevel_obj(boss,.00065,3)
        rail=bpy.data.objects.get('Stock_Default_BottomTelescopingRail')
        if rail is not None and not rail.get('reference_sling_cut',False):
            g.apply_mods(rail)
            cutter=g.cyl('TEMP_SlingRailClearance',(x,0,z),.0055,.10,'dark','Y',48)
            subtract(rail,cutter)
            g.bevel_obj(rail,.00025,3)
            rail['reference_sling_cut']=True
        # New pieces receive real UVs; all existing UVs remain intact.
        bpy.ops.object.select_all(action='DESELECT')
        for obj in added:obj.select_set(True)
        bpy.context.view_layer.objects.active=added[0]
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=1.151917,island_margin=.012,scale_to_bounds=True)
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.select_all(action='DESELECT')
        scene.view_layers[0].update()
    finally:
        for layer,value in excluded:layer.exclude=value
        bpy.context.window.scene=previous_scene
    return [obj.name for obj in added]

def main():
    added=refine_geometry()
    path=ROOT/'AssetManifest.json'
    manifest=json.loads(path.read_text(encoding='utf-8'))
    for row in manifest['assets']:
        row['editable_objects']=len(bpy.data.collections[row['collection']].objects)
    manifest['refinement']='Closed magazine insertion seams; restored solid upper stock web, diagonal opening and hollow lower sling socket from the official side reference.'
    path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    # Save only this new asset library, never the user's other scenes.
    ids={bpy.data.scenes[name] for name in manifest['saved_scenes']}
    ids.update(bpy.data.collections[row['collection']] for row in manifest['assets'])
    bpy.data.libraries.write(str(ROOT/'M7_Modular_Replica.blend'),ids,fake_user=True,compress=True)
    print(json.dumps({'refined_objects':added,'source_file':str(ROOT/'M7_Modular_Replica.blend')}))

if __name__=='__main__':main()
