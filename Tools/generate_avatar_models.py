"""Original modular training robot. cm, forward +X, segment meshes length 1 on X.

Rigid joints are deliberately visible; no third-party art or DCC dependency.
"""
from pathlib import Path
import generate_weapon_models as wp

wp.base.OUT = Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/Avatar'
PALETTE = {'Armor': (.12,.19,.23), 'Edge': (.4,.49,.53),
           'Joint': (.025,.032,.038), 'Visor': (.025,.48,.58),
           'Badge': (.78,.83,.77)}


def box(m, x0,x1,y0,y1,z0,z1,mat):
    m.box('',x0,x1,y0,y1,z0,z1,mat)


def write(m):
    m.write(PALETTE)
    return m


def torso():
    m=wp.Mesh('SM_AV_Torso')
    m.octagonal_prism('',-10,9,0,9,21,26,'Armor')
    box(m,9,12,-15,15,5,26,'Edge')
    box(m,12,13,-11,11,9,22,'Armor')
    for y in (-12,0,12):
        box(m,11,16,y-4,y+4,-10,2,'Joint')
        box(m,16,17,y-3,y+3,-8,0,'Edge')
    box(m,-17,-10,-14,14,-4,24,'Joint')
    for y in (-10,10):
        box(m,-19,-17,y-2,y+2,0,20,'Edge')
    box(m,13,14,-7,7,16,19,'Visor')
    m.cylinder('',(0,0,37),6,6,'z','Joint')
    return write(m)


def helmet():
    m=wp.Mesh('SM_AV_Helmet')
    m.octagonal_prism('',-10,9,0,0,12,15,'Armor')
    box(m,9,11,-10,10,-2,7,'Visor')
    box(m,11,12,-11,11,7,9,'Edge')
    box(m,10,14,-6,6,-10,-4,'Joint')
    for y in (-4,0,4): box(m,14,15,y-1,y+1,-9,-5,'Edge')
    for y in (-13,13): box(m,-5,3,y-2,y+2,-3,8,'Joint')
    m.cylinder('',(0,0,-16),6,3,'z','Joint')
    return write(m)


def pelvis():
    m=wp.Mesh('SM_AV_Pelvis')
    box(m,-9,9,-17,17,-5,7,'Joint')
    box(m,9,12,-15,15,-3,5,'Armor')
    box(m,12,13,-3,3,0,4,'Badge')
    for y in (-17,17): box(m,-6,6,y-3,y+3,-9,5,'Edge')
    return write(m)


def segment(name,radius):
    m=wp.Mesh('SM_AV_'+name)
    m.octagonal_prism('',0,1,0,0,radius*.72,radius*.72,'Joint')
    m.octagonal_prism('',.12,.83,0,0,radius,radius,'Armor')
    m.octagonal_prism('',.15,.23,0,0,radius*1.08,radius*1.08,'Edge')
    box(m,.33,.71,-radius*.5,radius*.5,radius*.85,radius*1.03,'Edge')
    return write(m)


def glove():
    m=wp.Mesh('SM_AV_Glove')
    m.octagonal_prism('',-5,2,0,0,3.9,3.0,'Joint')
    box(m,-4,1,-3,3,2.3,3.5,'Armor')
    for y in (-2.7,-.9,.9,2.7):
        box(m,1,4,y-.7,y+.7,-2.8,1.8,'Joint')
        box(m,1.1,3.3,y-.6,y+.6,1.8,2.6,'Edge')
    box(m,-3,1,3.2,5,-2,0,'Joint')
    return write(m)


def boot():
    m=wp.Mesh('SM_AV_Boot')
    box(m,-6,14,-6,6,-5,1,'Joint')
    box(m,-5,5,-5,5,1,10,'Armor')
    box(m,5,13,-5.5,5.5,1,5,'Edge')
    for x in (-3,2,7,12): box(m,x,x+1,-6.1,6.1,-4.8,-3,'Edge')
    return write(m)


if __name__=='__main__':
    meshes=[torso(),helmet(),pelvis(),glove(),boot()]
    meshes += [segment(n,r) for n,r in [('UpperArm',7),('Forearm',5.5),('Thigh',8.5),('Shin',6.7)]]
    # A neutral assembled preview is an asset for inspecting proportions, not the runtime rig.
    preview=wp.Mesh('SM_AV_Preview')
    def place(mesh,origin,segment_length=None):
        start=len(preview.vertices)
        for x,y,z in mesh.vertices:
            if segment_length: x,y,z=z,y,-x*segment_length
            preview.vertices.append((x+origin[0],y+origin[1],z+origin[2]))
        preview.faces += [(tuple(i+start for i in f),mat) for f,mat in mesh.faces]
    parts={m.name.removeprefix('SM_AV_'):m for m in meshes}
    place(parts['Torso'],(0,0,87));place(parts['Helmet'],(0,0,141));place(parts['Pelvis'],(0,0,71))
    for side in (-1,1):
        place(parts['UpperArm'],(0,side*26,115),29)
        place(parts['Forearm'],(0,side*26,86),25)
        place(parts['Glove'],(0,side*26,59))
        place(parts['Thigh'],(0,side*12,71),34)
        place(parts['Shin'],(0,side*12,37),27)
        place(parts['Boot'],(0,side*12,5))
    write(preview)
