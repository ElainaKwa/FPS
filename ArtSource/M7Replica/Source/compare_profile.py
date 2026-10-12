"""Compare already aligned reference/render silhouettes for visual review."""
import json
from pathlib import Path
import numpy as np
from PIL import Image

root = Path(__file__).resolve().parents[1]
a = np.asarray(Image.open(root/'Reference/Official_M7_Universe_Side.webp').convert('RGBA'))
b = np.asarray(Image.open(root/'Preview_Reference_Profile.png').convert('RGBA'))
assert a.shape == b.shape
ra, rb = a[...,3] > 51, b[...,3] > 51
intersection, union = (ra & rb).sum(), (ra | rb).sum()

def bounds(mask):
    y,x=np.nonzero(mask)
    return [int(x.min()),int(y.min()),int(x.max()),int(y.max())]

def region(box):
    x0,y0,x1,y1=box
    aa,bb=ra[y0:y1,x0:x1],rb[y0:y1,x0:x1]
    return round(float((aa&bb).sum()/(aa|bb).sum()),4)

result = {
    'silhouette_iou': round(float(intersection/union),4),
    'reference_bounds_px': bounds(ra), 'render_bounds_px': bounds(rb),
    'regional_iou': {
        'handguard': region((160,285,938,477)),
        'receiver': region((938,286,1605,631)),
        'stock': region((1605,300,2000,660)),
        'magazine': region((985,540,1220,850)),
        'rear_grip': region((1335,474,1560,800)),
    },
    'meaning': '2D exterior overlap only; does not establish topology, surface detail or hidden-side accuracy.',
    'alignment': 'Same 2000x1000 image canvas and trace coordinates; no post-render rescale or warp.',
}
(root/'ProfileComparison.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
rgb=np.zeros((*ra.shape,3),dtype=np.uint8)
rgb[ra&rb]=[130,140,150]
rgb[ra&~rb]=[235,140,55]
rgb[rb&~ra]=[65,170,235]
Image.fromarray(rgb).save(root/'Preview_Profile_Comparison.png')
print(json.dumps(result))
