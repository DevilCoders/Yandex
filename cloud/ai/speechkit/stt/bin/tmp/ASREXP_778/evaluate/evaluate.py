import json
import os
import time
import typing
from collections import defaultdict
from dataclasses import dataclass
from random import shuffle

import nirvana.job_context as nv
import requests
import yt.wrapper as yt
import toloka.client as toloka

from cloud.ai.lib.python.datetime import now, parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep, MarkupAssignmentStatus, MarkupDataVersions,
)
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient
from cloud.ai.speechkit.stt.lib.tmp.ASREXP_778 import (
    table_source_meta, table_evaluation_entries_meta, table_evaluations_meta,
    SourceAudio, EvaluationEntry, Submission, Evaluation,
)
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


@dataclass
class Task:
    audio_index: int
    reference_audio_url: str
    hypothesis_audio_url: str

    def to_toloka_format(self, is_honeypot: bool):
        data = {
            'input_values': {
                'left_audio_url': self.reference_audio_url,
                'right_audio_url': self.hypothesis_audio_url,
            },
        }
        if is_honeypot:
            data['known_solutions'] = [
                {
                    'correctness_weight': 1,
                    'output_values': {
                        'result': self.reference_audio_url,
                    },
                },
            ]

        overlap = 10000 if is_honeypot else 5
        data['overlap'] = overlap

        return data


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs
    params = op_ctx.parameters

    with open(inputs.get('submission.json')) as f:
        submission = Submission.from_yson(json.load(f))

    postpone_until = parse_datetime(params.get('postpone-until'))
    if postpone_until is not None:
        while True:
            time.sleep(60 * 60)
            if now() >= postpone_until:
                break

    submission_audio_index_to_s3_url = upload_submission_audio_to_s3(inputs.get("wav.tar.gz"))
    tasks, honeypots = get_tasks_and_honeypots(submission_audio_index_to_s3_url)

    tasks_json = [t.to_toloka_format(is_honeypot=False) for t in tasks]
    honeypots_json = [t.to_toloka_format(is_honeypot=True) for t in honeypots]

    toloka_client = TolokaClient(oauth_token=params.get('toloka-token'), lang='ru-RU')
    revaluate: bool = params.get('revaluate')

    revaluate_pool_name_suffix = ' [RE]' if revaluate else ''

    pool_id = toloka_client.create_pool(
        pool=(
            '18640947', {
                'private_name':
                    f'Participant "{submission.login}" '
                    f'submission {submission.id} evaluation{revaluate_pool_name_suffix}',
            },
        ),
        tasks=tasks_json + honeypots_json,
    )

    toloka_client.await_pool_status(pool_id, toloka.Pool.Status.CLOSED)

    evaluation_id = generate_id()

    assignments = toloka_client.get_assignments_failsafe(
        pool_id=pool_id,
        markup_id=evaluation_id,
        markup_step=MarkupStep.COMPARE_AUDIO_EUPHONY,
        pull_interval_seconds=60,
    )

    reference_audio_url_to_index = {}
    for task in tasks:
        reference_audio_url_to_index[task.reference_audio_url] = task.audio_index

    received_at = now()
    evaluation_entries = []
    audio_index_to_total = defaultdict(int)
    audio_index_to_wins = defaultdict(int)
    for assignment in assignments:
        if assignment.data.status != MarkupAssignmentStatus.ACCEPTED:
            continue
        for task in assignment.tasks:
            if len(task.known_solutions) > 0:
                continue
            assert task.version == MarkupDataVersions.COMPARE_AUDIO_EUPHONY

            left_audio_url = task.input.left_audio_url
            right_audio_url = task.input.right_audio_url
            chosen_audio_url = task.solution.chosen_audio_url

            if left_audio_url in reference_audio_url_to_index:
                reference_audio_url = left_audio_url
            elif right_audio_url in reference_audio_url_to_index:
                reference_audio_url = right_audio_url
            else:
                raise ValueError(f'Neither audios from markup correspond to reference audio')

            audio_index = reference_audio_url_to_index[reference_audio_url]

            audio_index_to_total[audio_index] += 1
            if chosen_audio_url != reference_audio_url:
                audio_index_to_wins[audio_index] += 1

            evaluation_entries.append(EvaluationEntry(
                login=submission.login,
                submission_id=submission.id,
                evaluation_id=evaluation_id,
                audio_index=audio_index,
                left_audio_url=left_audio_url,
                right_audio_url=right_audio_url,
                reference_audio_url=reference_audio_url,
                chosen_audio_url=chosen_audio_url,
                pool_id=pool_id,
                assignment_id=assignment.data.source_id,
                received_at=received_at,
            ))

    total = 0
    wins = 0
    for audio_index in audio_index_to_total:
        # Decide win by majority vote
        audio_total = audio_index_to_total[audio_index]
        audio_wins = audio_index_to_wins[audio_index]
        total += 1
        if audio_wins > audio_total // 2:
            wins += 1

    score = wins / total

    with open(outputs.get('score.txt'), 'w') as f:
        f.write(str(score))

    evaluation = Evaluation(
        login=submission.login,
        score=score,
        wins=wins,
        total=total,
        submission_id=submission.id,
        submitted_at=submission.received_at,
        evaluation_id=evaluation_id,
        evaluated_at=received_at,
    )

    yt_table_name = 'table'
    ch_table_name = 'evaluations'

    if revaluate:
        yt_table_name += '_final'
        ch_table_name += '_final'

    table_evaluation_entries = Table(meta=table_evaluation_entries_meta, name=yt_table_name)
    table_evaluations = Table(meta=table_evaluations_meta, name=yt_table_name)

    table_evaluation_entries.append_objects(evaluation_entries)
    table_evaluations.append_objects([evaluation])

    append_evaluation_to_clickhouse(evaluation, ch_table_name, params.get('ch-db-password'))


