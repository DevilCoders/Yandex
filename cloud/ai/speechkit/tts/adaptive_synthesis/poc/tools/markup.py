import json
import os
import pandas as pd

from collections import namedtuple


MarkupResult = namedtuple(
    'MarkupResult',
    field_names=[
        'audio1',
        'audio2',
        'mark',
        'golden_mark',
        'assignment_id',
        'task_suite_id',
        'worker_id',
        'status',
        'started_date',
        'submitted_date',
        'accept_verdict',
        'accept_comment'
    ],
    defaults=[None, None]
)

PaySample = namedtuple('PaySample', field_names=['assignment_id', 'verdict', 'comment'], defaults=[''])


def read_assignments(path_to_file):
    field_to_name_mapping = {
        'audio1': 'INPUT:audio1',
        'audio2': 'INPUT:audio2',
        'mark': 'OUTPUT:mark',
        'golden_mark': 'GOLDEN:mark',
        'assignment_id': 'ASSIGNMENT:assignment_id',
        'task_suite_id': 'ASSIGNMENT:task_suite_id',
        'worker_id': 'ASSIGNMENT:worker_id',
        'status': 'ASSIGNMENT:status',
        'started_date': 'ASSIGNMENT:started',
        'submitted_date': 'ASSIGNMENT:submitted'
    }

    df = pd.read_csv(path_to_file, sep='\t')

    markups = []
    for idx, row in df.iterrows():
        if str(row['INPUT:audio1']) == 'nan':
            continue
            print(row)
            input()

        parameters = {field: row[name] for field, name in field_to_name_mapping.items()}
        cur_markup = MarkupResult(
            **parameters
        )
        markups.append(cur_markup)

    return markups


def read_markup_from_json(path_to_json):
    with open(path_to_json, 'r') as f:
        obj = json.load(f)

    markups = []

    for item in obj['items']:
        if item['status'] == 'EXPIRED' or item['status'] == 'SKIPPED':
            continue

        tasks = item['tasks']
        marks = item['solutions']
        user_id = item['user_id']
        status = item['status']
        task_suite_id = item['task_suite_id']
        submitted_date = item['submitted']

        for i in range(len(tasks)):
            task = tasks[i]
            assignment_id = task['id']

            markup = MarkupResult(
                audio1=task['input_values']['audio1'],
                audio2=task['input_values']['audio2'],
                mark=marks[i]['output_values']['mark'],
                golden_mark=task['known_solutions'][0]['output_values']['mark'] if 'known_solutions' in task else None,
                assignment_id=assignment_id,
                task_suite_id=task_suite_id,
                worker_id=user_id,
                status=status,
                started_date=None,
                submitted_date=submitted_date,
                accept_verdict=None,
                accept_comment=None
            )
            markups.append(markup)

    return markups


def save_pay_samples(path, pay_samples):
    with open(os.path.join(path, 'assignments_paid.tsv'), 'w') as f:
        header = ['ASSIGNMENT:assignment_id', 'ACCEPT:verdict', 'ACCEPT:comment']
        f.write('\t'.join(header) + '\n')
        for sample in pay_samples:
            f.write('\t'.join([sample.assignment_id, sample.verdict, sample.comment]) + '\n')
