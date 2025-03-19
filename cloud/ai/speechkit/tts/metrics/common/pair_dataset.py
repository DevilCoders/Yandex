import json
import numpy as np
import os
import uuid
import boto3
from distortion import VoiceDistorter, ALL_DISTORTIONS, DistortionType
import random

from audio import AudioSample


def append_before_extension(filename, tag):
    return "{0}_{2}{1}".format(*os.path.splitext(filename) + (tag,))


class PairSample():
    def __init__(self, sample_dict):
        self.uuid = sample_dict.get('uuid', uuid.uuid4())
        self.reference = sample_dict.get('reference', None)
        self.synthesis = sample_dict.get('synthesis', None)
        self.text = sample_dict.get('text', None)
        self.question = sample_dict.get('question', None)
        self.reference_model = sample_dict.get('reference_model', None)
        self.synthesis_model = sample_dict.get('synthesis_model', None)
        self.golden_mark = sample_dict.get('golden_mark', None)
        self.speaker = sample_dict.get('speaker', None)

        self.reference_url = sample_dict.get('reference_url', None)
        self.synthesis_url = sample_dict.get('synthesis_url', None)

    def to_json(self):
        return {'uuid': self.uuid,
                'reference': self.reference,
                'synthesis': self.synthesis,
                'text': self.text,
                'question': self.question,
                'reference_model': self.reference_model,
                'synthesis_model': self.synthesis_model,
                'golden_mark': self.golden_mark,
                'reference_url': self.reference_url,
                'synthesis_url': self.synthesis_url,
                'speaker': self.speaker
                }


