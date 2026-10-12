"""Create editable, restrained black PBR layers on the M7 geometry input."""
import bpy,json,re,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CONFIG=json.loads((ROOT/'Scripts/material_settings.json').read_text(encoding='utf-8'))
MANIFEST=ROOT/'AssetManifest.json'
PRESERVE={'M7R_06_RecessBlack','M7R_07_EngravingGray',*CONFIG['global_settings']['preserve_materials']}

def enum_set(owner,key,value):
    available=[x.identifier for x in owner.bl_rna.properties[key].enum_items]
    assert value in available,(key,value,available)
    setattr(owner,key,value)

class Graph:
    def __init__(self,material):
        self.nodes=material.node_tree.nodes;self.links=material.node_tree.links
    def node(self,kind,name):
        node=self.nodes.new(kind);node.name=name;node.label=name;return node
    def put(self,source,target):
        if hasattr(source,'is_output'):self.links.new(source,target)
        else:target.default_value=source
    def math(self,operation,a,b,name):
        n=self.node('ShaderNodeMath',name);enum_set(n,'operation',operation)
        self.put(a,n.inputs[0]);self.put(b,n.inputs[1]);return n.outputs[0]
    def vector(self,operation,a,b,name):
        n=self.node('ShaderNodeVectorMath',name);enum_set(n,'operation',operation)
        self.put(a,n.inputs[0]);self.put(b,n.inputs[1]);return n.outputs[0]
    def ramp(self,value,start,end,name):
        n=self.node('ShaderNodeMapRange',name)
        self.put(value,n.inputs['Value']);n.inputs['From Min'].default_value=start;n.inputs['From Max'].default_value=end
        n.inputs['To Min'].default_value=0;n.inputs['To Max'].default_value=1;n.clamp=True
        return n.outputs['Result']
    def mix(self,a,b,factor,name):
        n=self.node('ShaderNodeMixRGB',name);enum_set(n,'blend_type','MIX')
        self.put(factor,n.inputs[0]);self.put(a,n.inputs[1]);self.put(b,n.inputs[2]);return n.outputs[0]
    def noise(self,position,scale,name):
        n=self.node('ShaderNodeTexNoise',name);self.put(position,n.inputs['Vector'])
        n.inputs['Scale'].default_value=scale;n.inputs['Detail'].default_value=2;n.inputs['Roughness'].default_value=.55
        return n.outputs['Fac']
    def ellipsoid(self,position,mask):
        def one(center,name):
            d=self.vector('SUBTRACT',position,center,name+'_Offset')
            d=self.vector('MULTIPLY',d,[1/v for v in mask['radii_m']],name+'_Radius')
            length=self.node('ShaderNodeVectorMath',name+'_Distance');enum_set(length,'operation','LENGTH');self.put(d,length.inputs[0])
            inside=self.ramp(length.outputs['Value'],.55,1,name+'_Falloff')
            return self.math('SUBTRACT',1,inside,name+'_Inside')
        result=one(mask['center_m'],mask['id'])
        if mask.get('mirror_y'):
            center=list(mask['center_m']);center[1]=-center[1]
            result=self.math('MAXIMUM',result,one(center,mask['id']+'_Opposite'),mask['id']+'_BothSides')
        return self.math('MULTIPLY',result,mask['strength'],mask['id']+'_Strength')