"""
Задание в Толоке – сравнение двух аудио на то, какое из них благозвучнее – то, что слева, или то, что справа.

Для конкурса зафиксирован test set из 10200 аудио, из которых случайные 100 выбраны для разметки.

Для ханипотов используется таблица test set аудио, где для каждого аудио есть 5 вариантов "испорченных" аудио,
у которых вторые 8 тактов взяты из другого аудио, ожидается, что такие аудио заведомо менее благозвучные и
исполнитель должен выбрать изначальное аудио из test set.

Для реальных заданий используются аудио из test set, а также аудио, сгенерированное участниками конкурса. Соотносятся
аудио по индексам 0-10199 (по факту по 100 используемых в разметке). Каждый раз, когда исполнители при разметке
выбирают, как более благозвучное, аудио участника конкурса, ему начисляются очки. По итоговому score по всем 100
аудио строится leaderboard.

В итоге и для ханипотов, и для реальных заданий из двух аудио одно всегда из test set, а другое – заранее
испорченное или присланное участником. Мы всегда знаем, какое из пары аудио из test set, но его надо ставить
случайным образом либо слева, либо справа. Это делается специальным js в настройках проекта Толоки, в качестве
ответа передается URL выбранного аудио.
"""


def get_tasks_and_honeypots(submission_audio_index_to_s3_url: typing.Dict[int, str]) -> typing.Tuple[
    typing.List[Task], typing.List[Task],
]:
    table_source = Table(meta=table_source_meta, name='table')
    source_audios = []
    source_audio_index_to_s3_url = {}
    for row in yt.read_table(table_source.path):
        source_audio = SourceAudio.from_yson(row)
        source_audios.append(source_audio)
        source_audio_index_to_s3_url[source_audio.index] = source_audio.url

    honeypots = []
    for source_audio in source_audios:
        reference_url = source_audio.url
        for honeypot_url, is_active in zip(source_audio.honeypots_urls, source_audio.honeypots_active):
            if is_active:
                honeypots.append(Task(
                    audio_index=source_audio.index,
                    reference_audio_url=reference_url,
                    hypothesis_audio_url=honeypot_url,
                ))

    shuffle(honeypots)
    honeypots = honeypots[:1000]  # never take more than 1000 to make pool creation faster

    tasks = []
    for index, submission_url in submission_audio_index_to_s3_url.items():
        reference_url = source_audio_index_to_s3_url[index]
        tasks.append(Task(
            audio_index=index,
            reference_audio_url=reference_url,
            hypothesis_audio_url=submission_url,
        ))

    shuffle(tasks)

    return tasks, honeypots


def upload_submission_audio_to_s3(submission_wavs_archive_path: str) -> typing.Dict[int, str]:
    os.system(f'tar xzf {submission_wavs_archive_path}')

    audio_index_to_s3_url = {}
    s3 = create_client()

    for wav_filename in os.listdir('wav'):
        with open(f'wav/{wav_filename}', 'rb') as f:
            index = int(wav_filename[:wav_filename.find('.')])
            id = generate_id()
            s3_key = f'Tmp/ASREXP-778/markup/{id}.wav'

            s3.upload_fileobj(
                f,
                Bucket='cloud-ai-data',
                Key=s3_key,
                ExtraArgs={'ACL': 'public-read'},
            )

            audio_index_to_s3_url[index] = f'https://storage.yandexcloud.net/cloud-ai-data/{s3_key}'

    return audio_index_to_s3_url


def append_evaluation_to_clickhouse(evaluation: Evaluation, table: str, ch_db_password: str):
    query = f"""
INSERT INTO {table}
VALUES
('{evaluation.login}', {evaluation.score}, {evaluation.wins}, {evaluation.total}, '{evaluation.submission_id}', {int(evaluation.submitted_at.timestamp())}, '{evaluation.evaluation_id}', {int(evaluation.evaluated_at.timestamp())})
"""

    url = 'https://{host}:8443/?database={db}&query={query}'.format(
        host='rc1a-g1bcnglc16m728s7.mdb.yandexcloud.net',
        db='db1',
        query=query)
    auth = {
        'X-ClickHouse-User': 'user1',
        'X-ClickHouse-Key': ch_db_password,
    }

    resp = requests.post(
        url,
        headers=auth,
        verify=False)
    resp.raise_for_status()
