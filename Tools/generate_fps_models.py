"""Generate editable OBJ sources for the reusable FPS demo props.

Coordinates are centimeters in Unreal space: rifle points along +X, up is +Z.
Run with Python 3; no third-party packages are needed.
"""

from __future__ import annotations

import math
from pathlib import Path


OUT = Path(__file__).resolve().parents[1] / "ArtSource" / "FPSDemo"


class Mesh:
    def __init__(self, name: str):
        self.name = name
        self.vertices: list[tuple[float, float, float]] = []
        self.faces: list[tuple[tuple[int, ...], str]] = []

    def add(self, verts, faces, material: str):
        offset = len(self.vertices)
        self.vertices.extend(verts)
        for face in faces:
            self.faces.append((tuple(offset + i + 1 for i in face), material))

    def box(self, name, x0, x1, y0, y1, z0, z1, material):
        v = [(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0),
             (x0,y0,z1),(x1,y0,z1),(x1,y1,z1),(x0,y1,z1)]
        f = [(1,2,3,4),(5,8,7,6),(1,5,6,2),(2,6,7,3),(3,7,8,4),(4,8,5,1)]
        self.add(v, [(i-1 for i in face) for face in f], material)

    def cylinder(self, name, center, radius, half_length, axis, material, sides=12):
        cx, cy, cz = center
        v = []
        for end in (-half_length, half_length):
            for i in range(sides):
                a = 2 * math.pi * i / sides
                u, w = radius * math.cos(a), radius * math.sin(a)
                if axis == "x": v.append((cx + end, cy + u, cz + w))
                else: v.append((cx + u, cy + w, cz + end))
        f = [tuple(reversed(range(sides))), tuple(range(sides, sides*2))]
        f += [(i, (i+1)%sides, (i+1)%sides+sides, i+sides) for i in range(sides)]
        self.add(v, f, material)

    def octagonal_prism(self, name, x0, x1, cy, cz, ry, rz, material):
        v = []
        for x in (x0, x1):
            for i in range(8):
                a = math.pi / 8 + 2 * math.pi * i / 8
                v.append((x, cy + ry*math.cos(a), cz + rz*math.sin(a)))
        f = [tuple(reversed(range(8))), tuple(range(8,16))]
        f += [(i,(i+1)%8,(i+1)%8+8,i+8) for i in range(8)]
        self.add(v, f, material)

    def write(self, materials: dict[str, tuple[float, float, float]]):
        OUT.mkdir(parents=True, exist_ok=True)
        mtl = OUT / f"{self.name}.mtl"
        mtl.write_text("\n".join(
            f"newmtl {name}\nKa 0 0 0\nKd {c[0]} {c[1]} {c[2]}\nKs 0.4 0.4 0.4\nNs 80\nillum 2\n"
            for name, c in materials.items()), encoding="ascii")
        lines = [f"mtllib {mtl.name}", f"o {self.name}"]
        lines.extend(f"v {x:.5f} {y:.5f} {z:.5f}" for x,y,z in self.vertices)
        active = None
        for face, mat in self.faces:
            if mat != active:
                lines.append(f"usemtl {mat}")
                active = mat
            lines.append("f " + " ".join(map(str, face)))
        path = OUT / f"{self.name}.obj"
        path.write_text("\n".join(lines) + "\n", encoding="ascii")
        print(f"{path}: {len(self.vertices)} vertices, {len(self.faces)} faces")


