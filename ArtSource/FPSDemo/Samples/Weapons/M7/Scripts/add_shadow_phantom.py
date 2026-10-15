"""Add reference-driven Shadow Rail stock and Phantom grip visual assets.

One current source library, unchanged receiver and frozen Tidal assembly.
Geometry modules and material configuration are the program-stage authority.
"""
import bpy,json,math,sys,hashlib
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from rebuild_tidal_reference import Builder,stable_triangulation
from build_materials import create_material,CONFIG
from freeze_tidal_attachment import guard_source
from export_library import resolve_preset

DEFINITIONS=[
 ('SM_M7_Stock_ShadowRail','Stock','Stock',(-.179,0,-.0025),'build_shadow_stock','Shadow / Rail / Stock'),
 ('SM_M7_Grip_Phantom','RearGrip','RearGrip',(-.0625,0,-.065),'build_phantom_grip','Phantom / Rear Grip')]


def verify_openings(builder,review):
    bpy.context.view_layer.update();graph=bpy.context.evaluated_depsgraph_get();graph.update()
    evaluated={o.name:o.evaluated_get(graph) for o in builder.objects}
    checks=review.get('through_apertures',review.get('holes',[]))
    for check in checks:
        points=check.get('probe_points_m') or [check.get('center_m',check.get('center'))]
        names=check.get('objects') or list(evaluated)
        blockers=[]
        for index,point in enumerate(points):
            origin=Vector(point);origin.y=.12
            for name in names:
                obj=evaluated[name];inverse=obj.matrix_world.inverted()
                hit,*_=obj.ray_cast(inverse@origin,inverse.to_3x3()@Vector((0,-1,0)),distance=.24)
                if hit:blockers.append({'object':name,'probe':index})
        check['verified_through_open']=not blockers;check['ray_scope_objects']=names;check['blockers']=blockers
        assert not blockers,('Attachment aperture blocked',check.get('name'),blockers)
    return checks


def object_contract(collection):
    """An exact read-only raw mesh/transform contract for retained assets."""
    import numpy as np
    digest=hashlib.sha256()
    for obj in sorted(collection.all_objects,key=lambda o:o.name):
        digest.update(json.dumps([obj.name,obj.type,[list(row) for row in obj.matrix_world]],sort_keys=True).encode())
        if obj.type=='MESH':
            mesh=obj.data
            for data,key,count,dtype in [(mesh.vertices,'co',3,'<f4'),(mesh.loops,'vertex_index',1,'<i4'),
                                        (mesh.polygons,'material_index',1,'<i4')]:
                buf=np.empty(len(data)*count,dtype=dtype);data.foreach_get(key,buf);digest.update(buf.tobytes())
            for uv in mesh.uv_layers:
                buf=np.empty(len(uv.data)*2,dtype='<f4');uv.data.foreach_get('uv',buf);digest.update(buf.tobytes())
            digest.update(json.dumps([m.name if m else None for m in mesh.materials]).encode())
    return digest.hexdigest()


