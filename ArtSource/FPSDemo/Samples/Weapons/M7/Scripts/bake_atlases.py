"""Bake each editable asset into its own unique UV and portable PBR atlas."""
import bpy,json,math,hashlib,numpy as np,sys
from pathlib import Path
from mathutils import Matrix
ROOT=Path(__file__).resolve().parents[1]
UV='M7_MaterialUV'
MANIFEST=ROOT/'AssetManifest.json'

def enum_set(owner,key,value):
    valid=[x.identifier for x in owner.bl_rna.properties[key].enum_items]
    assert value in valid,(key,value,valid)
    setattr(owner,key,value)

def ensure_gltf_output_group():
    """Match Blender 5.2's official group, preserving existing AO links.

    The importer reuses this global group by name and reads Thickness by
    index, as well as iridescence sockets by name.  An Occlusion-only group
    therefore breaks otherwise valid GLB imports.
    """
    from io_scene_gltf2.blender.com.material_helpers import get_gltf_node_name,create_settings_group
    name=get_gltf_node_name()
    group=bpy.data.node_groups.get(name)
    if group is None:
        group=create_settings_group(name)
    expected=(('Occlusion',0.0),('Thickness',0.0),('Dispersion',0.0),
              ('Iridescence Factor',0.0),('Iridescence Thickness Minimum',100.0))
    for index,(socket_name,default) in enumerate(expected):
        matches=[item for item in group.interface.items_tree
                 if item.item_type=='SOCKET' and item.in_out=='INPUT' and item.name==socket_name]
        if len(matches)>1:
            raise RuntimeError('Duplicate glTF Material Output input: '+socket_name)
        if matches:
            socket=matches[0]
            if socket.socket_type!='NodeSocketFloat':
                raise RuntimeError('Unexpected glTF Material Output socket type: '+socket_name)
        else:
            socket=group.interface.new_socket(name=socket_name,in_out='INPUT',socket_type='NodeSocketFloat')
        socket.default_value=default
        # Moving the existing socket keeps its identifier and every material
        # AO connection intact while restoring the official input order.
        group.interface.move(socket,index)
    if not any(node.type=='GROUP_INPUT' for node in group.nodes):
        node=group.nodes.new('NodeGroupInput');node.location=(-200,0)
    if not any(node.type=='GROUP_OUTPUT' for node in group.nodes):
        group.nodes.new('NodeGroupOutput')
    return group

def gpu_setup(scene):
    prefs=bpy.context.preferences.addons['cycles'].preferences
    for kind in ('OPTIX','CUDA'):
        try:prefs.compute_device_type=kind;prefs.get_devices()
        except TypeError:continue
        usable=[d for d in prefs.devices if d.type==kind]
        if usable:
            for d in prefs.devices:d.use=d in usable
            scene.cycles.device='GPU';return kind
    scene.cycles.device='CPU';return 'CPU'

def select_only(objects):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]

def unwrap(objects,size):
    for obj in objects:
        if obj.data.users>1:obj.data=obj.data.copy()
        while obj.data.uv_layers:obj.data.uv_layers.remove(obj.data.uv_layers[0])
        layer=obj.data.uv_layers.new(name=UV);layer.active_render=True
    select_only(objects)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(70),margin_method='FRACTION',
                            island_margin=6/size,area_weight=.25,correct_aspect=True,scale_to_bounds=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    for obj in objects:
        assert obj.data.uv_layers.active.name==UV
        obj.data.uv_layers.active.active_render=True

def new_image(name,size,role):
    image=bpy.data.images.new(name,width=size,height=size,alpha=False,float_buffer=False)
    image.colorspace_settings.name='sRGB' if role=='BaseColor' else 'Non-Color'
    image.generated_color=(.5,.5,1,1) if role=='Normal' else ((1,1,1,1) if role=='AO' else (0,0,0,1))
    return image

def save_image(image,path):
    image.filepath_raw=str(path);image.file_format='PNG';image.save();image.pack()
    return path.relative_to(ROOT).as_posix()

def original_channel(material,channel):
    nodes=material.node_tree.nodes
    tagged=material.get('m7_bake_channels')
    if tagged:
        name=json.loads(tagged)[channel]
        return nodes[name].outputs[0]
    shader=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    socket=shader.inputs[{'BaseColor':'Base Color','Roughness':'Roughness','Metallic':'Metallic'}[channel]]
    return socket.links[0].from_socket if socket.is_linked else socket.default_value

def connect_emission(material,channel):
    nodes=material.node_tree.nodes;links=material.node_tree.links
    emit=nodes.get('M7_BAKE_Emission') or nodes.new('ShaderNodeEmission');emit.name='M7_BAKE_Emission'
    value=original_channel(material,channel)
    for link in list(emit.inputs['Color'].links):links.remove(link)
    if hasattr(value,'is_output'):links.new(value,emit.inputs['Color'])
    else:emit.inputs['Color'].default_value=(*([value]*3 if isinstance(value,(float,int)) else value[:3]),1)
    emit.inputs['Strength'].default_value=1
    output=next(n for n in nodes if n.type=='OUTPUT_MATERIAL')
    links.new(emit.outputs[0],output.inputs['Surface'])

