import json
from os import makedirs, listdir
from os.path import join, exists
from typing import List

import numpy as np
from collections import namedtuple
from tqdm import tqdm

from ytreader import YTTableParallelReader

from contrib.tts.tacotron.text_processing import utterance_to_symbols
from tools.data import filter_holdout_templates, train_val_split, read_yson
from tools.data_interface.recordings import Recording, read_recordings


class TrainingSample(Recording):
    Description = namedtuple(
        'TrainingSampleDescription',
        field_names=[
            'ID',
            'sequence',
            'speaker',
            'split',
            'text',
            'utterance'
        ]
    )

    def __init__(self, recording, mel_npy, sequence, split, utterance):
        super(TrainingSample, self).__init__(**(recording.to_dict()))
        self.recording = recording
        self.mel_npy = mel_npy
        self.sequence = sequence
        self.split = split
        self.utterance = utterance

    def to_dict(self):
        result = self.recording.to_dict()
        result.update({
            'mel_npy': self.mel_npy,
            'sequence': self.sequence,
            'split': self.split,
            'utterance': self.utterance
        })
        return result

    @property
    def description(self):
        return TrainingSample.Description(
            self.sample_id,
            self.sequence,
            self.speaker_id,
            self.split,
            self.text,
            self.utterance
        )

    def save_for_learning(self, path_to_directory):
        with open(join(path_to_directory, self.sample_id + '.json'), 'w') as f:
            json.dump(self.description._asdict(), f)
        with open(join(path_to_directory, self.sample_id + '.mel.npy'), 'wb') as f:
            f.write(self.mel_npy)


def save_train_samples(samples, path_to_directory):
    if not exists(path_to_directory):
        makedirs(path_to_directory)

    for sample in samples:
        sample.save_for_learning(path_to_directory)


def read_train_samples(path, origin_pool):
    recordings = read_recordings(origin_pool)
    id_to_recording = {r.sample_id: r for r in recordings}
    all_ids = set(id_to_recording.keys())

    samples = []
    for file in listdir(path):
        if not file.endswith('.json'):
            continue

        sample_id = file[:-5]

        mel_file = join(path, sample_id + '.mel.npy')
        mel_npy = np.load(mel_file)

        with open(join(path, file), 'r') as f:
            sample_description = json.load(f)

        recording = id_to_recording[sample_id]
        all_ids.remove(sample_id)

        sample = TrainingSample(
            recording,
            mel_npy,
            sample_description['sequence'],
            sample_description['split'],
            sample_description['utterance']
        )
        samples.append(sample)

    assert len(all_ids) == 0
    return samples


def create_training_samples(recordings, yt_preprocessed_table, path_to_save):
    training_samples = read_processed_pool(
        yt_preprocessed_table,
        recordings
    )

    training_samples = filter_holdout_templates(training_samples, 0.33)
    training_samples = train_val_split(training_samples, 0.33)
    save_train_samples(training_samples, path_to_save)

    return training_samples


def read_processed_pool(yt_table, recordings: List[Recording]) -> List[TrainingSample]:
    reader = YTTableParallelReader("hahn", yt_table, cache_size=128, num_readers=1)
    samples = []
    id_to_recording = {recording.sample_id: recording for recording in recordings}

    for line in tqdm(reader):
        sample_id = str(line[b'ID'], encoding='utf-8')
        utterance = read_yson(line[b'utterance'])

        sample = TrainingSample(
            recording=id_to_recording[sample_id],
            mel_npy=line[b'mel__npy'],
            sequence=utterance_to_symbols(utterance),
            split=None,
            utterance=utterance
        )
        samples.append(sample)

    return samples
