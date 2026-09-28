"""Deterministic original synthesized FPS sounds, mono 48 kHz / PCM16.
No recordings, downloads, or third-party samples. Peak limited to -4.4 dBFS.
"""
import math
import random
import struct
import wave
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / 'ArtSource/FPSDemo/Audio'
RATE = 48000


def write(name, duration, bursts=(), tones=(), seed=1):
    rng=random.Random(seed)
    samples=[]
    low=0.0
    for i in range(int(duration*RATE)):
        t=i/RATE
        noise=rng.uniform(-1,1)
        low=.84*low+.16*noise
        value=0.0
        for start,amp,decay,freq in bursts:
            age=t-start
            if age>=0:
                attack=min(age/.0008,1)
                value += attack*amp*math.exp(-age/decay)*(noise*.45+low*1.1+math.sin(2*math.pi*freq*age)*.6)
        for start,amp,length,freq in tones:
            age=t-start
            if 0<=age<length:
                value += amp*math.sin(math.pi*age/length)**2*math.sin(2*math.pi*freq*age)
        samples.append(value)
    peak=max(abs(x) for x in samples) or 1
    gain=.6/peak
    OUT.mkdir(parents=True,exist_ok=True)
    with wave.open(str(OUT/(name+'.wav')),'wb') as wav:
        wav.setparams((1,2,RATE,0,'NONE','not compressed'))
        wav.writeframes(b''.join(struct.pack('<h',int(max(-1,min(1,x*gain))*32767)) for x in samples))
    print(name, len(samples)/RATE, 'seconds')


if __name__=='__main__':
    for i,(name,freq,decay) in enumerate([('AK',92,.075),('M4',145,.05),('MP5',205,.033),('UMP45',112,.052),('M9',260,.038),('HeavyPistol',65,.10)]):
        write('S_Fire_'+name,.45,[(0,1,decay,freq),(.032,.17,.024,850),(.09,.08,.04,230)],seed=30+i)
    write('S_ReloadStart',.24,[(0,.6,.028,620),(.10,.35,.035,330)])
    write('S_ReloadEnd',.26,[(0,.6,.02,380),(.065,.5,.035,780)])
    write('S_Equip',.22,[(0,.25,.045,500),(.095,.6,.018,950)])
    write('S_Inspect',.18,[(0,.4,.028,710)])
    write('S_Empty',.10,[(0,.4,.012,1200)])
    write('S_Hit',.14,tones=[(0,.4,.09,1450),(.025,.2,.08,2100)])
    write('S_Kill',.35,tones=[(0,.4,.12,880),(.08,.35,.12,1175),(.16,.3,.16,1760)])