def restore_surface(material):
    nodes=material.node_tree.nodes;links=material.node_tree.links
    shader=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    if nodes.get('M7_FineFinishNormal'):links.new(nodes['M7_FineFinishNormal'].outputs['Normal'],shader.inputs['Normal'])
    output=next(n for n in nodes if n.type=='OUTPUT_MATERIAL');links.new(shader.outputs[0],output.inputs['Surface'])

def activate_image(materials,image):
    for material in materials:
        nodes=material.node_tree.nodes
        target=nodes.get('M7_BAKE_Target') or nodes.new('ShaderNodeTexImage');target.name='M7_BAKE_Target'
        target.image=image;nodes.active=target;target.select=True

def bake(scene,mesh,materials,image,kind):
    activate_image(materials,image);select_only([mesh]);bpy.context.view_layer.update()
    result=bpy.ops.object.bake(type=kind,uv_layer=UV,use_clear=False,margin=8,
                              normal_space='TANGENT',normal_r='POS_X',normal_g='POS_Y',normal_b='POS_Z')
    assert 'FINISHED' in result

def source_material_atlas(material,images):
    nodes=material.node_tree.nodes;links=material.node_tree.links
    for node in list(nodes):
        if node.name.startswith('M7_Baked'):nodes.remove(node)
    shader=next(n for n in nodes if n.type=='BSDF_PRINCIPLED')
    uv=nodes.new('ShaderNodeUVMap');uv.name='M7_BakedUV';uv.uv_map=UV
    textures={}
    for key,image in images.items():
        node=nodes.new('ShaderNodeTexImage');node.name='M7_Baked_'+key;node.image=image
        links.new(uv.outputs['UV'],node.inputs['Vector']);textures[key]=node
    links.new(textures['BaseColor'].outputs['Color'],shader.inputs['Base Color'])
    separate=nodes.new('ShaderNodeSeparateColor');separate.name='M7_Baked_ORM_Channels'
    links.new(textures['ORM'].outputs['Color'],separate.inputs[0])
    links.new(separate.outputs[1],shader.inputs['Roughness']);links.new(separate.outputs[2],shader.inputs['Metallic'])
    normal=nodes.new('ShaderNodeNormalMap');normal.name='M7_Baked_Normal';normal.uv_map=UV
    normal.inputs['Strength'].default_value=1;links.new(textures['Normal'].outputs['Color'],normal.inputs['Color'])
    links.new(normal.outputs['Normal'],shader.inputs['Normal'])
    group=ensure_gltf_output_group()
    ao=nodes.new('ShaderNodeGroup');ao.name='M7_Baked_AO_Export';ao.node_tree=group
    links.new(separate.outputs[0],ao.inputs['Occlusion'])
    material['m7_baked_uv']=UV

