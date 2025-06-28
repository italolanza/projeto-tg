import scipy.signal as signal
import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec


class GeradorFiltroPassaFaixa:

    # Especificações do filtro baseadas no artigo
    FS_FILTER = 48000       # Frequência de amostragem do filtro em Hz
    LOWCUT = 20.0          # Frequência de corte inferior em Hz
    HIGHCUT = 20000.0       # Frequência de corte superior em Hz
    ORDER = 4               # Ordem do filtro
    FILTER_TYPE = 'butter'  # Opções: 'butter', 'cheby1', 'cheby2'
    RP = 0.5                # Ripple máximo na banda de passagem em dB (apenas para Chebyshev)

    def __init__(self, lowcut=100.0, highcut=20000.0, fs=48000, order=4, filter_type="cheby1", ripple=0.5):
        self.LOWCUT=lowcut
        self.HIGHCUT=highcut
        self.FS_FILTER=fs
        self.ORDER=order
        self.FILTER_TYPE=filter_type
        self.rs=ripple
        

    def projetar_filtro(self):
        """Projeta o filtro e retorna um objeto do tipo Filtro que contem coeficientes em formato SOS."""
        
        # Normaliza as frequências de corte para a frequência de Nyquist (FS/2)
        nyquist = 0.5 * self.FS_FILTER
        low = self.LOWCUT / nyquist
        high = self.HIGHCUT / nyquist

        # Projeta o filtro e obtém os coeficientes em formato SOS (Second-Order Sections)
        # 'sos' é uma matriz de formato [n_sections, 6]
        # Cada linha é [b0, b1, b2, a0, a1, a2]
        print(f"Projetando o filtro {self.FILTER_TYPE.capitalize()}...")
        if self.FILTER_TYPE == 'butter':
            sos = signal.butter(self.ORDER, [low, high], btype='band', output='sos')
        elif self.FILTER_TYPE == 'cheby1':
            # Chebyshev Tipo 1: Roll-off mais acentuado, com ripple na banda de passagem.
            sos = signal.cheby1(self.ORDER, self.RP, [low, high], btype='band', output='sos')
        elif self.FILTER_TYPE == 'cheby2':
            sos = signal.cheby2(self.ORDER, self.RP, [low, high], btype='band', output='sos')
        else:
            raise ValueError("Tipo de filtro não suportado. Use 'butter', 'cheby1' ou 'cheby2'.")

        # A biblioteca CMSIS-DSP usa coeficientes para a forma de transferência direta I (Direct Form I).
        # A estrutura de dados dela é [b0, b1, b2, a1, a2] para cada estágio. Note que a0 é sempre 1 e omitido,
        # e os sinais de a1 e a2 são invertidos.
        print(f"sos coef= {sos}")
        num_stages = sos.shape[0]
        coeffs_cmsis = np.zeros(num_stages * 5)

        for i in range(num_stages):
            b0, b1, b2, a0, a1, a2 = sos[i]
            coeffs_cmsis[i*5 + 0] = b0
            coeffs_cmsis[i*5 + 1] = b1
            coeffs_cmsis[i*5 + 2] = b2
            coeffs_cmsis[i*5 + 3] = (a1 * (-1))     # Inverte o sinal de a1
            coeffs_cmsis[i*5 + 4] = (a2 * (-1))     # Inverte o sinal de a1

        # Exibe os coeficientes no formato para copiar e colar em C
        print(f"// Coeficientes do Filtro Passa-Faixa {self.FILTER_TYPE.capitalize()} de Ordem {self.ORDER}")
        print(f"// Fs={self.FS_FILTER}Hz, Fc=[{self.LOWCUT}Hz, {self.HIGHCUT}Hz]")
        print(f"#define NUM_STAGES {num_stages}")
        print("float32_t pCoeffs[NUM_STAGES * 5] = {")
        for i, c in enumerate(coeffs_cmsis):
            print(f"    {c:.8f}f,", end='')
            if (i + 1) % 5 == 0:
                print("")
        print("};")

        return Filtro(lowcut=self.LOWCUT, highcut=self.HIGHCUT, fs=self.FS_FILTER, order=self.ORDER, filter_type=self.FILTER_TYPE, sos_coefs=sos)

