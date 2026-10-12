"""Switch a complete equipped M7's barrel and follow its dependent sockets.

Run this text in Blender, then use the M7 panel in the viewport sidebar.
The two barrel collections remain original editable assets.
"""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1] if '__file__' in globals() else Path(bpy.data.filepath).resolve().parent

def switch_barrel(scene,barrel_name):
    embedded=bpy.data.texts.get('M7_AssetManifest.json')
    manifest=json.loads(embedded.as_string() if embedded else (ROOT/'AssetManifest.json').read_text(encoding='utf-8'))
    rows={r['name']:r for r in manifest['assets']}
    instances=[o for o in scene.objects if o.instance_collection and o.instance_collection.name in rows]
    installed={o.instance_collection.name:o for o in instances}
    barrels=[o for o in instances if rows[o.instance_collection.name]['mount_key']=='Barrel']
    if len(barrels)!=1:raise ValueError('当前场景必须安装一套枪管。请选择整枪装配场景。')
    if barrel_name not in rows or rows[barrel_name]['mount_key']!='Barrel':raise ValueError('未知枪管资产')
    for name in installed:
        allowed=rows[name].get('compatible_barrels')
        if allowed and barrel_name not in allowed:
            raise ValueError('此场景的机械瞄具仅适用于标准枪管。请选择倍率镜装配场景。')
    root=Vector(manifest['mounts']['Barrel'])
    barrel=rows[barrel_name]
    fixed={key:Vector(value) for key,value in manifest['mounts'].items()}
    for key,value in barrel['local_mounts_m'].items():fixed[key]=root+Vector(value)
    barrel_object=barrels[0]
    barrel_object.instance_collection=bpy.data.collections[barrel_name]
    barrel_object.location=root;barrel_object['source_asset']=barrel_name
    for name,obj in installed.items():
        key=rows[name]['mount_key']
        if key in ('Foregrip','Muzzle'):obj.location=fixed[key]
    scene['barrel_asset']=barrel_name
    scene['assembly_parts']=json.dumps([barrel_name if rows[n]['mount_key']=='Barrel' else n for n in installed])
    scene.view_layers[0].update()
    return {'barrel':barrel_name,'foregrip':list(fixed['Foregrip']),'muzzle':list(fixed['Muzzle'])}

class M7_OT_switch_barrel(bpy.types.Operator):
    bl_idname='m7.switch_barrel'
    bl_label='更换 M7 枪管'
    bl_options={'REGISTER','UNDO'}
    variant:bpy.props.EnumProperty(items=[('SM_M7_Barrel_Default','标准枪管',''),('SM_M7_Barrel_Tidal','堤风超长枪管','')])
    def execute(self,context):
        try:switch_barrel(context.scene,self.variant)
        except ValueError as error:
            self.report({'WARNING'},str(error));return {'CANCELLED'}
        return {'FINISHED'}

class M7_PT_barrel(bpy.types.Panel):
    bl_label='M7 枪管更换'
    bl_idname='M7_PT_barrel'
    bl_space_type='VIEW_3D'
    bl_region_type='UI'
    bl_category='M7 配装'
    def draw(self,context):
        layout=self.layout
        layout.operator('m7.switch_barrel',text='标准枪管').variant='SM_M7_Barrel_Default'
        layout.operator('m7.switch_barrel',text='堤风超长枪管').variant='SM_M7_Barrel_Tidal'

def register():
    for cls in (M7_OT_switch_barrel,M7_PT_barrel):
        old=getattr(bpy.types,cls.__name__,None)
        if old:bpy.utils.unregister_class(old)
        bpy.utils.register_class(cls)

if __name__=='__main__':register()
