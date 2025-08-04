import matplotlib.pyplot as plt
import numpy as np

# Data for the table based on the WAV header structure
header_data = [
    # RIFF Chunk Descriptor
    ['0', '4', 'ChunkID ("RIFF")', '#d4eaf7'],
    ['4', '4', 'ChunkSize', '#d4eaf7'],
    ['8', '4', 'Format ("WAVE")', '#d4eaf7'],
    # "fmt " sub-chunk
    ['12', '4', 'Subchunk1ID ("fmt ")', '#fce8b2'],
    ['16', '4', 'Subchunk1Size', '#fce8b2'],
    ['20', '2', 'AudioFormat', '#fce8b2'],
    ['22', '2', 'NumChannels', '#fce8b2'],
    ['24', '4', 'SampleRate', '#fce8b2'],
    ['28', '4', 'ByteRate', '#fce8b2'],
    ['32', '2', 'BlockAlign', '#fce8b2'],
    ['34', '2', 'BitsPerSample', '#fce8b2'],
    # "data" sub-chunk
    ['36', '4', 'Subchunk2ID ("data")', '#d5f5e3'],
    ['40', '4', 'Subchunk2Size', '#d5f5e3'],
    ['44', '...', 'Dados de Áudio (PCM)...', '#ffffff']
]

# Extracting data for plotting
offsets = [row[0] for row in header_data]
sizes = [row[1] for row in header_data]
names = [row[2] for row in header_data]
colors = [row[3] for row in header_data]

# Create figure
fig, ax = plt.subplots(figsize=(10, 6))
ax.axis('off')

# Table
table_data = [[offset, size, name] for offset, size, name in zip(offsets, sizes, names)]
col_labels = ["Offset (Bytes)", "Tamanho (Bytes)", "Nome do Campo"]
the_table = plt.table(cellText=table_data,
                      colLabels=col_labels,
                      cellLoc='center',
                      loc='center',
                      colWidths=[0.2, 0.2, 0.6])

# Style the table
the_table.auto_set_font_size(False)
the_table.set_fontsize(12)
the_table.scale(1, 1.5)

# Style cells
cells = the_table.get_celld()
for i in range(len(table_data) + 1):
    for j in range(3):
        cells[(i, j)].set_edgecolor('gray')
        if i == 0:
            cells[(i, j)].set_text_props(weight='bold')
            cells[(i, j)].set_facecolor('#a9cce3')
        else:
            cells[(i, j)].set_facecolor(colors[i-1])

# Title
plt.title("Estrutura do Cabeçalho de um Arquivo WAV (44 bytes)", fontsize=20, weight='bold', pad=20)
plt.tight_layout()

# Save the image
plt.savefig("wav_header.pdf")