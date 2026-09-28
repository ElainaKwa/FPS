"""Original low-poly weapon meshes, centimeters, +X muzzle, +Z up.

No downloaded models or dependencies. Run with Python 3. Source geometry
and material groups remain editable through OBJ import in a DCC application.
"""
import json
from pathlib import Path
import generate_fps_models as base

base.OUT = Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/Weapons'
COLORS = {'Metal': (.09, .12, .15), 'Steel': (.38, .43, .47),
          'Polymer': (.026, .032, .037), 'Wood': (.32, .105, .035),
          'Sand': (.40, .31, .19), 'Mark': (.72, .78, .68)}


class Mesh(base.Mesh):
    def box(self, name, x0, x1, y0, y1, z0, z1, material):
        first = len(self.faces)
        super().box(name, x0, x1, y0, y1, z0, z1, material)
        # Outward winding (the older source generator's boxes faced inward).
        self.faces[first:] = [(tuple(reversed(f)), m) for f, m in self.faces[first:]]

    def profile(self, points, width, material):
        # A clockwise X/Z outline, extruded across Y.
        n = len(points)
        vertices = [(x, y, z) for y in (-width/2, width/2) for x, z in points]
        faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
        faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
        self.add(vertices, faces, material)


def box(m, x0, x1, z0, z1, width, mat='Metal'):
    m.box('', x0, x1, -width/2, width/2, z0, z1, mat)


def barrel(m, start, end, radius=1.0, z=0):
    m.cylinder('', ((start+end)/2, 0, z), radius, (end-start)/2, 'x', 'Steel', 12)
    m.cylinder('', (end-1, 0, z), radius*1.5, 1.3, 'x', 'Metal', 12)
    m.cylinder('', (end+.31, 0, z), radius*.7, .015, 'x', 'Polymer', 12)


def grip(m, x=0, mat='Polymer'):
    m.profile([(x-3,-3),(x+3,-3),(x,-16),(x-6,-16)], 4, mat)
    for z in (-7,-10,-13):
        box(m, x-5, x+1, z, z+.5, 4.2, 'Metal')
    box(m, x+2, x+12, -10, -9, 1.3)
    box(m, x+11, x+12, -9, -3, 1.3)
    box(m, x+5, x+6, -8, -3, .7, 'Polymer')


def sights(m, front, height):
    # Open rear notch and narrow front post, no opaque lens.
    for y in (-1.4, 1.4):
        m.box('', 0, 2, y-.4, y+.4, height-2, height, 'Metal')
    box(m, front, front+1, 2, height-1, .55)
    box(m, front, front+1, height-1, height-.3, .25, 'Mark')


def rail(m, end):
    box(m, -2, end, 3.5, 4.5, 3)
    for x in range(0, int(end), 3):
        box(m, x, x+1.3, 4.5, 5, 3.5, 'Steel')


def rifle(kind):
    m = Mesh('SM_WP_'+kind)
    box(m, -8, 20, -4, 4, 5.2)
    grip(m)
    if kind == 'AK':
        m.profile([(-35,2),(-9,2),(-9,-2),(-31,-10),(-36,-10)], 4.8, 'Wood')
        box(m, 20, 39, -3, 3, 5.5, 'Wood')
        barrel(m, 36, 57, .95)
        barrel(m, 18, 43, .65, 3.6)
        m.profile([(12,-4),(19,-4),(21,-12),(27,-24),(21,-26),(15,-16)], 3.6, 'Metal')
        for z in (-9,-13,-17):
            box(m, 16, 21, z, z+.5, 3.85, 'Steel')
        sights(m, 50, 6)
    else:
        barrel(m, -25, -8, 1.5)
        m.profile([(-34,3),(-20,3),(-18,-2),(-30,-9),(-35,-9)], 4.5, 'Polymer')
        box(m, -35, -33, -10, 4, 5, 'Polymer')
        m.octagonal_prism('', 19, 42, 0, 0, 3.6, 3.6, 'Sand')
        for x in range(22, 40, 4):
            for y in (-3.4, 3.4):
                m.box('', x, x+2, y-.15, y+.15, -1, 1, 'Polymer')
        barrel(m, 40, 56)
        m.profile([(12,-4),(19,-4),(19,-14),(16,-22),(10,-22)], 3.4, 'Sand')
        rail(m, 41)
        sights(m, 41, 7)
    box(m, 3, 10, .5, 2.5, 5.35, 'Polymer')
    m.box('', 9, 11, 2.5, 4.5, 0, 1, 'Steel')
    return m, 57 if kind == 'AK' else 56, 6 if kind == 'AK' else 7


def smg(kind):
    m = Mesh('SM_WP_'+kind)
    if kind == 'MP5':
        m.octagonal_prism('', -7, 25, 0, 0, 2.8, 3.2, 'Metal')
        m.octagonal_prism('', 16, 31, 0, -1, 3.5, 3.8, 'Polymer')
        for y in (-2, 2):
            m.box('', -28, -6, y-.4, y+.4, -.4, .4, 'Steel')
        box(m, -30, -27, -8, 3, 4, 'Polymer')
        m.profile([(10,-3),(15,-3),(16,-12),(20,-22),(15,-23),(11,-14)], 2.7, 'Metal')
        barrel(m, 28, 38, .85)
        sights(m, 30, 6)
    else:
        box(m, -8, 30, -4, 4, 5.7, 'Polymer')
        # Skeleton stock: separate struts leave a genuine opening.
        box(m, -30, -8, 0, 2, 3.3, 'Polymer')
        box(m, -30, -8, -7, -5, 3.3, 'Polymer')
        box(m, -31, -28, -9, 3, 4.2, 'Polymer')
        box(m, 11, 17, -24, -3, 3.4, 'Polymer')
        box(m, 10, 18, -25, -23, 4, 'Sand')
        barrel(m, 28, 39, 1.1)
        rail(m, 27)
        sights(m, 27, 7)
        for x in (18,23,28):
            box(m, x, x+2, -.5, 1, 5.8, 'Steel')
    grip(m, -2)
    return m, 38 if kind == 'MP5' else 39, 6 if kind == 'MP5' else 7


def pistol(kind):
    m = Mesh('SM_WP_'+kind)
    heavy = kind == 'HeavyPistol'
    end = 25 if heavy else 20
    mat = 'Steel' if heavy else 'Metal'
    m.octagonal_prism('', -5, end, 0, 1, 2.6 if heavy else 2.0, 3.1, mat)
    box(m, -5, end-4, -4, -1.5, 3.8, 'Metal')
    grip(m, 0, 'Sand' if heavy else 'Polymer')
    box(m, -7, 1, -17, -15, 4.5, 'Polymer')
    for x in (-4,-2,0,2):
        box(m, x, x+.6, -.2, 2.6, 5.0 if heavy else 3.9, 'Polymer')
    barrel(m, end-1, end+1, 1 if heavy else .7, .6)
    sights(m, end-2, 5)
    return m, end+1, 5


if __name__ == '__main__':
    manifest = {}
    for kind, make in [('AK',rifle),('M4',rifle),('MP5',smg),('UMP45',smg),('M9',pistol),('HeavyPistol',pistol)]:
        mesh, muzzle, sight = make(kind)
        mesh.write(COLORS)
        manifest[kind] = {'mesh': mesh.name, 'muzzle_x': muzzle, 'sight_z': sight,
                          'vertices': len(mesh.vertices), 'triangles': sum(len(f)-2 for f,_ in mesh.faces)}
    (base.OUT/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
