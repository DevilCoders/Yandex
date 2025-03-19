import json
import os

import numpy as np
import pandas as pd

from tools.audio import AudioSample
from tools.data import read_mp3, hash_id_from_raw_audio
from tools.data_interface.recordings import Recording


def read_public_speech(path_to_pool, manifest_file_name):
    path_to_manifest = os.path.join(path_to_pool, manifest_file_name)
    df = pd.read_csv(path_to_manifest)
    df.columns = ['audio', 'text', 'duration']

    samples = []
    for line in df.values:
        audio_path = line[0]
        text_path = line[1]

        audio_path = audio_path.replace('radio-v4', 'radio-v4_mp3')
        audio_path = audio_path.replace('public_speech', 'public_speech_mp3').replace('wav', 'mp3')
        audio = AudioSample(data=read_mp3(os.path.join(path_to_pool, audio_path)), sample_rate=16000)

        text_path = text_path.replace('radio-v4', 'radio-v4_mp3').replace('public_speech', 'public_speech_mp3')
        with open(os.path.join(path_to_pool, text_path), 'r') as f:
            text = f.read()

        sample_id = hash_id_from_raw_audio(audio.data)
        samples.append(Recording(path_to_pool, sample_id=sample_id, audio=audio, text=text))

    return samples


def read_multispeaker_dataset(path_to_dataset, mel_audio_reader, only_speakers):
    for i, file in enumerate(os.listdir(path_to_dataset)):
        if file.endswith(".json"):
            file_name = file[:-5]
            with open(os.path.join(path_to_dataset, file), 'r') as fp:
                sample_description = json.load(fp)

            print(i, file_name, end='\r')
            utterance = sample_description['utterance']
            speaker = sample_description['speaker']
            text = sample_description['text']
            sample_id = sample_description['ID']

            if speaker not in only_speakers:
                continue

            audio = mel_audio_reader(os.path.join(path_to_dataset, file_name + '.mel.npy'))
            sample = Recording(path_to_dataset, sample_id, text, audio, speaker_id=speaker)

            yield sample


def get_vocode_reader_from_mel(model):
    def vocode_reader(path):
        mel = np.load(path)
        wave = model.vocode(mel)
        return AudioSample(data=wave, sample_rate=model.MODEL_SAMPLE_RATE)
    return vocode_reader
