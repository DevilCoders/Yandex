import boto3
import datetime
import json
import numpy as np
import pandas as pd
import requests

from collections import namedtuple
from copy import deepcopy
from dateutil.relativedelta import relativedelta

from tools.data import bootstrap, group_by
from tools.data_interface.quality_evalutation import *
from tools.distortion import distort


TolokaBase = namedtuple('TolokaBase', ['host', 'token'])
Task = namedtuple('Task', ['pool_id', 'audio1', 'audio2', 'golden_mark'])


class Toloka(TolokaBase):
    def __new__(cls):
        return super(Toloka, cls).__new__(
            cls,
            host='https://toloka.yandex.ru',
            token=Toloka.read_token()
        )

    @staticmethod
    def read_token():
        with open(os.path.expanduser('~/.toloka/token'), 'r') as f:
            return f.read()[:-1]

    def request(self, method, path, params=None, body=None):
        headers = {
            "Authorization": f"OAuth {self.token}",
            "Content-Type": "application/json"
        }
        result = requests.request(
            method=method,
            url=f'{self.host}{path}',
            headers=headers,
            data=body,
            params=params
        )

        result_json = json.loads(str(result.content, encoding="utf-8"))
        if result.status_code == 200:
            print('OK')
        elif result.status_code == 201:
            print('CREATED')
        elif result.status_code == 202:
            print('ACCEPTED')
        else:
            raise Exception(f'Failed request, code: {result.status_code}\nResponse:\n{result_json}')

        return result_json


def mix_with_other_phrases(pool: QualityEvaluationPool, ratio):
    bootstrapped_samples = bootstrap(pool.samples, ratio)

    new_samples = deepcopy(pool.samples)

    grouped_by_speaker_samples = group_by(bootstrapped_samples, lambda sample: sample.speaker_id)
    for speaker_id, samples in grouped_by_speaker_samples.items():
        new_pair_indices = list(range(len(samples)))
        np.random.shuffle(new_pair_indices)

        for i in range(len(samples)):
            cur_sample = samples[i]
            assert cur_sample.speaker_id == samples[new_pair_indices[i]].speaker_id

            if cur_sample.path_to_reference == samples[new_pair_indices[i]].path_to_reference:
                continue

            new_sample = cur_sample.set_phrase_combination_with_copy(
                path_to_synthesis=samples[new_pair_indices[i]].path_to_synthesis,
                phrase_combination=PhraseCombination.DIFFERENT
            )
            new_samples.append(new_sample)

    return QualityEvaluationPool(new_samples)


def add_ref_to_ref_test(pool: QualityEvaluationPool, ratio):
    bootstrapped_samples = bootstrap(pool.samples, ratio)
    new_samples = deepcopy(pool.samples)

    grouped_by_speaker_samples = group_by(bootstrapped_samples, lambda sample: sample.speaker_id)
    for speaker_id, samples in grouped_by_speaker_samples.items():
        new_pair_indices = list(range(len(samples)))
        np.random.shuffle(new_pair_indices)

        for i in range(len(samples)):
            assert samples[i].speaker_id == samples[new_pair_indices[i]].speaker_id

            if samples[i].path_to_reference == samples[new_pair_indices[i]].path_to_reference:
                continue

            cur_sample = samples[i].set_pair_type_with_copy(
                path_to_synthesis=samples[new_pair_indices[i]].path_to_reference,
                pair_type=PairType.REF_TO_REF
            )
            new_samples.append(cur_sample)

    return QualityEvaluationPool(new_samples)


def create_mixed_pool(pools, ratios, approximate_length):
    assert len(pools) == len(ratios)
    assert np.sum(ratios) == 1.

    samples = []
    relative_ratios = [ratio * approximate_length / pool.length for ratio, pool in zip(ratios, pools)]
    for pool, ratio in zip(pools, relative_ratios):
        sampled_pool = bootstrap(pool, ratio)
        samples.extend(sampled_pool.sample_descriptions)

    return QualityEvaluationPool(samples)


