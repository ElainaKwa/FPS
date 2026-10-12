"""Densify curved skins and refine all visible bevels without closing holes."""
import bpy,bmesh,json,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
shape={ '__name__':'m7_surface_functions', '__file__':str(ROOT/'Source/sculpt_shell_volume.py') }
exec(compile((ROOT/'Source/sculpt_shell_volume.py').read_text(encoding='utf-8'),shape['__file__'],'exec'),shape)

def main():
    previous=bpy.context.window.scene
    edit=bpy.data.scenes['M7_Library_Edit'];bpy.context.window.scene=edit
    layers=list(edit.view_layers[0].layer_collection.children)
    excluded=[(l,l.exclude) for l in layers]
    for layer in layers:layer.exclude=False
    edit.view_layers[0].update()
    rows=[]
    specs=[
      ('TIDAL_HandguardShell',lambda x,z:shape['guard_shift'](x,z,True),.020,.156,.604,-.042,.038,.0275,.0245),
      ('CORE_DefaultHandguardOutline',lambda x,z:shape['guard_shift'](x,z,False),.020,.157,.539,-.045,.047,.0275,.0245),
      ('CORE_UpperForgedContour',shape['upper_shift'],.019,-.165,.168,-.033,.039,.0255,None),
      ('CORE_LowerForgedContour',shape['lower_shift'],.016,-.163,.158,-.120,-.011,.0215,None),
    ]
    try:
        for name,shift,threshold,x0,x1,z0,z1,outer,inner in specs:
            obj=bpy.data.objects[name]
            if obj.get('high_detail_version')==1:continue
            before=len(obj.data.vertices)
            bm=bmesh.new();bm.from_mesh(obj.data)
            # Recover the original side skin before inserting new samples.
            # This avoids merely subdividing the old coarse approximation.
            for v in bm.verts:
                x,y,z=v.co
                base=abs(y)-shift(x,z)
                if base>threshold and abs(y)>.014:
                    v.co.y=math.copysign(base,y)
            for axis,start,end,step in ((0,x0,x1,.004),(2,z0,z1,.0018)):
                count=math.ceil((end-start)/step)
                for i in range(1,count):
                    point=Vector((0,0,0));point[axis]=start+(end-start)*i/count
                    normal=Vector((0,0,0));normal[axis]=1
                    bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                        plane_co=point,plane_no=normal,dist=.0000001,clear_inner=False,clear_outer=False)
            for v in bm.verts:
                x,y,z=v.co
                if abs(y)>threshold:
                    v.co.y=y+math.copysign(1,y)*shift(x,z)
            # Small triangles sample the continuous cross-section; large
            # nonplanar n-gons no longer create irregular specular patches.
            bmesh.ops.triangulate(bm,faces=list(bm.faces))
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
            for f in bm.faces:f.smooth=abs(f.normal.y)>.35
            for e in bm.edges:
                e.smooth=len(e.link_faces)==2 and e.calc_face_angle(0)<math.radians(42)
            bm.to_mesh(obj.data);bm.free();obj.data.update()
            if obj.data.has_custom_normals:
                obj.data.normals_split_custom_set([(0,0,0)]*len(obj.data.loops))
            # Continuous surface derivatives give the dense curved skin a
            # consistent reflection across Boolean and triangulation edges.
            normals=[tuple(n.vector) for n in obj.data.corner_normals]
            epsilon=.00005
            for polygon in obj.data.polygons:
                if abs(polygon.normal.y)<.60:continue
                for index in polygon.loop_indices:
                    vertex=obj.data.vertices[obj.data.loops[index].vertex_index]
                    x,y,z=vertex.co
                    if abs(y)<.014:continue
                    sign=1 if y>=0 else -1
                    dx=(shift(x+epsilon,z)-shift(x-epsilon,z))/(2*epsilon)
                    dz=(shift(x,z+epsilon)-shift(x,z-epsilon))/(2*epsilon)
                    normal=Vector((-dx,sign,-dz)).normalized()
                    if polygon.normal.y*sign<0:normal=-normal
                    normals[index]=tuple(normal)
            obj.data.normals_split_custom_set(normals)
            obj['high_detail_version']=1
            rows.append({'shell':name,'vertices_before':before,'vertices_after':len(obj.data.vertices),
                         'faces_after':len(obj.data.polygons)})
        assets={r['collection'] for r in json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))['assets']}
        seen=set();bevels=0
        for name in assets:
            for obj in bpy.data.collections[name].all_objects:
                if obj in seen:continue
                seen.add(obj)
                if obj.type=='FONT':
                    obj.data.bevel_resolution=max(obj.data.bevel_resolution,3)
                    obj.data.resolution_u=max(obj.data.resolution_u,16)
                for mod in obj.modifiers:
                    if mod.type=='BEVEL':
                        mod.segments=max(mod.segments,8 if mod.width>=.001 else 6)
                        bevels+=1
        edit.view_layers[0].update();dg=bpy.context.evaluated_depsgraph_get()
        aperture_checks=[]
        for name,points in [('CORE_DefaultHandguardOutline',[(.4735,.0235),(.462,-.0015),(.466,-.026)]),
          ('TIDAL_HandguardShell',[(.358,-.0235),(.449,-.0235),(.495,.024),(.529,.001)])]:
            obj=bpy.data.objects[name].evaluated_get(dg)
            for x,z in points:
                hit,*_=obj.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
                aperture_checks.append({'shell':name,'x':x,'z':z,'open':not hit})
        assert all(row['open'] for row in aperture_checks)
        report={'success':True,'refined_shells':rows,'refined_bevel_modifiers':bevels,
                'through_openings':aperture_checks,
                'method':'Fresh samples of curved skins, triangulated small surface cells, smooth normal transitions, 6–8 segment edge bevels.'}
        (ROOT/'HighDetailReview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        path=ROOT/'AssetManifest.json';manifest=json.loads(path.read_text(encoding='utf-8'))
        manifest['high_detail_review']=report
        for row in manifest['assets']:row['editable_objects']=len(bpy.data.collections[row['collection']].objects)
        path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
        print(json.dumps(report,ensure_ascii=False))
    finally:
        for layer,value in excluded:layer.exclude=value
        bpy.context.window.scene=previous

if __name__=='__main__':main()