def create_material(name,role,masks):
    material=bpy.data.materials.new(name);material.use_nodes=True;material.node_tree.nodes.clear()
    graph=Graph(material);params=CONFIG['roles'][role]
    geometry=graph.node('ShaderNodeNewGeometry','M7_SourceGeometry')
    position=geometry.outputs['Position']
    grain=graph.noise(position,params['finegrain_scale_per_m'],'M7_FineGrain')
    middle=graph.math('SUBTRACT',grain,.5,'M7_CenteredGrain')
    roughness=graph.math('ADD',params['roughness'],graph.math('MULTIPLY',middle,2*params['roughness_variation'],'M7_RoughnessVariation'),'M7_RoughnessBase')
    # Color varies only a few percent; coarse splotches never imitate depth.
    tone=graph.noise(position,120,'M7_SubtleFinishVariation')
    tone=graph.math('ADD',.985,graph.math('MULTIPLY',tone,.030,'M7_ColorVariation'),'M7_ColorMultiplier')
    color_node=graph.node('ShaderNodeMixRGB','M7_BlackFinishColor');enum_set(color_node,'blend_type','MULTIPLY')
    color_node.inputs[0].default_value=1;color_node.inputs[1].default_value=(*params['base_color'],1)
    graph.put(tone,color_node.inputs[2]);color=color_node.outputs[0]
    polish=0.0;loss=0.0
    for mask in masks:
        contact=graph.ellipsoid(position,mask)
        if mask['effect']=='finish_polish':
            roughness=graph.math('ADD',roughness,graph.math('MULTIPLY',contact,mask['roughness_delta'],mask['id']+'_RoughnessChange'),mask['id']+'_Polish')
            polish=graph.math('MAXIMUM',polish,graph.math('MULTIPLY',contact,mask['color_multiplier']-1,mask['id']+'_ColorChange'),mask['id']+'_PolishUnion')
        elif role.startswith('coated_') or role=='dark_steel_hardware':
            edge=graph.ramp(geometry.outputs['Pointiness'],.505,.565,mask['id']+'_ConvexEdge')
            vor=graph.node('ShaderNodeTexVoronoi',mask['id']+'_SparseWearCells');graph.put(position,vor.inputs['Vector'])
            vor.inputs['Scale'].default_value=CONFIG['global_settings']['coating_loss_speck_scale_per_m']
            separate=graph.node('ShaderNodeSeparateColor',mask['id']+'_CellRandom');graph.put(vor.outputs['Color'],separate.inputs[0])
            sparse=graph.ramp(separate.outputs[0],mask.get('sparse_noise_threshold',.96),1,mask['id']+'_Sparse')
            chip=graph.math('MULTIPLY',contact,graph.math('MULTIPLY',edge,sparse,mask['id']+'_EdgeCells'),mask['id']+'_Chip')
            loss=graph.math('MAXIMUM',loss,chip,mask['id']+'_ChipUnion')
    multiply=graph.node('ShaderNodeMixRGB','M7_ContactColor');enum_set(multiply,'blend_type','MULTIPLY')
    multiply.inputs[0].default_value=1;graph.put(color,multiply.inputs[1]);graph.put(graph.math('ADD',1,polish,'M7_PolishMultiplier'),multiply.inputs[2])
    exposed=CONFIG['roles']['exposed_steel'];color=graph.mix(multiply.outputs[0],(*exposed['base_color'],1),loss,'M7_OUT_BaseColor')
    metallic=graph.math('ADD',params['metallic'],graph.math('MULTIPLY',loss,exposed['metallic']-params['metallic'],'M7_ExposedMetal'),'M7_OUT_Metallic')
    roughness=graph.math('MAXIMUM',.12,roughness,'M7_RoughnessFloor')
    roughness=graph.math('MINIMUM',.94,roughness,'M7_OUT_Roughness')
    bump=graph.node('ShaderNodeBump','M7_FineFinishNormal')
    bump.inputs['Strength'].default_value=params['grain_strength'];bump.inputs['Distance'].default_value=params['grain_height_m']
    graph.put(grain,bump.inputs['Height'])
    shader=graph.node('ShaderNodeBsdfPrincipled','M7_LayeredSurface')
    graph.put(color,shader.inputs['Base Color']);graph.put(roughness,shader.inputs['Roughness']);graph.put(metallic,shader.inputs['Metallic'])
    shader.inputs['Specular IOR Level'].default_value=params['specular_ior_level']
    graph.put(bump.outputs['Normal'],shader.inputs['Normal'])
    output=graph.node('ShaderNodeOutputMaterial','M7_SurfaceOutput');graph.put(shader.outputs[0],output.inputs['Surface'])
    material['m7_role']=role;material['m7_contact_masks']=json.dumps([m['id'] for m in masks])
    material['m7_bake_channels']=json.dumps({'BaseColor':'M7_OUT_BaseColor','Roughness':'M7_OUT_Roughness','Metallic':'M7_OUT_Metallic'})
    material.diffuse_color=(*params['base_color'],1);material.asset_mark()
    return material

