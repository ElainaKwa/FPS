"""Shape the actual exterior shell surfaces, retaining the reference silhouette."""
import bpy,bmesh,math,json,numpy as np
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]

def smooth(a,b,value):
    t=max(0,min(1,(value-a)/(b-a)))
    return t*t*(3-2*t)

def panel(x,z,x0,x1,z0,z1,edge=.005):
    return smooth(x0,x0+edge,x)*smooth(-x1,-x1+edge,-x)*smooth(z0,z0+edge,z)*smooth(-z1,-z1+edge,-z)

def guard_shift(x,z,tidal):
    dome=.0048*math.exp(-((z-.001)/.023)**4)
    chamfer=-.0043*smooth(.022,.039,z)-.0038*smooth(.026,.043,-z)
    longitudinal=-.0020*(1-smooth(.158,.190,x))
    if tidal:
        recess=-.0026*panel(x,z,.245,.475,-.010,.011,.004)
        front=-.0015*smooth(.558,.604,x)
    else:
        recess=-.0018*panel(x,z,.249,.486,-.019,-.010,.0028)
        front=-.0014*smooth(.507,.538,x)
    return dome+chamfer+longitudinal+recess+front

def upper_shift(x,z):
    dome=.0038*math.exp(-((z-.016)/.018)**2)
    shoulder=-.0032*smooth(.029,.038,z)
    recess=-.0013*panel(x,z,-.120,.122,-.013,.005,.006)
    return dome+shoulder+recess

def lower_shift(x,z):
    dome=.0028*math.exp(-((z+.054)/.035)**2)
    taper=-.0022*smooth(.082,.119,-z)
    recess=-.0010*panel(x,z,-.123,.111,-.081,-.027,.009)
    return dome+taper+recess

