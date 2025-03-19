import json
from typing import List
from os.path import join
import numpy as np
from tqdm import tqdm

from tools.audio import AudioSample
from tools.data import hash_id_from_raw_audio


class Recording:
    def __init__(
            self,
            origin_pool,
            sample_id,
            text,
            audio,
            speaker_id=None,
            is_template=False,
            template_id=None,
            accented_text=None,
            meta_info: dict = None
    ):
        if meta_info is None:
            meta_info = {}
        self.origin_pool = origin_pool
        self.sample_id = sample_id
        self.text = text
        self.audio = audio
        self.speaker_id = speaker_id
        self.is_template = is_template
        self.template_id = template_id
        self.accented_text = accented_text
        self.meta_info = meta_info

    def to_dict(self):
        result = {
            'origin_pool': self.origin_pool,
            'sample_id': self.sample_id,
            'text': self.text,
            'audio': self.audio,
            'speaker_id': self.speaker_id,
            'accented_text': self.accented_text,
            'is_template': self.is_template,
            'template_id': self.template_id
        }
        if len(self.meta_info) != 0:
            result['meta_info'] = self.get_str_meta_info()
        return result

    def get_str_meta_info(self):
        return json.dumps(self.meta_info)


def save_samples(path, recordings: List[Recording]):
    descriptions = []

    for recording in tqdm(recordings):
        audio_path = join(path, recording.sample_id + '.wav')
        recording.audio.save(audio_path)

        description = recording.to_dict()
        description['audio'] = audio_path
        descriptions.append(description)

    with open(join(path, 'description.json'), 'w') as fp:
        json.dump(descriptions, fp, indent=4)


def read_recordings(path_to_dataset):
    with open(join(path_to_dataset, 'description.json'), 'r') as f:
        description = json.load(f)

    common_duration = 0
    samples = []
    for description_sample in description:
        audio = AudioSample(file=description_sample['audio'])
        text = description_sample['text']
        speaker_id = description_sample['speaker_id']

        if np.min(audio.data) == np.max(audio.data):
            print(text)
            print('filtered, cause of zero sound')
            continue

        if 'sample_id' in description_sample:
            sample_id = description_sample['sample_id']
        else:
            sample_id = hash_id_from_raw_audio(audio.data)

        common_duration += audio.duration

        #         to check consistency
        #         if not description_sample['is_template']:
        #             if description_sample['template_id'] == 33:
        #             print(text)
        #             audio.play()

        accented_text = None
        if 'accented_text' in description_sample:
            accented_text = description_sample['accented_text']

        template_id = None
        if description_sample['is_template']:
            template_id = description_sample['template_id']

        sample = Recording(
            path_to_dataset,
            sample_id,
            text,
            audio,
            speaker_id=speaker_id,
            is_template=description_sample['is_template'],
            template_id=template_id,
            accented_text=accented_text
        )

        samples.append(sample)

    print(f'Common duration {common_duration}')

    return samples