def rebuild_catalog(manifest,edit):
    catalog=bpy.data.scenes['M7_Attachment_Catalog']
    for obj in list(catalog.objects):
        if obj.type not in ('CAMERA','LIGHT'):bpy.data.objects.remove(obj,do_unlink=True)
    bpy.context.window.scene=edit;edit.view_layers[0].update()
    graph=bpy.context.evaluated_depsgraph_get();graph.update()
    rows={r['name']:r for r in manifest['assets']}
    names=['SM_M7_Barrel_Default','SM_M7_Barrel_Tidal']+[r['name'] for r in manifest['assets']
      if r['name'] not in ('SM_M7_Core','SM_M7_Barrel_Default','SM_M7_Barrel_Tidal')]
    for i,name in enumerate(names):
        row=rows[name];col=bpy.data.collections[row['collection']];mount=Vector(row['mount_position_m'])
        pts=[o.matrix_world@Vector(v)-mount for o in col.all_objects if o.type in ('MESH','FONT')
             for v in o.evaluated_get(graph).bound_box]
        centre=Vector([(min(p[k] for p in pts)+max(p[k] for p in pts))/2 for k in range(3)])
        if i<2:target=Vector((-.47+i*.94,0,.65));text_z=.535
        else:
            j=i-2;target=Vector(((j%4-1.5)*.45,0,.30-(j//4)*.31));text_z=target.z-.135
        obj=bpy.data.objects.new(name+'_Catalog_Instance',None);catalog.collection.objects.link(obj)
        obj.instance_type='COLLECTION';obj.instance_collection=col;obj.location=target-centre
        curve=bpy.data.curves.new('M7CatalogLabel_'+name,'FONT');curve.body=row.get('display_name',name.removeprefix('SM_M7_').replace('_',' / '))
        curve.size=.016;curve.align_x='CENTER';curve.materials.append(bpy.data.materials['M7R_07_EngravingGray'])
        label=bpy.data.objects.new(curve.name,curve);catalog.collection.objects.link(label)
        label.location=(target.x,.08,text_z);label.rotation_euler=(math.pi/2,0,math.pi)


def replace_equipped(manifest):
    rows={r['name']:r for r in manifest['assets']}
    replacement={'SM_M7_Stock_Default':'SM_M7_Stock_ShadowRail','SM_M7_Grip_D2':'SM_M7_Grip_Phantom'}
    for preset in manifest['presets']:
        if preset['scene']=='M7_Default_Assembly':continue
        preset['parts']=[replacement.get(n,n) for n in preset['parts']]
        preset.update(resolve_preset(manifest,rows,preset))
        scene=bpy.data.scenes[preset['scene']]
        for obj in scene.objects:
            if not obj.instance_collection:continue
            old=obj.instance_collection.name
            if old in replacement:
                name=replacement[old];obj.instance_collection=bpy.data.collections[name]
                obj.name=name+'_'+scene.name+'_Instance';obj['source_asset']=name
                obj.location=preset['part_mounts_m'][name]
        scene['assembly_parts']=json.dumps(preset['parts']);scene['barrel_asset']=preset['barrel_asset']
    tidal=next(p for p in manifest['presets'] if p['scene']=='M7_Equipped_Tidal')
    manifest['equipped_parts']=list(tidal['parts'])


def main():
    path=ROOT/'AssetManifest.json';manifest=json.loads(path.read_text(encoding='utf-8'))
    material_path=ROOT/'MaterialReport.json';report=json.loads(material_path.read_text(encoding='utf-8'))
    previous=bpy.context.window.scene;edit=bpy.data.scenes['M7_Library_Edit'];bpy.context.window.scene=edit
    excluded=[(l,l.exclude) for l in edit.view_layers[0].layer_collection.children]
    for l,_ in excluded:l.exclude=False
    edit.view_layers[0].update();guard_source(ROOT,manifest,edit)
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    requested=set(args)
    definitions=[d for d in DEFINITIONS if not requested or d[0] in requested]
    assert definitions,'No requested attachment selected'
    new_names={d[0] for d in definitions}
    retained={r['name']:object_contract(bpy.data.collections[r['collection']])
              for r in manifest['assets'] if r['name'] not in new_names}
    prior_review=ROOT/'ShadowPhantomReview.json'
    reviews=json.loads(prior_review.read_text(encoding='utf-8')).get('assets',{}) if prior_review.is_file() else {}
    try:
        for name,category,key,mount,module_name,title in definitions:
            row=next((r for r in manifest['assets'] if r['name']==name),None)
            collection=bpy.data.collections.get(name)
            if collection and not row:raise RuntimeError('Refusing to replace an unrelated collection: '+name)
            if not collection:
                collection=bpy.data.collections.new(name);edit.collection.children.link(collection)
            for obj in list(collection.objects):
                data=obj.data;bpy.data.objects.remove(obj,do_unlink=True)
                if isinstance(data,bpy.types.Mesh) and not data.users:bpy.data.meshes.remove(data)
            root=bpy.data.objects.new(name+'_Root',None);collection.objects.link(root)
            root.empty_display_type='PLAIN_AXES';root.empty_display_size=.012
            root['source_asset']=name;root['mount_key']=key;root['mount_position_m']=mount
            collection.instance_offset=mount;collection.asset_mark()
            collection.asset_data.author='FPSDemo';collection.asset_data.description=title+' visual attachment reconstructed from user reference'
            tags={t.name for t in collection.asset_data.tags}
            for tag in ('M7','Sample','Attachment',category):
                if tag not in tags:collection.asset_data.tags.new(tag)
            materials={role:create_material('M7M_'+name.removeprefix('SM_M7_')+'_'+role,material_role,[])
                for role,material_role in [('coat','coated_receiver'),('hardware','dark_steel_hardware'),
                  ('polymer','polymer_stock' if key=='Stock' else 'polymer_grip'),('rubber','rubber_black'),('dark','recess_black')]}
            builder=Builder(collection,root,materials)
            module=__import__(module_name);review=module.build(builder)
            stable_triangulation(builder.objects)
            edit.view_layers[0].update()
            verify_openings(builder,review)
            if row is None:
                row={'name':name,'category':category,'mount_key':key,'mount_position_m':list(mount),
                     'default':False,'collection':name};manifest['assets'].append(row)
            for field in ('texture_sets','baked_uv_layer','atlas_resolution'):row.pop(field,None)
            row['editable_objects']=len(collection.objects);row['display_name']=title
            row['preserved_constant_materials']=[];row['geometry_review']='ShadowPhantomReview.json'
            report['atlases']=[v for v in report.get('atlases',[]) if v.get('asset')!=name]
            report['assignments']=[v for v in report.get('assignments',[]) if v.get('asset')!=name]
            report['assignments'] += [{'asset':name,'object':o.name,'material':o.data.materials[0].name,
              'role':o.data.materials[0]['m7_role']} for o in builder.objects]
            reviews[name]={'mount_position_m':list(mount),'editable_objects':len(collection.objects),
                          'geometry_objects':len(builder.objects),'construction':review}
        manifest['expected_asset_count']=len(manifest['assets'])
        assert manifest['expected_asset_count']==19
        replace_equipped(manifest);rebuild_catalog(manifest,edit)
        for name,digest in retained.items():
            assert object_contract(bpy.data.collections[name])==digest,'Retained asset altered: '+name
        guard_source(ROOT,manifest,edit)
        manifest['shadow_phantom_review']='ShadowPhantomReview.json'
        manifest['material_stage']='ShadowRail and Phantom rebuilt to latest side references; two fresh atlases and exports pending'
        report['maps_baked']=False;report['geometry_unchanged']=False
        report['status']='Two side-profile attachment revisions; only their atlas bake is pending'
        review={'success':True,'assets':reviews,'retained_geometry_and_uv_sha256':retained,
                'frozen_tidal_unchanged':True,'default_parts_unchanged':True,
                'replaced_presets':['M7_Equipped_Tidal','M7_Equipped_Standard'],
                'reference_priority':'Latest user side views define silhouettes; earlier three-quarter views supplement depth',
                'previous_attachment_visual_version':'rejected by user',
                'visual_acceptance':'pending user review','unreal_tested':False}
        (ROOT/'ShadowPhantomReview.json').write_text(json.dumps(review,ensure_ascii=False,indent=2),encoding='utf-8')
        path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        material_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        text=bpy.data.texts.get('M7_AssetManifest.json')
        if text:text.clear();text.from_string(json.dumps(manifest,ensure_ascii=False,indent=2))
    finally:
        for l,value in excluded:l.exclude=value
        for name in new_names:
            layer=edit.view_layers[0].layer_collection.children.get(name)
            if layer:layer.exclude=True
        bpy.context.window.scene=previous
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/manifest['source_file']),compress=True)
    print('SHADOW_PHANTOM_ADDED',json.dumps({'assets':len(manifest['assets']),
          'new_geometry_objects':{n:r['geometry_objects'] for n,r in reviews.items()},
          'retained_assets_unchanged':len(retained)},ensure_ascii=False),flush=True)


if __name__=='__main__':main()