def main():
    assert bpy.app.background,'Bake in a separate background process.'
    # Repair existing baked materials even when all atlases are skipped.
    ensure_gltf_output_group()
    bpy.context.preferences.filepaths.save_version=0
    manifest=json.loads(MANIFEST.read_text(encoding='utf-8'))
    requested=set(sys.argv[sys.argv.index('--')+1:]) if '--' in sys.argv else set()
    edit=bpy.data.scenes['M7_Library_Edit'];bpy.context.window.scene=edit
    layers=list(edit.view_layers[0].layer_collection.children)
    excluded=[(l,l.exclude) for l in layers]
    for layer in layers:layer.exclude=False
    edit.view_layers[0].update()
    report=json.loads((ROOT/'MaterialReport.json').read_text(encoding='utf-8'))
    report.setdefault('atlases',[])
    for row in manifest['assets']:
        if requested and row['name'] not in requested:continue
        if row.get('texture_sets') and all((ROOT/path).is_file() for paths in row['texture_sets'].values() for path in paths.values()):
            print('BAKE_SKIP',row['name'],flush=True);continue
        print('BAKE_BEGIN',row['name'],flush=True)
        collection=bpy.data.collections[row['collection']]
        objects=[o for o in collection.all_objects if o.type=='MESH']
        size=4096 if row['name'] in ('SM_M7_Core','SM_M7_Barrel_Tidal') else (2048 if row['category'] in ('Barrel','Stock','Magazine','Optic','RearGrip','Foregrip','Grip') else 1024)
        bpy.context.window.scene=edit;edit.view_layers[0].update();unwrap(objects,size)
        source_materials={m for o in objects for m in o.data.materials if m}
        dg=bpy.context.evaluated_depsgraph_get();dg.update()
        temp=bpy.data.scenes.new('M7_MATERIAL_BAKE_TEMP');temp.world=edit.world;temp.render.engine='CYCLES'
        temp.cycles.samples=16;device=gpu_setup(temp);enum_set(temp.render.bake,'normal_space','TANGENT')
        temp.render.bake.use_selected_to_active=False
        material_copies={material:material.copy() for material in source_materials}
        copies=[];created_meshes=[];working=[]
        for obj in objects:
            data=bpy.data.meshes.new_from_object(obj.evaluated_get(dg),preserve_all_data_layers=True,depsgraph=dg)
            created_meshes.append(data)
            copy=bpy.data.objects.new('M7_BAKE_'+obj.name,data);temp.collection.objects.link(copy);copy.matrix_world=obj.matrix_world.copy()
            for i,material in enumerate(list(data.materials)):
                if material:data.materials[i]=material_copies[material]
            copies.append(copy)
        bpy.context.window.scene=temp;select_only(copies)
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);bpy.ops.object.join()
        mesh=bpy.context.object;materials=[m for m in mesh.data.materials if m]
        mesh.data.uv_layers.active=mesh.data.uv_layers[UV];mesh.data.uv_layers[UV].active_render=True
        image_paths={};final_images={};orm=np.zeros((size*size,4),np.float32);orm[:,3]=1
        pixels=np.empty(size*size*4,np.float32)
        for channel,orm_index in [('BaseColor',None),('Roughness',1),('Metallic',2)]:
            image=new_image(row['name']+'_'+channel,size,channel)
            for material in materials:connect_emission(material,channel)
            print('BAKE_CHANNEL',row['name'],channel,device,flush=True);bake(temp,mesh,materials,image,'EMIT')
            if orm_index is None:
                image_paths[channel]=save_image(image,ROOT/'Textures/Baked'/(row['name']+'_'+channel+'.png'));final_images[channel]=image
            else:
                image.pixels.foreach_get(pixels);orm[:,orm_index]=pixels.reshape(-1,4)[:,0]
                bpy.data.images.remove(image)
        for material in materials:restore_surface(material)
        for channel,kind in [('AO','AO'),('Normal','NORMAL')]:
            image=new_image(row['name']+'_'+channel,size,channel)
            print('BAKE_CHANNEL',row['name'],channel,device,flush=True);bake(temp,mesh,materials,image,kind)
            if channel=='AO':
                image.pixels.foreach_get(pixels);orm[:,0]=pixels.reshape(-1,4)[:,0];bpy.data.images.remove(image)
            else:
                image_paths[channel]=save_image(image,ROOT/'Textures/Baked'/(row['name']+'_'+channel+'.png'));final_images[channel]=image
        assert np.isfinite(orm).all();np.clip(orm,0,1,out=orm)
        image=new_image(row['name']+'_ORM',size,'ORM');image.pixels.foreach_set(orm.ravel())
        image_paths['ORM']=save_image(image,ROOT/'Textures/Baked'/(row['name']+'_ORM.png'));final_images['ORM']=image
        del pixels,orm
        mapped=[]
        for material in source_materials:
            if not material.name.startswith('M7M_'):continue
            source_material_atlas(material,final_images);mapped.append(material.name)
        row['texture_sets']={name:dict(image_paths) for name in sorted(mapped)}
        row['baked_uv_layer']=UV;row['atlas_resolution']=[size,size]
        report['atlases'].append({'asset':row['name'],'resolution':[size,size],'files':image_paths,
                                 'mapped_materials':sorted(mapped),'uv':UV,'normal_convention':'OpenGL tangent +Y',
                                 'geometry_unchanged':True})
        bpy.context.window.scene=edit
        for obj in list(temp.objects):bpy.data.objects.remove(obj,do_unlink=True)
        bpy.data.scenes.remove(temp)
        for material in material_copies.values():
            if material.users==0:bpy.data.materials.remove(material)
        for data in created_meshes:
            if data.users==0:bpy.data.meshes.remove(data)
        MANIFEST.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        report['maps_baked']=len(report['atlases'])==len(manifest['assets'])
        report['status']='baked_pbr_maps_complete' if report['maps_baked'] else 'atlas_baking_in_progress'
        (ROOT/'MaterialReport.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source/M7_Material_Study.blend'),compress=True)
        print('BAKE_DONE',row['name'],flush=True)
    for layer,value in excluded:layer.exclude=value
    bpy.context.window.scene=bpy.data.scenes['M7_Equipped_Tidal']
    complete=all(r.get('texture_sets') for r in manifest['assets'])
    manifest['material_stage']='Portable baked BaseColor, Normal and ORM atlases; editable layer graphs retained' if complete else 'Atlas baking in progress'
    manifest['normal_map_convention']='OpenGL tangent +Y; DirectX import workflows must invert the green channel.'
    MANIFEST.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source/M7_Material_Study.blend'),compress=True)
    print(json.dumps({'baked_assets':sum(bool(r.get('texture_sets')) for r in manifest['assets']),'source':bpy.data.filepath}),flush=True)
if __name__=='__main__':main()