def main():
    manifest=json.loads(MANIFEST.read_text(encoding='utf-8'));assignments=[];cache={}
    for row in manifest['assets']:
        collection=bpy.data.collections[row['collection']]
        for obj in collection.all_objects:
            if obj.type not in ('MESH','FONT'):continue
            for slot in obj.material_slots:
                old=slot.material
                if not old or old.name in PRESERVE:continue
                role=CONFIG['source_material_roles'].get(old.name)
                if not role:continue
                for rule in CONFIG['object_role_overrides']:
                    if old.name in rule['only_source_materials'] and re.search(rule['object_regex'],obj.name):role=rule['role']
                # Collections provide reliable fallback classification when
                # object names contain older generator prefixes.
                if role=='coated_receiver':
                    if row['category']=='Barrel':role='coated_handguard'
                    elif row['category']=='Magazine':role='coated_magazine'
                    elif row['category']=='Optic':role='coated_optic'
                if role=='polymer_stock' and row['category'] in ('Foregrip','RearGrip','Grip'):role='polymer_grip'
                masks=[m for m in CONFIG['wear_masks'] if re.search(m['object_regex'],obj.name)]
                key=(row['name'],role,tuple(m['id'] for m in masks))
                if key not in cache:
                    short=row['name'].removeprefix('SM_M7_');tag=hashlib.sha1('|'.join(key[2]).encode()).hexdigest()[:5]
                    name='M7M_'+short+'_'+role+'_'+tag
                    cache[key]=create_material(name,role,masks)
                slot.material=cache[key]
                assignments.append({'asset':row['name'],'object':obj.name,'old_material':old.name,'material':slot.material.name,'role':role,'masks':list(key[2])})
        row['preserved_constant_materials']=sorted({slot.material.name for obj in collection.all_objects if obj.type in ('MESH','FONT') for slot in obj.material_slots if slot.material and slot.material.name in PRESERVE})
        row['editable_objects']=len(collection.objects)
    manifest['preserved_constant_materials']=sorted(PRESERVE)
    manifest['style_basis']=CONFIG.get('style_basis','Docs/FPS/Art/DesignDecisions.md, sections 2 and 5')
    manifest['style_interpretation']=CONFIG.get('style_interpretation','Restrained industrial black/gray with differentiated materials and local wear.')
    manifest['material_stage']='Editable procedural material layers; atlas baking pending'
    MANIFEST.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    for scene in bpy.data.scenes:
        if scene.name in manifest['saved_scenes']:scene['asset_library']=manifest['library']
    report={'status':'procedural_layers_built','geometry_unchanged':True,'material_count':len(cache),
            'assignments':assignments,'settings':'Scripts/material_settings.json','current_source':'Source/M7_Material_Study.blend',
            'baseline_sha256':manifest['baseline']['sha256'],'visual_acceptance':'pending user review',
            'unreal_tested':False,'maps_baked':False}
    report['style_basis']=manifest['style_basis'];report['style_interpretation']=manifest['style_interpretation']
    (ROOT/'MaterialReport.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    scene=bpy.data.scenes['M7_Equipped_Tidal'];bpy.context.window.scene=scene
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source/M7_Material_Study.blend'),compress=True)
    print(json.dumps({'material_count':len(cache),'assigned_slots':len(assignments),'geometry_unchanged':True,'source':bpy.data.filepath}))
if __name__=='__main__':main()
