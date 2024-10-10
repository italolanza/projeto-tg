import numpy as np
from scipy.signal import sosfreqz, sosfilt, iirdesign

class HighpassFilter:
    Hd = None # Class Member
    
    def __init__(self):
        pass
        
    def doFilterHP(self, x):
        if HighpassFilter.Hd is None:
            Fstop = 20       # Frequência de parada
            Fpass = 30       # Frequência de passagem
            Astop = 60       # Atenuação na faixa de parada (dB)
            Apass = 0.0001   # Ripple na faixa de passagem (dB)
            Fs = 48000       # Frequência de amostragem
            
            sos = highpass_filter_design(Fstop, Fpass, Astop, Apass, Fs)
            HighpassFilter.Hd = sos
            
        y = sosfilt(HighpassFilter.Hd, x)
        return y

def highpass_filter_design(Fstop, Fpass, Astop, Apass, Fs):
    #wp = 2 * np.pi * Fpass / Fs
    #ws = 2 * np.pi * Fstop / Fs
    
    sos = iirdesign(wp=Fpass, ws=Fstop, gstop=Astop, gpass=Apass, analog=False, ftype='butter', output='sos', fs=Fs)
    return sos

# Exemplo de uso
#filtro = HighpassFilter()
#input_signal = np.random.randn(1000)  # Sinal de entrada aleatório
#output_signal = filtro.doFilterHP(input_signal)