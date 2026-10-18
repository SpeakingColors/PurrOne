import numpy as np, sys, os
d = sys.argv[1]; sr = 48000
def load(n): return np.fromfile(os.path.join(d, n), dtype=np.float32)
def midi(n): return 440*2**((n-69)/12)
def alias_report(name, f0):
    x = load(name)[int(0.3*sr):int(0.3*sr)+2**16].astype(np.float64)
    w = np.blackman(len(x)); X = np.abs(np.fft.rfft(x*w))**2; fr = np.fft.rfftfreq(len(x), 1/sr)
    bw = 2*sr/len(x)*3
    harm = np.zeros(len(fr), bool)
    for k in range(1, int((sr/2)/f0)+1): harm |= np.abs(fr-k*f0) < bw
    tot = X[fr>20].sum(); al = X[(~harm)&(fr>20)].sum()
    print(f"{name:14s} alias/total = {10*np.log10(al/tot+1e-20):7.1f} dB")
for n,note in [("saw_c7.raw",96),("saw_c5.raw",72),("pulse_c6.raw",84)]: alias_report(n, midi(note))
for n in ["saw_c7.raw","saw_c5.raw","pulse_c6.raw","sync_c4.raw","selfosc.raw","sub_c1.raw","extreme.raw"]:
    x = load(n)
    print(f"{n:14s} nan={np.isnan(x).any()} peak={np.abs(x).max():.3f} rms={np.sqrt((x**2).mean()):.3f} dc={x[int(.3*sr):int(1.0*sr)].mean():+.4f}")
x = load("selfosc.raw")[int(.3*sr):int(.9*sr)]; X=np.abs(np.fft.rfft(x*np.hanning(len(x)))); print("selfosc peak Hz", np.fft.rfftfreq(len(x),1/sr)[X.argmax()], "(cutoff set 880)")
x = load("sub_c1.raw")[int(.3*sr):int(.9*sr)]; X=np.abs(np.fft.rfft(x*np.hanning(len(x)))); print("sub_c1 strongest Hz", np.fft.rfftfreq(len(x),1/sr)[X.argmax()], "(C1 = 32.7, sub = 16.4 octave below A)")
x = load("extreme.raw"); print("extreme tail last 1000 abs max", np.abs(x[-1000:]).max())