class Filtro:

    def __init__(self, lowcut, highcut, fs, order, filter_type, sos_coefs):
        self.LOWCUT = lowcut
        self.HIGHCUT = highcut
        self.FS_FILTER = fs
        self.ORDER = order
        self.FILTER_TYPE = filter_type
        self.SOS_COEF = sos_coefs

    def filtrar_sinal(self, input_signal):
        return signal.sosfilt(self.SOS_COEF, input_signal)


def visualizar_graficos_do_filtro(filter, wav_filepath):
    """
    Carrega um arquivo .wav, aplica o filtro e plota os resultados
    nos domínios do tempo e da frequência para análise.
    """
    try:
        # --- Carregar o Arquivo de Áudio ---
        fs_wav, audio_data = wavfile.read(wav_filepath)
        print(f"Arquivo '{wav_filepath}' carregado com sucesso.")
        print(f"Frequência de amostragem do áudio: {fs_wav} Hz")

        if fs_wav != filter.FS_FILTER:
            print(f"ERRO: A frequência de amostragem do áudio ({fs_wav} Hz) é diferente da do filtro ({filter.FS_FILTER} Hz).")
            print("Por favor, use um arquivo de áudio com a amostragem correta.")
            return

        if np.issubdtype(audio_data.dtype, np.integer):
            audio_data = audio_data / np.iinfo(audio_data.dtype).max
        
        if audio_data.ndim > 1:
            print("Áudio estéreo detectado. Usando apenas o canal esquerdo.")
            audio_data = audio_data[:, 0]
            
    except FileNotFoundError:
        print(f"ERRO: Arquivo não encontrado em '{wav_filepath}'")
        return
    except Exception as e:
        print(f"Ocorreu um erro ao ler o arquivo: {e}")
        return

    # --- Aplicar o Filtro ao Sinal ---
    audio_filtered = filter.filtrar_sinal(audio_data)
    print("Filtragem concluída.")

    # --- Preparar dados para Visualização ---
    time_axis = np.arange(len(audio_data)) / fs_wav
    n_fft = len(audio_data)
    fft_original = np.fft.fft(audio_data, n_fft)
    fft_filtered = np.fft.fft(audio_filtered, n_fft)
    freq_axis = np.fft.fftfreq(n_fft, d=1/fs_wav)
    half_n_fft = n_fft // 2
    positive_freq_axis = freq_axis[:half_n_fft]
    db_fft_original = 20 * np.log10(np.abs(fft_original[:half_n_fft]))
    db_fft_filtered = 20 * np.log10(np.abs(fft_filtered[:half_n_fft]))

    # --- Gerar os Gráficos Comparativos ---
    plt.style.use('seaborn-v0_8-whitegrid')
    
    fig = plt.figure(figsize=(16, 12))
    gs = gridspec.GridSpec(3, 2, figure=fig)
    fig.suptitle(f'Análise do Filtro Passa-Faixa ({filter.LOWCUT}-{filter.HIGHCUT} Hz) no arquivo: {wav_filepath.split("/")[-1]}', fontsize=16)

    ax_orig_time = fig.add_subplot(gs[0, 0])
    ax_filt_time = fig.add_subplot(gs[0, 1])
    ax_orig_freq = fig.add_subplot(gs[1, 0])
    ax_filt_freq = fig.add_subplot(gs[1, 1])
    ax_filt_resp_full = fig.add_subplot(gs[2, 0])
    ax_filt_resp_zoom = fig.add_subplot(gs[2, 1])

    # Gráficos de Tempo e Frequência do Sinal
    ax_orig_time.plot(time_axis, audio_data, label='Sinal Original', color='blue', alpha=0.8)
    ax_orig_time.set_title('Sinal Original (Domínio do Tempo)')
    ax_orig_time.set_xlabel('Tempo (s)'); ax_orig_time.set_ylabel('Amplitude'); ax_orig_time.legend()

    ax_filt_time.plot(time_axis, audio_filtered, label='Sinal Filtrado', color='green', alpha=0.8)
    ax_filt_time.set_title('Sinal Filtrado (Domínio do Tempo)')
    ax_filt_time.set_xlabel('Tempo (s)'); ax_filt_time.legend()

    ax_orig_freq.plot(positive_freq_axis, db_fft_original, label='Espectro Original', color='blue')
    ax_orig_freq.set_title('Espectro do Sinal Original (FFT)')
    ax_orig_freq.set_xlabel('Frequência (Hz)'); ax_orig_freq.set_ylabel('Magnitude (dB)'); ax_orig_freq.legend()
    y_lim_freq = (np.min(db_fft_original) - 10, np.max(db_fft_original) + 5)
    ax_orig_freq.set_ylim(y_lim_freq)

    ax_filt_freq.plot(positive_freq_axis, db_fft_filtered, label='Espectro Filtrado', color='green')
    ax_filt_freq.set_title('Espectro do Sinal Filtrado (FFT)')
    ax_filt_freq.set_xlabel('Frequência (Hz)'); ax_filt_freq.legend()
    ax_filt_freq.set_ylim(y_lim_freq)
    
    # Gráfico 6 e 7: Resposta de Frequência do Filtro (Completa e Zoom)
    w, h = signal.sosfreqz(filter.SOS_COEF, worN=16384, fs=filter.FS_FILTER)
    db_h = 20 * np.log10(np.abs(h))
    
    # Gráfico Completo
    ax_filt_resp_full.plot(w, db_h, label=f'Filtro {filter.FILTER_TYPE.capitalize()}', color='purple')
    ax_filt_resp_full.set_title('Resposta de Frequência do Filtro (Completa)')
    ax_filt_resp_full.set_xlabel('Frequência (Hz)'); ax_filt_resp_full.set_ylabel('Ganho (dB)')
    ax_filt_resp_full.axvline(filter.LOWCUT, color='orange', linestyle='--', alpha=0.7, label=f'Corte Inferior ({filter.LOWCUT} Hz)')
    ax_filt_resp_full.axvline(filter.HIGHCUT, color='red', linestyle='--', alpha=0.7, label=f'Corte Superior ({filter.HIGHCUT} Hz)')
    ax_filt_resp_full.set_ylim(-80, 5)
    ax_filt_resp_full.set_xlim(0, filter.FS_FILTER / 2)
    ax_filt_resp_full.legend()

    # Gráfico com Zoom na Frequência Baixa
    ax_filt_resp_zoom.plot(w, db_h, label=f'Filtro {filter.FILTER_TYPE.capitalize()}', color='purple')
    ax_filt_resp_zoom.set_title('Resposta de Frequência (Zoom < 500 Hz)')
    ax_filt_resp_zoom.set_xlabel('Frequência (Hz)')
    ax_filt_resp_zoom.axvline(filter.LOWCUT, color='orange', linestyle='--', alpha=0.7, label=f'Corte Inferior ({filter.LOWCUT} Hz)')
    ax_filt_resp_zoom.set_xlim(0, 500)
    ax_filt_resp_zoom.set_ylim(-80, 5)
    ax_filt_resp_zoom.grid(True, which='both')
    ax_filt_resp_zoom.legend()
    
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    plt.show()


if __name__ == '__main__':
    # Peça ao usuário para fornecer o caminho do arquivo .wav
    # Use áudios gravados pelo seu setup para ter uma representação fiel.
    # Exemplo de caminho: "C:/Users/SeuNome/Documentos/TCC/audios/furadeira_saudavel.wav"
    # Ou simplesmente: "furadeira_saudavel.wav" se estiver na mesma pasta do script.
    
    print("Este script carrega um arquivo .wav, aplica um filtro e exibe os resultados.")
    file_path = input("\nPor favor, insira o caminho para o arquivo .wav (com amostragem de 48000 Hz): ")
    
    g_filtro = GeradorFiltroPassaFaixa()
    filtro = g_filtro.projetar_filtro()
    print(f"Tipo de filtro configurado: {filtro.FILTER_TYPE.capitalize()}")

    visualizar_graficos_do_filtro(filtro, file_path,)
