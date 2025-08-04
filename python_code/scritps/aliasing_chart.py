import numpy as np
import matplotlib.pyplot as plt
from numpy.fft import fft, fftfreq

# --- 1. Parâmetros do Sinal ---
f_sinal = 10    # Frequência do sinal original em Hz
t_max = 1.0     # Duração da simulação em segundos

# --- 2. Sinal Analógico (alta resolução para visualização) ---
fs_analogico = 1000
t_analogico = np.linspace(0, t_max, int(fs_analogico * t_max), endpoint=False)
sinal_analogico = np.sin(2 * np.pi * f_sinal * t_analogico)

# --- 3. Amostragem Incorreta (Aliasing) ---
fs_aliasing = 12  # Viola o Teorema de Nyquist (fs < 2 * f_sinal)
t_aliasing = np.arange(0, t_max, 1/fs_aliasing)
sinal_aliasing = np.sin(2 * np.pi * f_sinal * t_aliasing)
f_fantasma = abs(fs_aliasing - f_sinal) # Frequência "fantasma" de 2 Hz
sinal_fantasma = np.sin(2 * np.pi * f_fantasma * t_analogico)

# --- 4. Amostragem Correta (Respeitando Nyquist) ---
fs_correta = 30 # Respeita o Teorema de Nyquist (fs > 2 * f_sinal)
t_correta = np.arange(0, t_max, 1/fs_correta)
sinal_correto = np.sin(2 * np.pi * f_sinal * t_correta)

# --- 5. Análise no Domínio da Frequência (FFT) ---

# FFT do sinal com aliasing
fft_aliasing = fft(sinal_aliasing)
freqs_aliasing = fftfreq(len(sinal_aliasing), 1/fs_aliasing)
mask_aliasing = freqs_aliasing >= 0
fft_aliasing_plot = (2/len(sinal_aliasing)) * np.abs(fft_aliasing[mask_aliasing])
freqs_aliasing_plot = freqs_aliasing[mask_aliasing]

# FFT do sinal amostrado corretamente
fft_correto = fft(sinal_correto)
freqs_correto = fftfreq(len(sinal_correto), 1/fs_correta)
mask_correto = freqs_correto >= 0
fft_correto_plot = (2/len(sinal_correto)) * np.abs(fft_correto[mask_correto])
freqs_correto_plot = freqs_correto[mask_correto]


# --- 6. Geração dos Gráficos Comparativos ---
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 12))
fig.suptitle('Comparativo: Respeitando vs. Violando o Teorema de Nyquist-Shannon', fontsize=22)

# --- Subplot 1: Domínio do Tempo ---
ax1.plot(t_analogico, sinal_analogico, label=f'Sinal Original ({f_sinal} Hz)', color='blue', linewidth=1.5, alpha=0.7)
ax1.plot(t_aliasing, sinal_aliasing, 'ro', label=f'Amostragem Incorreta ({fs_aliasing} Hz)', markersize=10, markeredgewidth=2.5)
ax1.plot(t_correta, sinal_correto, 'gx', label=f'Amostragem Correta ({fs_correta} Hz)', markersize=10, markeredgewidth=2.5)
ax1.plot(t_analogico, sinal_fantasma, color='red', linestyle='--', linewidth=2, label=f'Sinal "Fantasma" ({f_fantasma} Hz)')

ax1.set_title('Domínio do Tempo', fontsize=18)
ax1.set_xlabel('Tempo (s)', fontsize=18)
ax1.set_ylabel('Amplitude', fontsize=18)
ax1.legend(fontsize=14)
ax1.grid(True)
ax1.tick_params(axis='both', which='major', labelsize=14)

# --- Subplot 2: Domínio da Frequência ---
# Espectro do sinal com aliasing
markerline_alias, stemlines_alias, _ = ax2.stem(
    freqs_aliasing_plot, fft_aliasing_plot,
    linefmt='r-', markerfmt='ro', basefmt=' ',
    label=f'Amostragem Incorreta (Pico em {f_fantasma:.0f} Hz)'
)
# Deixando ambos os marcadores e linhas com a mesma espessura
plt.setp(stemlines_alias, 'linewidth', 3)
plt.setp(markerline_alias, 'markersize', 10, 'markeredgewidth', 2.5)


# Espectro do sinal amostrado corretamente
markerline_corr, stemlines_corr, _ = ax2.stem(
    freqs_correto_plot, fft_correto_plot,
    linefmt='g-', markerfmt='gx', basefmt=' ',
    label=f'Amostragem Correta (Pico em {f_sinal} Hz)'
)

# Deixando ambos os marcadores e linhas com a mesma espessura
plt.setp(stemlines_corr, 'linewidth', 3)
plt.setp(markerline_corr, 'markersize', 10, 'markeredgewidth', 2.5)


ax2.set_title('Domínio da Frequência (Espectro)', fontsize=18)
ax2.set_xlabel('Frequência (Hz)', fontsize=18)
ax2.set_ylabel('Magnitude', fontsize=18)
ax2.set_xlim(0, 15)
ax2.set_ylim(0, 1.2)
ax2.legend(fontsize=14)
ax2.grid(True)
ax2.tick_params(axis='both', which='major', labelsize=14)

# Ajusta o layout e salva
plt.tight_layout(rect=[0, 0.03, 1, 0.95])
plt.savefig('efeito_aliasing_comparativo_final.pdf')

print("Gráfico 'efeito_aliasing_comparativo_final.pdf' gerado com sucesso.")