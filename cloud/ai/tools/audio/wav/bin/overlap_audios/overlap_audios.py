import nirvana.job_context as nv
import os
import scipy
import numpy as np
from tqdm import tqdm
from scipy.io import wavfile
import zipfile
import glob
import librosa
import json


class NoiseAgumentator:
    def __init__(self, dir_noises):
        self.noises = [
            (file_path.rsplit('/', 1)[1].split('.')[0], file_path)
            for file_path in glob.glob(dir_noises + '/*.wav')]

    def make_noisy(self, audio_path, weight, count_noises):
        audio_sr, audio = wavfile.read(audio_path)
        audio = normalize_audio(audio)

        i = 0
        added_noises_idx = []  # какие шумы были наложены
        noised_audio = audio
        while i < count_noises:
            noise_idx, noise_path = self.noises[np.random.randint(0, len(self.noises) - 1)]  # случайный шум
            noise, _ = librosa.load(noise_path, sr=audio_sr)
            noise_audio = normalize_audio(noise)
            if len(audio) < len(noise_audio):
                noise_audio = noise_audio[:len(audio)]
            else:
                noise_audio = np.pad(noise_audio, (0, len(audio) - len(noise_audio)))
            added_noises_idx.append(noise_idx)

            noised_audio = (1.0 - weight) * noised_audio + weight * noise_audio
            i += 1
        return normalize_audio(noised_audio), added_noises_idx, audio_sr


def normalize_audio(audio):
    max_amp = max(abs(audio))
    audio = audio * 0.73 / max_amp
    audio = audio * 32768
    return audio.astype(np.int16)


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()

    audio_archive_path = inputs.get('audios.zip')

    noised_path = outputs.get("noised.zip")
    noises_info_path = outputs.get("noises_info.json")

    weight = parameters.get('weight')
    overlap_size = parameters.get('overlap_size')

    data_dir = os.getcwd()

    # папка с аудио
    with zipfile.ZipFile(audio_archive_path, "r") as zip_ref:
        zip_ref.extractall(os.path.join(data_dir, 'audios'))

    print("unzip audios")

    # в качестве шумов используем эти же аудио
    dir_noises = os.path.join(data_dir, 'audios')
    aug = NoiseAgumentator(dir_noises)

    noises_info = []
    audios = glob.glob(os.path.join(data_dir, f'audios/*.wav'))
    print(audios)

    os.mkdir("noised")
    for audio_path in tqdm(audios, position=0, leave=False):

        audio_noised, noises_idx, sr = aug.make_noisy(audio_path, weight, overlap_size)
        dir_path, audio_name = audio_path.split("audios/")
        audio_noised_path = f'{dir_path}noised/{audio_name}'
        scipy.io.wavfile.write(audio_noised_path, sr, audio_noised)
        # audio_name, noises
        noises_info.append((audio_path, noises_idx))
    print(noises_info)

    # запись всех зашумленных записей
    with zipfile.ZipFile(noised_path, "w") as zip_ref:
        for root, dirs, files in os.walk('noised'):  # Список всех файлов и папок в директории noised
            for file in files:
                zip_ref.write(os.path.join(root, file))

    # сохранение информации о том, какие записи были наложены поверх
    with open(noises_info_path, 'w') as f:
        result = []
        for item in noises_info:
            result.append({
                "audio": item[0],
                "overlap": item[1]
            })
        json.dump(result, f, indent=4)


if __name__ == '__main__':
    main()