class PairDataset():
    def __init__(self, config_path, s3_config):
        self.config_path = config_path
        with open(config_path, "r") as read_file:
            samples = json.load(read_file)

            self.samples = [PairSample(cur_sample) for cur_sample in samples]

        self.sample_id_to_index = {sample.uuid: i for i, sample in enumerate(self.samples)}

        self.s3_config = s3_config
        session = boto3.session.Session()

        self.s3 = session.client(
            service_name='s3',
            endpoint_url=str(self.s3_config['s3_endpoint_url']),
            aws_access_key_id=str(self.s3_config['aws_access_key_id']),
            aws_secret_access_key=str(self.s3_config['aws_secret_access_key'])
        )

    @staticmethod
    def merge_list_of_pair_datasets(config_path, list_pair_datasets, s3_config):
        concat_configs = []

        for cur_pair_dataset in list_pair_datasets:
            with open(cur_pair_dataset.config_path, "r") as read_file:
                cur_samples = json.load(read_file)
            concat_configs += cur_samples

        with open(config_path, "w") as write_file:
            json.dump(concat_configs, write_file)

        return PairDataset(config_path, s3_config)

    def get_sample_by_id(self, sample_id):
        if sample_id not in self.sample_id_to_index:
            raise Exception(f'No such sample with id={sample_id}')
        return self.samples[self.sample_id_to_index[sample_id]]

    def upload_to_s3(self, config_path, s3_bucket, s3_path, thread_pool):
        def save_sample(sample):
            s3_reference_path = os.path.join(s3_path, str(uuid.uuid4()) + '.wav')
            s3_synthesis_path = os.path.join(s3_path, str(uuid.uuid4()) + '.wav')

            urls = {'reference_url': f'{self.s3_config["s3_endpoint_url"]}/{s3_bucket}/{s3_reference_path}',
                    'synthesis_url': f'{self.s3_config["s3_endpoint_url"]}/{s3_bucket}/{s3_synthesis_path}'
                    }

            sample_with_url = PairSample({**sample.to_json(), **urls})

            self.s3.upload_file(sample.reference, s3_bucket, s3_reference_path, ExtraArgs={'ACL':'public-read'})
            self.s3.upload_file(sample.synthesis, s3_bucket, s3_synthesis_path, ExtraArgs={'ACL':'public-read'})

            return sample_with_url.to_json()

        finalized_samples = thread_pool.map(save_sample, self.samples)

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(finalized_samples, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def upload_to_s3_questions(self, config_path, s3_bucket, s3_path, thread_pool):
        def save_sample(sample):
            s3_synthesis_path = os.path.join(s3_path, str(uuid.uuid4()) + '.wav')

            urls = {'synthesis_url': f'{self.s3_config["s3_endpoint_url"]}/{s3_bucket}/{s3_synthesis_path}'
                    }

            sample_with_url = PairSample({**sample.to_json(), **urls})

            self.s3.upload_file(sample.synthesis, s3_bucket, s3_synthesis_path, ExtraArgs={'ACL':'public-read'})

            return sample_with_url.to_json()

        finalized_samples = thread_pool.map(save_sample, self.samples)

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(finalized_samples, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def set_golden_mark(self, config_path, value):
        result = []

        for cur_sample in self.samples:
            new_sample = PairSample({**cur_sample.to_json()})
            new_sample.golden_mark = value
            result.append(new_sample.to_json())

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def set_questions(self, config_path, questions):
        result = []

        for cur_sample in self.samples:
            for question in questions:
                new_sample = PairSample({**cur_sample.to_json()})
                new_sample.question = question
                result.append(new_sample.to_json())

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def limit_dataset(self, config_path, limit):
        if limit >= len(self.samples):
            limit = len(self.samples)
        result = random.sample(self.samples, limit)
        result_json = [cur_sample.to_json() for cur_sample in result]

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(result_json, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def get_tasks_pivr(self, tasks_path, control_tasks_path):
        shuffled_indices = list(range(len(self.samples)))
        np.random.shuffle(shuffled_indices)

        tasks = []
        control_tasks = []
        for sample_idx in shuffled_indices:
            sample = self.samples[sample_idx]

            cur_task = {'audio_1': sample.reference_url, 'audio_2': sample.synthesis_url, "speaker": sample.speaker}

            if sample.golden_mark is not None:
                cur_task['choice'] = "same" if sample.golden_mark else "diff"
                control_tasks.append(cur_task)
            else:
                tasks.append(cur_task)

        with open(tasks_path, 'w', encoding='utf-8') as f:
            json.dump(tasks, f, ensure_ascii=False, indent=4)

        with open(control_tasks_path, 'w', encoding='utf-8') as f:
            json.dump(control_tasks, f, ensure_ascii=False, indent=4)

    def get_tasks_sbs(self, tasks_path, control_tasks_path):
        shuffled_indices = list(range(len(self.samples)))
        np.random.shuffle(shuffled_indices)

        tasks = []
        control_tasks = []
        for sample_idx in shuffled_indices:
            sample = self.samples[sample_idx]

            cur_task = {'audio_a': sample.reference_url, 'audio_b': sample.synthesis_url, "text_hint": sample.text, "speaker": sample.speaker}

            if sample.golden_mark is not None:
                cur_task['choice'] = sample.golden_mark
                control_tasks.append(cur_task)
            else:
                tasks.append(cur_task)

        with open(tasks_path, 'w', encoding='utf-8') as f:
            json.dump(tasks, f, ensure_ascii=False, indent=4)

        with open(control_tasks_path, 'w', encoding='utf-8') as f:
            json.dump(control_tasks, f, ensure_ascii=False, indent=4)

    def get_tasks_questions(self, tasks_path, control_tasks_path):
        shuffled_indices = list(range(len(self.samples)))
        np.random.shuffle(shuffled_indices)

        tasks = []
        control_tasks = []
        for sample_idx in shuffled_indices:
            sample = self.samples[sample_idx]

            cur_task = {'audio': sample.synthesis_url, "text": sample.text, "synthesis_model": sample.synthesis_model, "question": sample.question, "speaker": sample.speaker}

            if sample.golden_mark is not None:
                cur_task['answer'] = sample.golden_mark
                control_tasks.append(cur_task)
            else:
                tasks.append(cur_task)

        with open(tasks_path, 'w', encoding='utf-8') as f:
            json.dump(tasks, f, ensure_ascii=False, indent=4)

        with open(control_tasks_path, 'w', encoding='utf-8') as f:
            json.dump(control_tasks, f, ensure_ascii=False, indent=4)

    def get_tasks_mos(self, tasks_path):
        shuffled_indices = list(range(len(self.samples)))
        np.random.shuffle(shuffled_indices)

        tasks = []
        for sample_idx in shuffled_indices:
            sample = self.samples[sample_idx]

            cur_task = {'audio': sample.synthesis_url, "text": sample.text, "synthesis_model": sample.synthesis_model, "speaker": sample.speaker}
            tasks.append(cur_task)

        with open(tasks_path, 'w', encoding='utf-8') as f:
            json.dump(tasks, f, ensure_ascii=False, indent=4)

    def distort_pivr(self, config_path, new_folder, distortion_types=ALL_DISTORTIONS, ratio_samples_to_distort=1.0):
        if not set(distortion_types).issubset(set(ALL_DISTORTIONS)):
            raise Exception(f'Distortion types {distortion_types} is not a subset of ALL_DISTORTIONS {ALL_DISTORTIONS}')

        num_samples = len(self.samples)
        num_samples_to_distort = round(num_samples * ratio_samples_to_distort)
        ids_distorted = np.random.choice(list(range(num_samples)), size=num_samples_to_distort, replace=False)

        new_samples = []
        group_by_distortion_samples = {}
        for index in range(num_samples):
            if index in ids_distorted:
                distortion_idx = np.random.randint(0, len(distortion_types))
                distortion_type = distortion_types[distortion_idx]

                if distortion_type not in group_by_distortion_samples:
                    group_by_distortion_samples[distortion_type] = {'audios': [], 'samples': []}

                group_by_distortion_samples[distortion_type]['audios'].append(AudioSample(file=self.samples[index].reference))
                group_by_distortion_samples[distortion_type]['samples'].append(self.samples[index])
            else:
                new_samples.append(self.samples[index].to_json())

        for distortion_type, values in group_by_distortion_samples.items():
            audios = values['audios']
            current_samples = values['samples']
            new_audios = VoiceDistorter.apply(distortion_type, audios)

            for new_audio, sample in zip(new_audios, current_samples):
                new_file_path = new_folder + '/' + sample.uuid + '_distorted.wav'
                new_audio.save(new_file_path)
                new_sample = sample
                new_sample.reference = new_file_path
                new_sample.golden_mark = False
                new_samples.append(new_sample.to_json())

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(new_samples, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def distort_sbs(self, config_path, new_folder, distortion_types=ALL_DISTORTIONS, ratio_samples_to_distort=1.0):
        if not set(distortion_types).issubset(set(ALL_DISTORTIONS)):
            raise Exception(f'Distortion types {distortion_types} is not a subset of ALL_DISTORTIONS {ALL_DISTORTIONS}')

        num_samples = len(self.samples)
        num_samples_to_distort = round(num_samples * ratio_samples_to_distort)
        ids_distorted = np.random.choice(list(range(num_samples)), size=num_samples_to_distort, replace=False)

        new_samples = []
        group_by_distortion_samples = {}
        for index in range(num_samples):
            if index in ids_distorted:
                distortion_idx = np.random.randint(0, len(distortion_types))
                distortion_type = distortion_types[distortion_idx]

                if distortion_type not in group_by_distortion_samples:
                    group_by_distortion_samples[distortion_type] = {'audios': [], 'samples': [],
                                                                    'golden_mark': []}
                group_by_distortion_samples[distortion_type]['samples'].append(self.samples[index])
                # if reference is distorted, then b (synthesis) is better than a (reference)
                group_by_distortion_samples[distortion_type]['audios'].append(
                    AudioSample(file=self.samples[index].reference))
                group_by_distortion_samples[distortion_type]['golden_mark'].append('b')
            else:
                new_samples.append(self.samples[index].to_json())

        for distortion_type, values in group_by_distortion_samples.items():
            audios = values['audios']
            current_samples = values['samples']
            golden_mark = values['golden_mark']
            new_audios = VoiceDistorter.apply(distortion_type, audios)

            for new_audio, sample, cur_golden_mark in zip(new_audios, current_samples, golden_mark):
                new_file_path = new_folder + '/' + sample.uuid + '_distorted.wav'
                new_audio.save(new_file_path)
                new_sample = sample
                new_sample.reference = new_file_path
                new_sample.golden_mark = cur_golden_mark
                new_samples.append(new_sample.to_json())

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(new_samples, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

    def distort_questions(self, config_path, new_folder, question, answer, distortion_types=ALL_DISTORTIONS):
        if not set(distortion_types).issubset(set(ALL_DISTORTIONS)):
            raise Exception(f'Distortion types {distortion_types} is not a subset of ALL_DISTORTIONS {ALL_DISTORTIONS}')

        num_samples = len(self.samples)

        new_samples = []
        group_by_distortion_samples = {}
        for index in range(num_samples):
            distortion_idx = np.random.randint(0, len(distortion_types))
            distortion_type = distortion_types[distortion_idx]

            if distortion_type not in group_by_distortion_samples:
                group_by_distortion_samples[distortion_type] = {'audios': [], 'samples': []}
            group_by_distortion_samples[distortion_type]['samples'].append(self.samples[index])
            group_by_distortion_samples[distortion_type]['audios'].append(
                AudioSample(file=self.samples[index].synthesis))

        for distortion_type, values in group_by_distortion_samples.items():
            audios = values['audios']
            current_samples = values['samples']
            new_audios = VoiceDistorter.apply(distortion_type, audios)

            for new_audio, sample in zip(new_audios, current_samples):
                new_file_path = new_folder + '/' + sample.uuid + '_distorted.wav'
                new_audio.save(new_file_path)
                new_sample = sample
                new_sample.synthesis = new_file_path
                new_sample.question = question
                new_sample.golden_mark = answer
                new_samples.append(new_sample.to_json())

        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(new_samples, f, ensure_ascii=False, indent=4)

        return PairDataset(config_path, self.s3_config)