def write_pool_to_s3(pool: QualityEvaluationPool, s3_path, s3_bucket='cloud-ai-data'):
    cloud_storage_url = 'https://storage.yandexcloud.net'
    finalized_samples = []

    session = boto3.session.Session(profile_name='cloud-ai-data')
    s3 = session.client(
        service_name='s3',
        endpoint_url=cloud_storage_url
    )

    num_samples = len(pool.samples)
    for i, sample in enumerate(pool.samples):
        print(f'{i}/{num_samples}', end='\r')
        reference_uid = str(uuid.uuid4())
        synthesis_uid = str(uuid.uuid4())

        s3_reference_path = os.path.join(s3_path, reference_uid + '.wav')
        s3_synthesis_path = os.path.join(s3_path, synthesis_uid + '.wav')

        finalized_sample = sample.set_urls_with_copy(
            url_reference=f'{cloud_storage_url}/{s3_bucket}/{s3_reference_path}',
            url_synthesis=f'{cloud_storage_url}/{s3_bucket}/{s3_synthesis_path}'
        )

        s3.upload_file(
            sample.path_to_reference,
            s3_bucket,
            s3_reference_path,
            ExtraArgs={'ACL': 'public-read'}
        )
        s3.upload_file(
            sample.path_to_synthesis,
            s3_bucket,
            s3_synthesis_path,
            ExtraArgs={'ACL': 'public-read'}
        )

        finalized_samples.append(finalized_sample)
    print()

    return QualityEvaluationPool(finalized_samples)


def create_tsv_task(pool: QualityEvaluationPool, path):
    with open(os.path.join(path, 'toloka_task.tsv'), 'w') as toloka_tsv_file:
        toloka_tsv_file.write('\t'.join(['INPUT:audio1', 'INPUT:audio2', 'GOLDEN:mark', 'HINT:text']) + '\n')

        shuffled_indices = list(range(pool.length))
        np.random.shuffle(shuffled_indices)

        for sample_idx in shuffled_indices:
            sample = pool.get(sample_idx)
            urls = (sample.url_synthesis, sample.url_reference)
            if np.random.uniform(0, 1) > 0.5:
                urls = (urls[1], urls[0])

            toloka_tsv_file.write('\t'.join([
                urls[0],
                urls[1],
                str(sample.golden_mark).lower() if sample.golden_mark is not None else '',
                ''
            ]) + '\n')


def create_toloka_pool(path_to_synthesized_samples, pool_name):
    mtt_pool = QualityEvaluationPool.read_from_synthesis_pool(path_to_synthesized_samples)
    print(f'pool length: {mtt_pool.length}')
    mtt_pool = distort(mtt_pool.samples, ratio_samples_to_distort=0.1)
    print(f'pool length after distortion: {mtt_pool.length}')

    path_to_multispeaker_pool = '../data/metric_pool/paired_pool/pool_multispeaker_noised/'
    path_to_mozilla_pool = '../data/metric_pool/paired_pool/pool_bad_mozilla_noised/'

    multispeaker_pool = QualityEvaluationPool.read_from_synthesis_pool(path_to_multispeaker_pool)
    mozilla_pool = QualityEvaluationPool.read_from_synthesis_pool(path_to_mozilla_pool)

    multispeaker_pool_30p = QualityEvaluationPool(bootstrap(multispeaker_pool.samples, 0.3))
    multispeaker_pool_30p_with_distortions = distort(multispeaker_pool_30p.samples)

    print(f'mozilla pool length: {mozilla_pool.length}')
    print(f'multispeaker pool length: {multispeaker_pool_30p.length}')
    print(f'multispeaker length after distortion: {multispeaker_pool_30p_with_distortions.length}')

    mixed_samples = deepcopy(mtt_pool.samples) + \
        deepcopy(multispeaker_pool_30p_with_distortions.samples) + \
        deepcopy(mozilla_pool.samples)

    mixed_pool = QualityEvaluationPool(mixed_samples)
    print(f'final pool length {mixed_pool.length}')

    s3_pool_path = os.path.join('Data/Speechkit/toloka/tts_quality_metric/', pool_name)
    toloka_pool_path = os.path.join('../data/metric_pool/toloka_pool/', pool_name)

    if not os.path.exists(toloka_pool_path):
        os.makedirs(toloka_pool_path)

    finalized_mixed_pool = write_pool_to_s3(
        mixed_pool,
        s3_pool_path
    )
    create_tsv_task(finalized_mixed_pool, toloka_pool_path)
    finalized_mixed_pool.to_tsv(os.path.join(toloka_pool_path, 'toloka_pool.tsv'))
    return finalized_mixed_pool


