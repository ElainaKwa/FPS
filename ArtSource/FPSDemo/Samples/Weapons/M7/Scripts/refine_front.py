"""Small local edge fillets at the Tidal handguard's front shoulder."""
import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
def main():
    previous=bpy.context.window.scene;scene=bpy.data.scenes['M7_Library_Edit'];bpy.context.window.scene=scene
    layers=list(scene.view_layers[0].layer_collection.children);excluded=[(l,l.exclude) for l in layers]
    for layer in layers:layer.exclude=False
    scene.view_layers[0].update();obj=bpy.data.objects['TIDAL_HandguardShell']
    name='M7 front shoulder fine radius'
    try:
        if obj.modifiers.get(name):return
        bm=bmesh.new();bm.from_mesh(obj.data);bm.edges.ensure_lookup_table()
        chosen=[]
        for edge in bm.edges:
            if len(edge.link_faces)!=2 or edge.calc_length()<.00025:continue
            if min(v.co.x for v in edge.verts)<.570 or min(v.co.z for v in edge.verts)<.0005:continue
            if edge.calc_face_angle(0)>math.radians(20):chosen.append(edge.index)
        bm.free();assert chosen,'No front shoulder transitions selected.'
        weights=obj.data.attributes.get('bevel_weight_edge') or obj.data.attributes.new('bevel_weight_edge','FLOAT','EDGE')
        selected=set(chosen)
        for i,value in enumerate(weights.data):value.value=1 if i in selected else 0
        modifier=obj.modifiers.new(name,'BEVEL')
        valid=[i.identifier for i in modifier.bl_rna.properties['limit_method'].enum_items]
        assert 'WEIGHT' in valid;modifier.limit_method='WEIGHT'
        modifier.width=.0020;modifier.segments=8;modifier.use_clamp_overlap=True;modifier.harden_normals=True
        scene.view_layers[0].update();dg=bpy.context.evaluated_depsgraph_get();evaluated=obj.evaluated_get(dg)
        checks=[]
        for x,z in ((.358,-.0235),(.449,-.0235),(.495,.024),(.529,.001)):
            hit,*_=evaluated.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
            checks.append({'x':x,'z':z,'open':not hit})
        assert all(c['open'] for c in checks)
        report={'modified_shell':obj.name,'selected_edges':len(chosen),'maximum_radius_m':.002,'segments':8,
                'localized_region':'Front positive-Z shoulder only; X >= .570 m','through_openings':checks,'success':True}
        (ROOT/'FrontRefinement.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        manifest_path=ROOT/'AssetManifest.json';manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
        manifest['front_refinement']=report
        manifest_path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        material_path=ROOT/'MaterialReport.json';materials=json.loads(material_path.read_text(encoding='utf-8'))
        materials['geometry_unchanged']=False
        materials['geometry_changes']=['Small front shoulder fillets on Tidal handguard, requested during material study.']
        material_path.write_text(json.dumps(materials,ensure_ascii=False,indent=2),encoding='utf-8')
        print(json.dumps(report,ensure_ascii=False))
    finally:
        for layer,value in excluded:layer.exclude=value
        bpy.context.window.scene=previous
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source/M7_Material_Study.blend'),compress=True)
if __name__=='__main__':main()
