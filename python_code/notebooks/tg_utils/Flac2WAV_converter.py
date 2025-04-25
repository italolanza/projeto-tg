import soundfile as sf
import os

# Lista de arquivos a serem processados
input_files = [
    '../misc/Fault-Detection-using-TinyML-Master/Machines Records/M1_H_S1.flac',
    '../misc/Fault-Detection-using-TinyML-Master/Machines Records/M1_F1_S1.flac',
    '../misc/Fault-Detection-using-TinyML-Master/Machines Records/M1_F2_S1.flac',
    '../misc/Fault-Detection-using-TinyML-Master/Machines Records/M1_F3_S1.flac',
    '../misc/Fault-Detection-using-TinyML-Master/Machines Records/M1_OFF_S1.flac'
]

# Configurações
OUTPUT_DIR = "output_wav"  # Pasta para salvar os .wav
os.makedirs(OUTPUT_DIR, exist_ok=True)  # Cria a pasta se não existir

for input_file in input_files:
    try:
        # Lê o arquivo .flac
        data, sample_rate = sf.read(input_file)
        
        # Verifica se há múltiplos canais e seleciona o primeiro
        if len(data.shape) > 1:
            data = data[:, 0]  # Canal 0 (esquerdo)
        
        # Remove os primeiros X samples
        # data = data[19663:]
        
        # Gera o nome do arquivo de saída
        output_file = os.path.join(OUTPUT_DIR, os.path.basename(input_file).replace(".flac", ".wav"))
        
        # Salva em .wav
        sf.write(output_file, data, sample_rate, format="WAV")
        print(f"Arquivo convertido: {output_file}")
    
    except Exception as e:
        print(f"Erro ao processar {input_file}: {str(e)}")