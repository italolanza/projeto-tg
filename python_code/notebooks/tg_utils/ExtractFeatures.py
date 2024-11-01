import numpy as np
from scipy.signal import sosfreqz, sosfilt, iirdesign, find_peaks
from scipy.signal.windows import hann
from scipy.stats import skew, kurtosis
from .doFilterHP import HighpassFilter

filtro_hp = HighpassFilter()

def ExtractFeatures(DataBuff, BufferLen, Fs):

    hann_window = hann(BufferLen) # Cria um filtro com Janela Hanning 
    DataBuff = DataBuff * hann_window
    DataBuff = filtro_hp.doFilterHP(DataBuff)
    
    fAxis_F1 = np.arange(start=0,stop=((BufferLen-1)/2), step=1)
    fAxis_F1 = fAxis_F1*(Fs/BufferLen) # creates freq axis
    
    # Cálculo dos parâmetros
    SigRMS = np.sqrt(np.mean(DataBuff**2))
    SigMean = np.mean(DataBuff)
    Abs = np.abs(DataBuff)
    MeanAbs = np.mean(Abs)
    Max = np.max(DataBuff)
    SigMedian = np.median(DataBuff)
    SigVariance = np.var(DataBuff)
    SigSkewness = skew(DataBuff)
    SigKurtosis = kurtosis(DataBuff)
    SigCrestFactor = Max / SigRMS
    SigShapeFactor = SigRMS / MeanAbs
    SigImpulseFactor = Max / MeanAbs
    SigMarginFactor = Max / (MeanAbs**2)

    #print("SigRMS:", SigRMS)
    #print("SigMean:", SigMean)
    #print("MeanAbs:", MeanAbs)
    #print("Max:", Max)
    #print("SigMedian:", SigMedian)
    #print("SigVariance:", SigVariance)
    #print("SigSkewness:", SigSkewness)
    #print("SigKurtosis:", SigKurtosis)
    #print("SigCrestFactor:", SigCrestFactor)
    #print("SigShapeFactor:", SigShapeFactor)
    #print("SigImpulseFactor:", SigImpulseFactor)
    #print("SigMarginFactor:", SigMarginFactor)

    DataFFT = np.abs(np.fft.fft(DataBuff))
    DataFFT = DataFFT[:int(BufferLen/2)] # pega apenas parte positiva da fft
    peaks, props = find_peaks(DataFFT, height=1, distance=10, prominence=1)
    NumPks = len(peaks)
    
    #print(f'Num Peaks: {len(peaks)}')

    indx_pks = np.argsort(props["peak_heights"]) # organiza o indice dos picos de maneira crescente 
                                                 # ultimo indice == indice do maior pico

    if NumPks == 0:
        pks1, pks2, pks3 = 0, 0, 0
        locs1, locs2, locs3 = 0, 0, 0
    elif NumPks == 1:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, pks3 = 0, 0
        locs2, locs3 = 0, 0
    elif NumPks == 2:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, locs2 = props["peak_heights"][indx_pks[-2]], fAxis_F1[peaks[indx_pks[-2]]]
        pks3, locs3 = 0, 0
    elif NumPks >= 3:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, locs2 = props["peak_heights"][indx_pks[-2]], fAxis_F1[peaks[indx_pks[-2]]]
        pks3, locs3 = props["peak_heights"][indx_pks[-3]], fAxis_F1[peaks[indx_pks[-3]]]
    
    return (SigRMS, SigMean, SigMedian, SigVariance, SigSkewness, SigKurtosis, 
            SigCrestFactor, SigShapeFactor, SigImpulseFactor, SigMarginFactor, 
            pks1, pks2, pks3, locs1, locs2, locs3)


