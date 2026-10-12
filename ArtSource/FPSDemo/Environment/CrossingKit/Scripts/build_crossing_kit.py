"""Editable industrial crossing kit; uses the project's shared modelling helpers/palette."""
import sys
import json
import math
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[5]
OUT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Tools'))
import build_industrial_art as kit

for folder in ('Source','Exports','Previews'):
    (OUT/folder).mkdir(parents=True,exist_ok=True)
kit.OUT = OUT
kit.ASSETS = []
kit.reset()
box,cyl = kit.box,kit.cylinder

col=kit.group('SM_IC_Portal600')
for side in (-1,1):
    box('Upright',(0,side*3.25,2.65),(.62,.50,5.30),'Graphite',.035)
    box('Footplate',(0,side*3.25,.10),(.95,.90,.20),'Steel',.025)
    box('Upright insert',(-.33,side*3.25,2.75),(.08,.25,4.50),'Steel',.018)
    box('Safety stripe',(-.38,side*3.25,1.15),(.035,.51,.22),'Yellow',.004)
box('Header',(0,0,5.35),(.8,7.05,.62),'Graphite',.04)
box('Header face',(-.415,0,5.36),(.05,5.9,.30),'Steel',.009)
for y in (-2.0,2.0):
    box('Header locator',(-.45,y,5.36),(.02,.36,.32),'Yellow',.004)
kit.export(col,'Meshes')

col=kit.group('SM_IC_Cargo180')
box('Container shell',(0,0,.9),(1.8,1.8,1.8),'Graphite',.045)
for side in (-1,1):
    for q in (-.76,.76):
        box('Frame',(q,side*.92,.9),(.10,.08,1.82),'Steel',.013)
    for z in (.12,1.68):
        box('Frame beam',(0,side*.94,z),(1.82,.08,.10),'Steel',.012)
    for q in (-.55,-.28,0,.28,.55):
        box('Pressed rib',(q,side*.918,.9),(.065,.035,1.35),'Graphite',.008)
    box('ID plate',(.40,side*.96,1.20),(.36,.03,.23),'Ceramic',.004)
    box('Caution',(-.52,side*.965,.32),(.4,.026,.10),'Yellow',.002)
kit.export(col,'Meshes')

col=kit.group('SM_IC_Vent150')
box('Casing',(0,0,0),(.16,1.5,1.0),'Steel',.016)
box('Dark cavity',(-.09,0,0),(.04,1.36,.86),'Rubber',.002)
for z in (-.33,-.22,-.11,0,.11,.22,.33):
    box('Louver',(-.14,0,z),(.14,1.31,.045),'Graphite',.006,rot=(0,.3,0))
kit.export(col,'Meshes')

col=kit.group('SM_IC_ServicePipe400')
cyl('Service main',(0,0,1.35),.17,4,'Steel','X')
for x in (-1.8,-.8,.8,1.8):
    cyl('Coupling',(x,0,1.35),.21,.12,'Graphite','X')
for x in (-1.45,1.45):
    box('Pipe support',(x,0,.59),(.12,.12,1.18),'Graphite',.014)
    box('Support base',(x,0,.05),(.45,.50,.10),'Steel',.009)
kit.export(col,'Meshes')

col=kit.group('SM_IC_Bollard120')
box('Concrete foot',(0,0,.10),(.55,.55,.20),'Concrete',.025)
box('Post',(0,0,.70),(.22,.22,1.20),'Graphite',.025)
for z in (.52,.90):
    box('Visibility band',(0,0,z),(.225,.225,.16),'Yellow',.01)
kit.export(col,'Meshes')

# Keep separated editable parts/modifiers. Gallery translations happen after origin-aligned export.
for i,col in enumerate([c for c in bpy.data.collections if c.name.startswith('SM_IC_')]):
    for obj in list(col.objects):
        obj.location.x+=(i%3)*5.5
        obj.location.y+=(i//3)*9
kit.lights_camera((5,3,1.8),(19,-23,19),22,OUT/'Previews/BlenderKit.png')
bpy.context.scene.render.resolution_x=1600
bpy.context.scene.render.resolution_y=1100
bpy.context.scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Source/CrossingKit.blend'))
bpy.ops.render.render(write_still=True)
manifest={'generator':'ArtSource/FPSDemo/Environment/CrossingKit/Scripts/build_crossing_kit.py',
          'shared_helpers':'Tools/build_industrial_art.py','blender':bpy.app.version_string,
          'units':'meters in Blender, centimeters in Unreal','assets':kit.ASSETS,
          'material_source':'ArtSource/FPSDemo/Industrial/manifest.json',
          'unreal_material_root':'/Game/FPSDemo/Art/Industrial/Materials'}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('CROSSING_KIT PASS assets='+str(len(kit.ASSETS)),flush=True)