def rifle():
    m = Mesh("SM_FPS_Rifle")
    dark, steel, teal, rubber = "GunDark", "GunSteel", "GunTeal", "GunRubber"
    m.octagonal_prism("receiver", -11, 43, 0, 0, 10.5, 9, dark)
    m.box("receiver_side_L", -7, 38, -11.1, -10.2, -3, 5, steel)
    m.box("receiver_side_R", -7, 38, 10.2, 11.1, -3, 5, steel)
    m.box("ejection_port", 17, 35, 11.15, 11.45, 0, 4.3, rubber)
    m.box("bolt_handle", 27, 32, 11.4, 16.5, 0.7, 2.2, steel)
    m.box("upper_rail", -8, 69, -5.2, 5.2, 9, 12.1, rubber)
    for x in range(-5, 68, 7):
        m.box("rail_tooth", x, x+2.8, -5.7, 5.7, 12, 13.4, steel)
    m.octagonal_prism("handguard", 38, 77, 0, 0.5, 8.5, 7.2, steel)
    for side in (-1, 1):
        y0, y1 = (8.0, 8.7) if side > 0 else (-8.7, -8.0)
        for x in (45, 55, 65):
            m.box("vent", x, x+5, y0, y1, -2.0, 2.5, rubber)
        m.box("side_accent", 41, 75, side*8.7-0.55, side*8.7+0.55, 4.1, 5.1, teal)
    m.cylinder("barrel", (78,0,0.5), 3.7, 17.5, "x", steel, 16)
    m.cylinder("muzzle_brake", (96,0,0.5), 6.1, 5.0, "x", dark, 12)
    m.cylinder("muzzle_end", (101.3,0,0.5), 3.4, 0.5, "x", rubber, 16)
    for side in (-1, 1):
        m.box("muzzle_port", 94, 98, side*5.5-0.4, side*5.5+0.4, -1.6, 2.6, rubber)
    m.box("foregrip", 47, 59, -4.0, 4.0, -19, -6, rubber)
    m.box("foregrip_end", 46, 60, -5.0, 5.0, -20.5, -18, teal)
    m.box("stock_beam", -36, -10, -5.3, 5.3, -3.5, 4, steel)
    for side in (-1, 1):
        y0, y1 = (-9,-6) if side < 0 else (6,9)
        m.box("stock_strut", -34, -15, y0, y1, -10, -6, dark)
    m.box("butt_pad", -43, -36, -10.5, 10.5, -11.5, 8.5, rubber)
    m.box("cheek_pad", -32, -7, -8.5, 8.5, 5.2, 9, rubber)
    m.box("pistol_grip", 2, 17, -6.2, 6.2, -29, -8, rubber)
    for z in (-13,-18,-23):
        m.box("grip_texture", 5, 14, 6.2, 6.8, z, z+1.4, steel)
    m.box("trigger_guard_front", 23, 25, -3, 3, -18, -8, steel)
    m.box("trigger_guard_bottom", 11, 25, -3, 3, -19, -17, steel)
    m.box("trigger", 15, 17, -1, 1, -16, -10, rubber)
    m.octagonal_prism("magazine", 25, 40, 0, -22, 7.5, 14, dark)
    m.box("magazine_base", 24, 41, -8.5, 8.5, -37, -35, teal)
    m.box("magazine_stripe_L", 26, 39, -8, -7.7, -30, -28, steel)
    m.box("magazine_stripe_R", 26, 39, 7.7, 8, -30, -28, steel)
    m.box("optic_riser", 9, 30, -5.5, 5.5, 13, 18, steel)
    m.cylinder("optic_scope", (20,0,25), 7, 14, "x", rubber, 16)
    m.cylinder("optic_lens_front", (34.2,0,25), 5, 0.6, "x", teal, 16)
    m.cylinder("optic_lens_back", (5.8,0,25), 5, 0.6, "x", teal, 16)
    m.cylinder("optic_ring_front", (31,0,25), 8, 1.5, "x", steel, 16)
    m.cylinder("optic_ring_back", (9,0,25), 8, 1.5, "x", steel, 16)
    for side in (-1, 1):
        m.box("receiver_glow", -2, 12, side*10.6-0.55, side*10.6+0.55, -5, -3.5, teal)
    m.write({dark:(.025,.035,.055), steel:(.23,.32,.39), teal:(.02,.68,.78), rubber:(.012,.018,.025)})


def station():
    m = Mesh("SM_FPS_HealStation")
    dark, metal, glow = "StationDark", "StationMetal", "StationGlow"
    m.cylinder("outer_platform", (0,0,-10), 160, 8, "z", dark, 24)
    m.cylinder("inner_platform", (0,0,1), 105, 5, "z", metal, 24)
    m.cylinder("glow_ring", (0,0,7), 118, 2.5, "z", glow, 24)
    m.cylinder("pedestal", (0,0,35), 42, 30, "z", dark, 16)
    m.cylinder("column", (0,0,90), 18, 38, "z", metal, 12)
    m.cylinder("energy_core", (0,0,119), 34, 28, "z", glow, 16)
    m.cylinder("core_top", (0,0,150), 41, 5, "z", dark, 16)
    for i in range(4):
        a = math.pi*i/2
        x, y = 59*math.cos(a), 59*math.sin(a)
        m.cylinder("support", (x,y,105), 5, 64, "z", metal, 8)
        m.cylinder("support_light", (x,y,143), 7, 3, "z", glow, 8)
    m.cylinder("canopy", (0,0,171), 76, 8, "z", dark, 16)
    m.cylinder("canopy_glow", (0,0,179), 51, 3, "z", glow, 16)
    for i in range(8):
        a = math.pi*i/4
        x, y = 122*math.cos(a), 122*math.sin(a)
        m.cylinder("floor_beacon", (x,y,3), 7, 4, "z", glow, 8)
    m.write({dark:(.02,.07,.09), metal:(.24,.36,.39), glow:(.02,.92,.38)})


if __name__ == "__main__":
    rifle()
    station()