def sculpt_shells():
    if not bpy.context.window:raise RuntimeError('Requires a Blender context.')
    edit=bpy.data.scenes['M7_Library_Edit'];previous=bpy.context.window.scene
    bpy.context.window.scene=edit
    layers=list(edit.view_layers[0].layer_collection.children)
    excluded=[(l,l.exclude) for l in layers]
    for l in layers:l.exclude=False
    edit.view_layers[0].update()
    version=1
    changes=[]
    specifications=[
      ('CORE_DefaultHandguardOutline',lambda x,z:guard_shift(x,z,False),.020,
       [.185,.225,.25,.30,.35,.40,.45,.485,.510],[-.039,-.030,-.024,-.019,-.014,-.010,-.005,.002,.008,.015,.022,.029,.035]),
      ('TIDAL_HandguardShell',lambda x,z:guard_shift(x,z,True),.020,
       [.175,.190,.225,.245,.275,.325,.375,.425,.475,.50,.54,.57,.595],[-.038,-.030,-.023,-.017,-.010,-.006,.002,.008,.011,.017,.022,.028,.034]),
      ('CORE_UpperForgedContour',upper_shift,.019,
       [-.15,-.12,-.08,-.04,0,.04,.08,.12,.15],[-.026,-.016,-.008,0,.007,.014,.020,.027,.032,.037]),
      ('CORE_LowerForgedContour',lower_shift,.016,
       [-.15,-.12,-.08,-.04,0,.04,.08,.12,.15],[-.115,-.095,-.082,-.07,-.054,-.04,-.026,-.017]),
    ]
    try:
        for name,shift,threshold,x_cuts,z_cuts in specifications:
            obj=bpy.data.objects[name]
            if obj.get('sculpted_shell_version',0)>=version:continue
            # Apply the existing edge finish, then insert surface bands.
            # UV interpolation survives these mesh cuts; no textured plane is
            # used to imitate the resulting bulge, recess or chamfer.
            bpy.context.view_layer.objects.active=obj
            for modifier in list(obj.modifiers):bpy.ops.object.modifier_apply(modifier=modifier.name)
            bm=bmesh.new();bm.from_mesh(obj.data)
            for axis,values in ((0,x_cuts),(2,z_cuts)):
                for value in values:
                    point=Vector((0,0,0));point[axis]=value
                    normal=Vector((0,0,0));normal[axis]=1
                    bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),
                        plane_co=point,plane_no=normal,dist=.0000005,clear_inner=False,clear_outer=False)
            for vertex in bm.verts:
                x,y,z=vertex.co
                if abs(y)>threshold:
                    vertex.co.y=y+math.copysign(shift(x,z),y) if shift(x,z)>=0 else y-math.copysign(-shift(x,z),y)
            bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
            bm.to_mesh(obj.data);bm.free();obj.data.update()
            for polygon in obj.data.polygons:
                polygon.use_smooth=abs(polygon.normal.y)>.48
            if obj.data.has_custom_normals:
                obj.data.normals_split_custom_set([(0,0,0)]*len(obj.data.loops))
            obj['sculpted_shell_version']=version
            changes.append(name)
        # The screws, sockets and ribs follow their curved parent skin.
        for collection_name,tidal in [('SM_M7_Barrel_Default',False),('SM_M7_Barrel_Tidal',True)]:
            collection=bpy.data.collections[collection_name]
            for obj in collection.objects:
                if obj.type!='MESH' or obj.name in ('CORE_DefaultHandguardOutline','TIDAL_HandguardShell'):continue
                if obj.get('follow_sculpted_shell',False):continue
                is_external=('Handguard' in obj.name or any(key in obj.name for key in ('TIDAL_QD','TIDAL_ShellFastener','TIDAL_FastenerSocket','TIDAL_UpperSide','TIDAL_LowerReinforcement')))
                if not is_external or abs(obj.location.y)<.01:continue
                points=[obj.matrix_world @ Vector(c) for c in obj.bound_box]
                x=sum(p.x for p in points)/8;z=sum(p.z for p in points)/8
                obj.location.y+=math.copysign(guard_shift(x,z,tidal),obj.location.y) if guard_shift(x,z,tidal)>=0 else -math.copysign(-guard_shift(x,z,tidal),obj.location.y)
                obj['follow_sculpted_shell']=True
        for name in ('RFN_ReceiverChannelRim','CORE_ChargingHandle','CORE_UpperShoulder_1','CORE_UpperShoulder_-1'):
            obj=bpy.data.objects.get(name)
            if not obj or obj.get('follow_sculpted_shell',False):continue
            obj.location.y+=math.copysign(.0034,obj.location.y)
            obj['follow_sculpted_shell']=True
        # Fine surface grain stays subtle enough that the sculpted forms read.
        for material in bpy.data.materials:
            if not material.name.startswith('M7R_') or not material.node_tree:continue
            for node in material.node_tree.nodes:
                if node.type=='NORMAL_MAP':node.inputs['Strength'].default_value=.035
        for image in bpy.data.images:
            if not image.name.startswith('M7R_') or not image.name.endswith('_ORM') or image.get('subtle_grain_v2'):continue
            array=np.empty(len(image.pixels),np.float32);image.pixels.foreach_get(array)
            array=array.reshape(-1,4)
            roughness=float(array[:,1].mean())
            array[:,1]=roughness+(array[:,1]-roughness)*.12
            image.pixels.foreach_set(array.ravel())
            image.filepath_raw=str(ROOT/'Textures'/(image.name+'.png'))
            image.save();image.pack();image['subtle_grain_v2']=True
        edit.view_layers[0].update()
        dg=bpy.context.evaluated_depsgraph_get()
        aperture_checks=[]
        for name,points in [('CORE_DefaultHandguardOutline',[(.4735,.0235),(.462,-.0015),(.466,-.026)]),
            ('TIDAL_HandguardShell',[(.358,-.0235),(.449,-.0235),(.495,.024),(.529,.001)])]:
            obj=bpy.data.objects[name].evaluated_get(dg)
            for x,z in points:
                hit,_,_,_=obj.ray_cast(Vector((x,1,z)),Vector((0,-1,0)))
                aperture_checks.append({'shell':name,'x':x,'z':z,'open':not hit})
        assert all(row['open'] for row in aperture_checks),'Sculpting capped an existing aperture.'
        report={'sculpted_shells':changes,'through_openings':aperture_checks,'success':True,
            'geometry_change':'Curved side skin, integrated recessed fields, sloped upper/lower shoulders and stepped receiver cross-sections; X/Z silhouette retained.'}
        return report
    finally:
        for layer,value in excluded:layer.exclude=value
        bpy.context.window.scene=previous

if __name__=='__main__':
    report=sculpt_shells()
    (ROOT/'SurfaceVolumeReview.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report))
