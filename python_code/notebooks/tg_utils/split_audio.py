import wave
import math
import argparse
import os

def split_audio(input_file, output_file1=None, output_file2=None):
    # Gerar nomes padrão de saída se não fornecidos
    if output_file1 is None or output_file2 is None:
        base_name = os.path.splitext(input_file)[0]
        if output_file1 is None:
            output_file1 = f"{base_name}_80.wav"
        if output_file2 is None:
            output_file2 = f"{base_name}_20.wav"
    
    # Abrir o arquivo de entrada
    with wave.open(input_file, 'rb') as wav_in:
        params = wav_in.getparams()
        n_frames = wav_in.getnframes()
        frames = wav_in.readframes(n_frames)
    
    # Calcular divisão dos frames (80% e 20%)
    split_frame = math.floor(n_frames * 0.8)
    frame_size = params.sampwidth * params.nchannels
    
    # Dividir frames de áudio
    frames_80 = frames[:split_frame * frame_size]
    frames_20 = frames[split_frame * frame_size:]
    
    # Salvar parte de 80%
    with wave.open(output_file1, 'wb') as wav_out:
        wav_out.setparams(params)
        wav_out.setnframes(split_frame)
        wav_out.writeframes(frames_80)
    
    # Salvar parte de 20%
    with wave.open(output_file2, 'wb') as wav_out:
        wav_out.setparams(params)
        wav_out.setnframes(n_frames - split_frame)
        wav_out.writeframes(frames_20)
    
    return output_file1, output_file2

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Divide um arquivo WAV em dois (80% e 20% da duração)')
    parser.add_argument('input_file', type=str, help='Caminho do arquivo WAV de entrada')
    parser.add_argument('--output80', type=str, help='Caminho do arquivo de saída (80%)', default=None)
    parser.add_argument('--output20', type=str, help='Caminho do arquivo de saída (20%)', default=None)
    
    args = parser.parse_args()
    
    if not os.path.isfile(args.input_file):
        raise FileNotFoundError(f"Arquivo de entrada não encontrado: {args.input_file}")
    
    out80, out20 = split_audio(
        args.input_file,
        args.output80,
        args.output20
    )
    
    print(f"Arquivo dividido com sucesso:")
    print(f"• Parte 80%: {out80}")
    print(f"• Parte 20%: {out20}")
