"""Generate original UI surfaces and line icons; never rasterize text or controls."""
from pathlib import Path
import math
import random
from PIL import Image, ImageDraw
output = Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/Industrial/UI'
output.mkdir(parents=True, exist_ok=True)
panel = Image.new('RGBA', (128, 128))
ImageDraw.Draw(panel).polygon([(0,0),(118,0),(127,9),(127,127),(0,127)], fill=(255,255,255,255))
panel.save(output / 'Panel.png')

rng = random.Random(20261012)
for name, size, base in [('Backdrop', (1600,900), (20,25,26)),
                         ('Surface', (512,512), (27,33,35)),
                         ('Paper', (512,256), (216,217,213))]:
    im = Image.new('RGBA', size)
    pixels = im.load()
    for y in range(size[1]):
        for x in range(size[0]):
            grain = rng.uniform(-1.2,1.2)
            shade = (0.65*math.sin(x*.017+y*.022) - 2*((x/size[0]-.5)**2+(y/size[1]-.5)**2))
            pixels[x,y] = tuple(max(0,min(255,round(c+grain+shade))) for c in base)+(255,)
    im.save(output / f'T_UI_{name}.png')

# Independent masks keep icon colour and size editable in the UMG Designer.
for name in ['Book','Exit','Shield','Check','CheckCircle','Alert','Signal','Chevron','Arrow','Triangle','Corner']:
    im=Image.new('RGBA',(256,256)); d=ImageDraw.Draw(im); ink=(255,255,255,255)
    def line(points, width=12): d.line([(x*4,y*4) for x,y in points],fill=ink,width=width,joint='curve')
    if name=='Book':
        line([(8,9),(30,16),(56,9),(56,51),(32,57),(8,50),(8,9)])
        line([(32,17),(32,57)]);line([(15,23),(23,26)]);line([(15,34),(23,37)]);line([(40,25),(48,23)]);line([(40,36),(48,34)])
    elif name=='Exit':
        line([(10,7),(56,7),(56,57),(10,57),(10,7)]);line([(15,32),(43,32)]);line([(32,20),(44,32),(32,44)])
    elif name=='Shield':
        pts=[(10,12),(32,5),(54,12),(53,35),(46,49),(32,59),(18,49),(11,35),(10,12)]
        line(pts,10); d.polygon([(x*4,y*4) for x,y in [(11,13),(32,6),(32,31),(11,31)]],fill=ink)
        d.polygon([(x*4,y*4) for x,y in [(32,31),(53,31),(51,38),(44,49),(32,57)]],fill=ink)
    elif name in ['Check','CheckCircle']:
        if name=='CheckCircle':d.ellipse((8,8,248,248),fill=ink)
        color=(0,0,0,0) if name=='CheckCircle' else ink
        d.line([(17*4,32*4),(28*4,42*4),(48*4,19*4)],fill=color,width=16,joint='curve')
    elif name=='Alert':
        d.ellipse((8,8,248,248),fill=ink);d.rectangle((120,50,136,153),fill=(0,0,0,0));d.ellipse((120,174,138,192),fill=(0,0,0,0))
    elif name=='Signal':
        d.ellipse((112,112,144,144),fill=ink)
        for r in [60,104]:
            d.arc((128-r,128-r,128+r,128+r),145,215,fill=ink,width=10);d.arc((128-r,128-r,128+r,128+r),-35,35,fill=ink,width=10)
    elif name=='Chevron':line([(11,24),(32,45),(53,24)],10)
    elif name=='Arrow':line([(4,32),(57,32)],16);line([(36,10),(58,32),(36,54)],16)
    elif name=='Triangle':d.polygon([(128,8),(250,240),(6,240)],fill=ink)
    elif name=='Corner':d.polygon([(0,0),(255,0),(255,255)],fill=ink)
    im.resize((64,64),Image.Resampling.LANCZOS).save(output/f'T_UI_{name}.png')

shade=Image.new('RGBA',(512,128))
for y in range(128):
    for x in range(512):
        shade.putpixel((x,y),(0,0,0,round(180*(1-y/128)**1.5*(1-.7*x/512))))
shade.save(output/'T_UI_PreviewShade.png')

card=Image.open(output/'T_UI_Paper.png').convert('RGBA')
mask=Image.new('L',(2048,1024));ImageDraw.Draw(mask).rounded_rectangle((0,0,2047,1023),radius=20,fill=255)
card.putalpha(mask.resize(card.size,Image.Resampling.LANCZOS));card.save(output/'T_UI_Card.png')
edge=Image.new('RGBA',(512,512));ImageDraw.Draw(edge).rounded_rectangle((2,2,509,509),radius=20,outline=(255,255,255,255),width=8)
edge.resize((128,128),Image.Resampling.LANCZOS).save(output/'T_UI_CardEdge.png')
