"""Check PCM source/master-output files for non-silent, unclipped audio."""
import array
import math
import sys
import wave
from pathlib import Path

paths=[Path(p) for p in sys.argv[1:]]
if not paths:
    paths=sorted((Path(__file__).resolve().parents[1]/'ArtSource/FPSDemo/Audio').glob('*.wav'))
assert paths, 'No audio files'
for path in paths:
    with wave.open(str(path),'rb') as wav:
        assert wav.getsampwidth()==2, str(path)
        frames=wav.getnframes()
        rate=wav.getframerate()
        samples=array.array('h',wav.readframes(frames))
        if sys.byteorder!='little': samples.byteswap()
    assert frames and samples, str(path)
    peak=max(abs(v) for v in samples)/32768
    rms=math.sqrt(sum((v/32768)**2 for v in samples)/len(samples))
    assert peak>.0001 and rms>.000001, 'Silent: '+str(path)
    assert peak<.999, 'Clipping: '+str(path)
    print(f'PASS {path.name}: {frames/rate:.2f}s peak={peak:.4f} rms={rms:.6f}')