def get_cur_date_plus_year():
    cur_date = datetime.datetime.today()
    cur_date_plus_year = cur_date + relativedelta(years=1)
    return cur_date_plus_year


def get_template_pool_config():
    with open('../tools/toloka_template_pool_config.json', 'r') as f:
        template_pool_properties = json.load(f)
        return template_pool_properties


def read_tasks_from_tsv(path, pool_id):
    df = pd.read_csv(os.path.join(path, 'toloka_task.tsv'), sep='\t')
    tasks = []

    for idx, row in df.iterrows():
        task = Task(
            pool_id=pool_id,
            audio1=row['INPUT:audio1'],
            audio2=row['INPUT:audio2'],
            golden_mark=None if np.isnan(row['GOLDEN:mark']) else row['GOLDEN:mark']
        )
        tasks.append(task)

    return tasks


def task_to_json(task: Task):
    result = {
        "pool_id": task.pool_id,
        "input_values": {
            "audio1": task.audio1,
            "audio2": task.audio2
        }
    }

    if task.golden_mark is not None:
        result["known_solutions"] = [{
            "output_values": {"mark": task.golden_mark}
        }]

    return result


def create_web_toloka_pool(toloka, pool_name):
    template_pool_properties = get_template_pool_config()
    template_pool_properties['private_name'] = pool_name
    template_pool_properties['will_expire'] = get_cur_date_plus_year().isoformat()

    result = toloka.request('POST', f'/api/v1/pools', body=json.dumps(template_pool_properties))
    return result['id']


def push_tasks(toloka, pool_id, path_to_tasks):
    tasks = read_tasks_from_tsv(path_to_tasks, pool_id)
    tasks_json = list(map(lambda t: task_to_json(t), tasks))

    toloka.request(
        'POST',
        path='/api/v1/tasks',
        params={
            'async_mode': False,
            'allow_defaults': True,
            'skip_invalid_items': False,
            'open_pool': False,
        },
        body=json.dumps(tasks_json)
    )

    golden_tasks_count = len(list(filter(lambda t: 'known_solutions' in t, tasks_json)))
    tasks_count = len(tasks_json)
    real_tasks_count = tasks_count - golden_tasks_count

    print(f'tasks_count:\t\t{tasks_count}')
    print(f'real_tasks_count:\t\t{real_tasks_count}')
    print(f'golden_tasks_count:\t\t{golden_tasks_count}')


def run_toloka_assignments(pool_name, path_to_tasks):
    toloka = Toloka()

    new_pool_id = create_web_toloka_pool(toloka, pool_name)
    push_tasks(toloka, new_pool_id, path_to_tasks)

    result = toloka.request('POST', path=f'/api/v1/pools/{new_pool_id}/open')
    assert result['type'] == 'POOL.OPEN' and result['status'] == 'SUCCESS'
    return new_pool_id


def get_markup_results(pool_id, path_to_save):
    toloka = Toloka()
    result = toloka.request('GET', path=f'/api/v1/assignments', params={'pool_id': pool_id, 'limit': 10000})
    assert not result['has_more']
    with open(os.path.join(path_to_save, 'assignments.json'), 'w') as f:
        json.dump(result, f)
    return result