def ExtractTimeDomainFeatures(DataBuff, BufferLen, Fs):
    
    # Cálculo dos parâmetros/features
    SigRMS = np.sqrt(np.mean(DataBuff**2))
    SigMean = np.mean(DataBuff)
    Abs = np.abs(DataBuff)
    MeanAbs = np.mean(Abs)
    Max = np.max(DataBuff)
    SigMedian = np.median(DataBuff)
    SigVariance = np.var(DataBuff)
    SigSkewness = skew(DataBuff)
    SigKurtosis = kurtosis(DataBuff)
    SigCrestFactor = Max / SigRMS
    SigShapeFactor = SigRMS / MeanAbs
    SigImpulseFactor = Max / MeanAbs
    SigMarginFactor = Max / (MeanAbs**2)
    
    return (SigRMS, SigMean, SigMedian, SigVariance, SigSkewness, SigKurtosis, 
            SigCrestFactor, SigShapeFactor, SigImpulseFactor, SigMarginFactor)


def ExtractFrequencyDomainFeatures(DataBuff, BufferLen, Fs):
    
    fAxis_F1 = np.arange(start=0,stop=((BufferLen-1)/2), step=1)
    fAxis_F1 = fAxis_F1*(Fs/BufferLen) # creates freq axis

    DataFFT = np.abs(np.fft.fft(DataBuff))
    DataFFT = DataFFT[:int(BufferLen/2)] # pega apenas parte positiva da fft
    peaks, props = find_peaks(DataFFT, height=1, distance=10, prominence=1)
    NumPks = len(peaks)
    
    #print(f'Num Peaks: {len(peaks)}')

    indx_pks = np.argsort(props["peak_heights"]) # organiza o indice dos picos de maneira crescente 
                                                 # ultimo indice == indice do maior pico

    if NumPks == 0:
        pks1, pks2, pks3 = 0, 0, 0
        locs1, locs2, locs3 = 0, 0, 0
    elif NumPks == 1:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, pks3 = 0, 0
        locs2, locs3 = 0, 0
    elif NumPks == 2:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, locs2 = props["peak_heights"][indx_pks[-2]], fAxis_F1[peaks[indx_pks[-2]]]
        pks3, locs3 = 0, 0
    elif NumPks >= 3:
        pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
        pks2, locs2 = props["peak_heights"][indx_pks[-2]], fAxis_F1[peaks[indx_pks[-2]]]
        pks3, locs3 = props["peak_heights"][indx_pks[-3]], fAxis_F1[peaks[indx_pks[-3]]]
    
    return (pks1, pks2, pks3, locs1, locs2, locs3)


#filtro = HighpassFilter()
#x = np.random.randn(1000)
#y = filtro.doFilterHP(x)
#BufferLen = 2048
#fAxis_F1 = np.arange(start=0,stop=((BufferLen-1)/2), step=1)
#print(len(fAxis_F1))

# import matplotlib.pyplot as plt
# from scipy.datasets import electrocardiogram

#x =  np.sin(np.linspace(0, 3 * np.pi, 2048)) +  np.sin(np.linspace(0, 7 * np.pi, 2048)) +  np.sin(np.linspace(0, 10 * np.pi, 2048))
#BufferLen = 2048
#Fs=60
#DataFFT = np.abs(np.fft.fft(x))
#DataFFT = DataFFT[:BufferLen/2] # pega apenas parte positiva da fft
#fAxis_F1 = np.arange(start=0,stop=((BufferLen-1)/2), step=1)
#fAxis_F1 = fAxis_F1*(Fs/BufferLen) # creates freq axis
#peaks, props = find_peaks(x, height=0)
#pks1, locs1 = props["peak_heights"][indx_pks[-1]], fAxis_F1[peaks[indx_pks[-1]]]
#pks2, locs2 = props["peak_heights"][indx_pks[-2]], fAxis_F1[peaks[indx_pks[-2]]]
#pks3, locs3 = props["peak_heights"][indx_pks[-3]], fAxis_F1[peaks[indx_pks[-3]]]
#print(x)
#print(f'peaks: {peaks}')
#print(f'peak_heights {_}')
#print(f'indx_pks: {np.argsort(_["peak_heights"])}')
#print(f'indx_pks: {np.argsort(_["peak_heights"])}')
#plt.plot(x)
#plt.plot(peaks, x[peaks], "x")
#plt.plot(np.zeros_like(x), "--", color="gray")
#plt.show()