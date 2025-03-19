import array
import logging
import os
import tempfile
import hashlib
import re
from copy import deepcopy
from io import BytesIO
from typing import List

import numpy as np
import sox
from scipy.io import wavfile

from tqdm import tqdm

from pydub import AudioSegment
from pydub.utils import get_array_type

from tools.audio import AudioSample
from tools.normalize import normalize


ALLOWED_SYMBOLS_SET = re.compile(r'[а-яА-ЯёЁ,.:\-;!? ()+"]*')


def check_allowed_symbols(text):
    if ALLOWED_SYMBOLS_SET.fullmatch(text) is None:
        for a in text:
            if ALLOWED_SYMBOLS_SET.fullmatch(a) is None:
                print(f'"{a}" = {ord(a)} is bad symbol')
                return False
    return True


def hash_id_from_raw_audio(data):
    sample_id = hashlib.sha256(data).hexdigest()[:32]
    id_regex = re.compile(r'[0-9a-f]{32}')
    assert id_regex.fullmatch(sample_id)
    return sample_id


def read_mp3(path):
    sound = AudioSegment.from_file(file=path)
    left = sound.split_to_mono()[0]

    bit_depth = left.sample_width * 8
    array_type = get_array_type(bit_depth)

    numeric_array = array.array(array_type, left._data)
    wave = np.array(numeric_array)
    wave = wave / np.max(wave)
    return wave


def group_by(values, func):
    groups = {}
    for value in values:
        group_id = func(value)
        if group_id not in groups:
            groups[group_id] = [value]
        else:
            groups[group_id].append(value)

    return groups


def bootstrap(full_samples, ratio):
    samples = []

    for i in range(len(full_samples)):
        if np.random.uniform(0, 1) < ratio:
            samples.append(full_samples[i])

    return samples


def sample_sox(audios: List[AudioSample], sample_rate):
    sox.logger.setLevel(logging.ERROR)
    tfm = sox.Transformer()
    tfm.rate(sample_rate, quality='h')

    new_audios = []

    with tempfile.TemporaryDirectory() as tmp:
        original_audio_file = os.path.join(tmp, 'original.wav')
        sampled_audio_file = os.path.join(tmp, 'sampled.wav')

        for i, audio in enumerate(audios):
            print(f'{i}/{len(audios) - 1}', end='\r')

            if audio.sample_rate != sample_rate:
                wavfile.write(original_audio_file, data=audio.data, rate=audio.sample_rate)
                tfm.build(original_audio_file, sampled_audio_file)
                new_audio = AudioSample(file=sampled_audio_file)
                new_audios.append(new_audio)
            else:
                new_audios.append(audio)

    return new_audios


def sample_scipy(audios: List[AudioSample], sample_rate):
    new_audios = []
    for audio in audios:
        new_audio = AudioSample(data=audio.data.copy(), sample_rate=audio.sample_rate)
        if new_audio.sample_rate != sample_rate:
            new_audio.resample(sample_rate)
        new_audios.append(new_audio)
    return new_audios


def lowercase_all_text(texts: List[str]):
    return list(map(lambda t: t.lower() if t is not None else None, texts))


def filter_holdout_templates(samples, percent_per_group):
    group_size = {}

    for sample in samples:
        if sample.is_template:
            if sample.template_id not in group_size:
                group_size[sample.template_id] = 0
            group_size[sample.template_id] += 1

    num_groups = len(group_size)
    group_size = [group_size[group_id] for group_id in range(num_groups)]

    group_percent = list(map(lambda x: round(x * percent_per_group), group_size))
    counters = [0 for _ in range(num_groups)]
    remaining_samples = [
        set(np.random.choice(g_size, choice_size, replace=False))
        for g_size, choice_size in zip(group_size, group_percent)
    ]

    real_percent = [0 for _ in range(num_groups)]

    new_samples = []
    for sample in samples:
        if not sample.is_template:
            new_samples.append(sample)
        else:
            sample_copy = deepcopy(sample)
            if counters[sample.template_id] in remaining_samples[sample.template_id]:
                sample_copy.split = 'train'
                real_percent[sample.template_id] += 1
            else:
                sample_copy.split = 'eval'

            new_samples.append(sample_copy)
            counters[sample.template_id] += 1

    # to check real percents
    # real_percent = [round(r_size / g_size, 3) for r_size, g_size in zip(real_percent, group_size)]
    # print(real_percent)
    return new_samples


def train_val_split(samples, percent_validate):
    new_samples = []
    for sample in tqdm(samples):
        new_sample = deepcopy(sample)
        if not sample.is_template:
            split = 'train' if np.random.uniform() > percent_validate else 'eval'
            new_sample.split = split

        new_samples.append(new_sample)

    return new_samples


def normalize_volume(audios: List[AudioSample]):
    new_audios = []

    for i, audio in enumerate(audios):
        new_audio = normalize(audio)
        new_audios.append(new_audio)

    return new_audios


def raw_to_wav(audio_sample):
    buffer = BytesIO()
    wavfile.write(buffer, audio_sample.sample_rate, audio_sample.data)
    return buffer.read()


def bytes_to_str(obj):
    if isinstance(obj, bytes):
        return str(obj, encoding='utf-8')
    else:
        return obj


def read_yson(obj):
    new_obj = {}
    if isinstance(obj, dict):
        for key, value in obj.items():
            new_obj[bytes_to_str(key)] = read_yson(value)
    elif isinstance(obj, list):
        new_obj = []
        for value in obj:
            new_obj.append(read_yson(value))
    else:
        return bytes_to_str(obj)

    return new_obj
