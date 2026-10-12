"""Make the isolated data file a normal .blend; embed component thumbnails."""
import bpy,json,numpy as np
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
assert bpy.app.background,'Finalize in a separate background process.'
scene=bpy.data.scenes['M7_Equipped_Tidal']
bpy.context.window.scene=scene
cam=scene.camera
from mathutils import Vector
from mathutils import Matrix
from bpy_extras.object_utils import world_to_camera_view
cam.location=(.99,2.10,.59)
cam.rotation_euler=(Vector((.25,0,-.051))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.ortho_scale=1.52
scene.render.film_transparent=False
scene.render.resolution_x,scene.render.resolution_y=2600,1300
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_perspective='CAMERA'
            area.spaces.active.shading.type='MATERIAL'
            area.spaces.active.overlay.show_overlays=False
manifest=json.loads((ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
for other in list(bpy.data.scenes):
    if other.name not in manifest['saved_scenes'] and not other.objects:
        bpy.data.scenes.remove(other)
catalog=bpy.data.scenes['M7_Attachment_Catalog']
catalog.camera.location=(0,2.6,.10)
catalog.camera.rotation_euler=(Vector((0,0,.05))-catalog.camera.location).to_track_quat('-Z','Y').to_euler()
catalog.camera.data.ortho_scale=2.12
catalog.render.resolution_x,catalog.render.resolution_y=2400,1850
edit=bpy.data.scenes['M7_Library_Edit']
layers=list(edit.view_layers[0].layer_collection.children)
excluded=[(layer,layer.exclude) for layer in layers]
for layer in layers:layer.exclude=False
edit.view_layers[0].update()
catalog.view_layers[0].update()
images={}
def pixels(filename):
    if filename not in images:
        im=bpy.data.images.load(str(ROOT/filename),check_existing=False)
        array=np.empty(im.size[0]*im.size[1]*4,np.float32)
        im.pixels.foreach_get(array)
        images[filename]=(im,array.reshape(im.size[1],im.size[0],4))
    return images[filename][1]
def thumbnail(array):
    height,width=array.shape[:2]
    scale=min(240/width,240/height)
    w,h=max(1,round(width*scale)),max(1,round(height*scale))
    xs=np.linspace(0,width-1,w).astype(int)
    ys=np.linspace(0,height-1,h).astype(int)
    canvas=np.zeros((256,256,4),np.float32)
    x,y=(256-w)//2,(256-h)//2
    canvas[y:y+h,x:x+w]=array[ys[:,None],xs[None,:]]
    return canvas
with bpy.context.temp_override(scene=edit,view_layer=edit.view_layers[0]):
    dg=bpy.context.evaluated_depsgraph_get()
    for row in manifest['assets']:
        asset=bpy.data.collections[row['collection']]
        if row['category']=='Core':
            array=pixels('Preview_Equipped_Tidal.png')
        else:
            array=pixels('Preview_Attachments.png')
            instance=next(o for o in catalog.objects if o.instance_collection==asset)
            points=[]
            for obj in asset.all_objects:
                if obj.type not in ('MESH','FONT'):continue
                evaluated=obj.evaluated_get(dg)
                transform=instance.matrix_world @ Matrix.Translation(-asset.instance_offset) @ obj.matrix_world
                points.extend(world_to_camera_view(catalog,catalog.camera,transform @ Vector(c))
                              for c in evaluated.bound_box)
            h,w=array.shape[:2]
            x0=max(0,int(min(p.x for p in points)*w)-16)
            x1=min(w,int(max(p.x for p in points)*w)+17)
            y0=max(0,int(min(p.y for p in points)*h)-16)
            y1=min(h,int(max(p.y for p in points)*h)+17)
            array=array[y0:y1,x0:x1]
        preview=asset.preview_ensure()
        preview.image_size=(256,256)
        preview.image_pixels_float=thumbnail(array).ravel()
        preview.is_image_custom=True
for layer,value in excluded:layer.exclude=value
for im,array in images.values():bpy.data.images.remove(im)
for name,filename in [('M7_Barrel_Switch.py','Source/switch_barrel.py'),('M7_AssetManifest.json','AssetManifest.json')]:
    text=bpy.data.texts.get(name) or bpy.data.texts.new(name)
    text.clear();text.from_string((ROOT/filename).read_text(encoding='utf-8'));text.use_fake_user=True
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'M7_Modular_Replica_V2.blend'),compress=True)
print('M7 modular library finalized as standalone .blend')
