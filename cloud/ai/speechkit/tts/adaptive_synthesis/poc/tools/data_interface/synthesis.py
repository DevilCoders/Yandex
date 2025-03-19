import os
import numpy as np
import pandas as pd
from collections import namedtuple, OrderedDict

from tools.audio import AudioSample
from tools.data_interface.recordings import Recording
from tools.data_interface.train import TrainingSample
from tools.data import hash_id_from_raw_audio


class SynthesisSample(TrainingSample):
    Description = namedtuple(
        'SynthesisSampleDescription',
        [
            'origin_pool',
            'sample_id',
            'speaker',
            'data_part',
            'is_template',
            'template_id',
            'reference_file',
            'reference_text',
            'synthesis_file',
            'synthesized_text',
            'meta_info'
        ]
    )

    def __init__(self, reference_sample: TrainingSample, synthesized_text: str, synthesis_audio: AudioSample):
        super().__init__(
            reference_sample.recording,
            reference_sample.mel_npy,
            reference_sample.sequence,
            reference_sample.split,
            reference_sample.utterance
        )
        self.reference = reference_sample
        self.synthesis_sample_id = hash_id_from_raw_audio(synthesis_audio.data)
        self.synthesized_text = synthesized_text
        self.synthesis_audio = synthesis_audio

    def save_audios(self, path, synthesis_file, reference_file):
        self.synthesis_audio.save(os.path.join(path, synthesis_file))
        self.reference.audio.save(os.path.join(path, reference_file))

        return SynthesisSample.Description(
            origin_pool=self.origin_pool,
            sample_id=self.sample_id,
            speaker=self.speaker_id,
            data_part=self.split,
            is_template=self.is_template,
            template_id=self.template_id,
            reference_file=reference_file,
            reference_text=self.reference.text,
            synthesis_file=synthesis_file,
            synthesized_text=self.synthesized_text,
            meta_info=self.get_str_meta_info()
        )

    @staticmethod
    def read_instance(pool_path, description: Description):
        synthesis_audio = AudioSample(os.path.join(pool_path, description.synthesis_file))
        reference_audio = AudioSample(os.path.join(pool_path, description.reference_file))
        reference = TrainingSample(
            Recording(
                origin_pool=description.origin_pool,
                sample_id=description.sample_id,
                text=description.reference_text,
                audio=reference_audio,
                speaker_id=description.speaker,
                is_template=description.is_template,
                template_id=description.template_id,
                meta_info=description.meta_info
            ),
            mel_npy=None,
            sequence=None,
            split=description.data_part,
            utterance=None
        )

        return SynthesisSample(
            reference,
            synthesized_text=description.synthesized_text,
            synthesis_audio=synthesis_audio
        )

    def write(self, fp, path, synthesis_file, reference_file):
        description = self.save_audios(path, synthesis_file, reference_file)
        print(description)
        fp.write(
            '\t'.join([value for _, value in description._asdict().items()])
        )

    @staticmethod
    def get_field_names():
        return SynthesisSample.Description._fields


def save_synthesized_samples(path, synthesis_samples):
    n_digits = int(round(np.log10(1 + len(synthesis_samples)))) + 1

    pool_description_file_name = f'pool_description.tsv'

    with open(os.path.join(path, pool_description_file_name), 'w') as pool_description_file:
        pool_description_file.write('\t'.join(SynthesisSample.get_field_names()) + '\n')

        for i, sample in enumerate(synthesis_samples):
            file_name = str(i).zfill(n_digits)
            synthesis_file = f'synthesis_{file_name}.wav'
            reference_file = f'reference_{file_name}.wav'

            description = sample.save_audios(
                path,
                synthesis_file,
                reference_file
            )
            pool_description_file.write(
                '\t'.join([str(value) for _, value in description._asdict().items()])
            )

            # sample.write(
            #     pool_description_file,
            #     path,
            #     synthesis_file,
            #     reference_file
            # )
            pool_description_file.write('\n')


def read_synthesized_samples(path):
    pool_description_file_name = f'pool_description.tsv'
    df = pd.read_csv(os.path.join(path, pool_description_file_name), sep='\t')
    samples = []

    for idx, row in df.iterrows():
        description = SynthesisSample.Description(**row)
        sample = SynthesisSample.read_instance(path, description)
        samples.append(sample)

    return samples
