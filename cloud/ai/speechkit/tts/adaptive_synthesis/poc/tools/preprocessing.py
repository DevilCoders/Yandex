import os
from copy import deepcopy
from tqdm import tqdm

import yt.wrapper as yt

from tools.data_interface.recordings import save_samples
from tools.yt_utils import set_yt_config

from tools.data import raw_to_wav, sample_sox, normalize_volume, lowercase_all_text
from tools.audio import PROJECT_SAMPLE_RATE

from tools.data_interface.train import *


def normalize_recordings(samples: List[Recording]) -> List[Recording]:
    audios = list(map(lambda s: s.audio, samples))
    texts = list(map(lambda s: s.text, samples))
    accented_texts = list(map(lambda s: s.accented_text, samples))

    print(f'Sample audio to sample_rate: {PROJECT_SAMPLE_RATE} Hz')
    audios = sample_sox(audios, PROJECT_SAMPLE_RATE)

    print('Normalize volume')
    audios = normalize_volume(audios)

    print('Lowercase all text')
    texts = lowercase_all_text(texts)
    accented_texts = lowercase_all_text(accented_texts)

    new_samples = deepcopy(samples)
    for i in range(len(new_samples)):
        new_samples[i].audio = audios[i]
        new_samples[i].text = texts[i]
        new_samples[i].accented_text = accented_texts[i]

    return new_samples


def create_tsv_for_preprocessing(samples, yt_table):
    set_yt_config(yt.config)
    tsv = []

    for sample in tqdm(samples):
        audio = sample.audio
        assert audio.sample_rate == PROJECT_SAMPLE_RATE

        wav = raw_to_wav(audio)
        tsv_sample = {
            'ID': sample.sample_id,
            'text': sample.text,
            'pcm__wav': wav,
            'duration': audio.duration,
            'accented_text': sample.accented_text
        }

        tsv.append(tsv_sample)

    tsv.sort(key=lambda x: x["ID"])

    schema = [
        {"name": "ID", "type": "utf8", "sort_order": "ascending"},
        {"name": "text", "type": "utf8"},
        {"name": "pcm__wav", "type": "string"},
        {"name": "duration", "type": "double"},
        {"name": "accented_text", "type": "utf8"}
    ]

    print('create table')
    if not yt.exists(yt_table):
        yt.create_table(yt_table, attributes={"schema": schema})
    print('write table')
    yt.write_table(yt_table, tsv)

    return tsv


def preprocess(path_to_recordings, path_to_save, yt_table_path):
    print(f'read_recordings from {path_to_recordings}')
    recordings = read_recordings(path_to_recordings)

    print('normalize_recordings')
    recordings = normalize_recordings(recordings)

    print(f'saving into {path_to_save}')
    if not os.path.exists(path_to_save):
        os.makedirs(path_to_save)
    save_samples(path_to_save, recordings)

    print(f'creating yt table {yt_table_path}')
    create_tsv_for_preprocessing(recordings, yt_table_path)

    return recordings
