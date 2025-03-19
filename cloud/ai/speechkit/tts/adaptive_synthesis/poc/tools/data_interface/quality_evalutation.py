import os
import uuid
import pandas as pd
from collections import namedtuple
from typing import List

from tools.data_interface.synthesis import SynthesisSample
from tools.audio import AudioSample
from tools.synthesizer import create_text_from_chunks_and_reference
from tools.chunk import read_chunk_sequence


class PairType:
    REF_TO_REF = 'REF_TO_REF'
    REF_TO_SYN = 'REF_TO_SYN'
    SYN_TO_SYN = 'SYN_TO_SYN'


class PhraseCombination:
    SAME = 'SAME'
    DIFFERENT = 'DIFFERENT'


QualityEvaluationSampleBase = namedtuple(
    'QualityEvaluationSampleBase',
    field_names=[
        'sample_id',
        'origin_pool',
        'path_to_reference',
        'path_to_synthesis',
        'speaker_id',
        'data_part',
        'is_template',
        'template_id',
        'is_distorted',
        'distortion_type',
        'pair_type',
        'phrase_combination',
        'url_reference',
        'url_synthesis',
        'golden_mark'
    ],
    defaults=[
        False,                   # is_distorted
        None,                    # distortion_type
        PairType.REF_TO_SYN,     # pair_type
        PhraseCombination.SAME,  # phrase_combination
        None,                    # url_reference
        None,                    # url_synthesis
        None                     # golden_mark
    ]
)


class QualityEvaluationSample(QualityEvaluationSampleBase):
    @staticmethod
    def new_id():
        return str(uuid.uuid4())

    def set_urls_with_copy(self, url_reference, url_synthesis):
        return self._replace(
            sample_id=QualityEvaluationSample.new_id(),
            url_reference=url_reference,
            url_synthesis=url_synthesis
        )

    def set_distorted_with_copy(self, distorted_audio: AudioSample, distortion_type):
        path_to_distorted = self.path_to_synthesis
        path_to_distorted = path_to_distorted.replace('.wav', f'_{distortion_type}.wav')
        distorted_audio.save(path_to_distorted)
        return self._replace(
            sample_id=QualityEvaluationSample.new_id(),
            path_to_synthesis=path_to_distorted,
            is_distorted=True,
            distortion_type=distortion_type,
            golden_mark=False
        )

    def set_phrase_combination_with_copy(self, path_to_synthesis, phrase_combination):
        return self._replace(
            sample_id=QualityEvaluationSample.new_id(),
            path_to_synthesis=path_to_synthesis,
            phrase_combination=phrase_combination
        )

    def set_pair_type_with_copy(self, path_to_synthesis, pair_type):
        return self._replace(
            sample_id=QualityEvaluationSample.new_id(),
            path_to_synthesis=path_to_synthesis,
            pair_type=pair_type
        )

    def as_dict(self):
        return self._asdict()


class QualityEvaluationPool:
    def __init__(self, samples: List[QualityEvaluationSample]):
        self.samples = samples

    @property
    def length(self):
        return len(self.samples)

    def get(self, i) -> QualityEvaluationSample:
        return self.samples[i]

    @staticmethod
    def read_from_synthesis_pool(path):
        metric_pool_df = pd.read_csv(os.path.join(path, 'pool_description.tsv'), sep='\t')

        samples = []
        for index, row in metric_pool_df.iterrows():
            if 'synthesis_chunks' in row:
                row['synthesized_text'] = create_text_from_chunks_and_reference(
                    read_chunk_sequence(row['synthesis_chunks']),
                    row['reference_text']
                )
                row['is_template'] = False
                row['template_id'] = None
                row['data_part'] = 'train'
                row['origin_pool'] = path
                row['meta_info'] = None
                del row['synthesis_chunks']

            description = SynthesisSample.Description(**row)
            sample = QualityEvaluationSample(
                sample_id=QualityEvaluationSample.new_id(),
                origin_pool=description.origin_pool,
                path_to_reference=os.path.join(path, description.reference_file),
                path_to_synthesis=os.path.join(path, description.synthesis_file),
                speaker_id=description.speaker,
                data_part=description.data_part,
                is_template=description.is_template,
                template_id=description.template_id
            )
            samples.append(sample)

        return QualityEvaluationPool(samples)

    @staticmethod
    def read_from_tsv(path):
        df = pd.read_csv(path, sep='\t')

        samples = []
        for idx, row in df.iterrows():
            parameters = {field: row[field] for field in QualityEvaluationSample._fields}
            sample = QualityEvaluationSample(**parameters)
            samples.append(sample)

        return QualityEvaluationPool(samples)

    def to_tsv(self, path):
        with open(path, 'w') as pool_file:
            header = [key for key, _ in self.samples[0].as_dict().items()]
            pool_file.write('\t'.join(header) + '\n')

            for sample in self.samples:
                pool_file.write('\t'.join([str(value) for _, value in sample.as_dict().items()]) + '\n')

    def find_sample(self, audio1_url, audio2_url) -> QualityEvaluationSample:
        for sample in self.samples:
            sample_urls = sample.url_reference, sample.url_synthesis
            audio_urls = audio1_url, audio2_url
            if sample_urls == audio_urls or sample_urls == (audio_urls[1], audio_urls[0]):
                return sample
        raise Exception(f'No such sample with {audio1_url} \t {audio2_url}')
