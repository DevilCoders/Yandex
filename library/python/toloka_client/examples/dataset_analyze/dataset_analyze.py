"""Need to install first:
!pip install toloka-kit==0.1.13
!pip install crowd-kit==0.0.5
!pip install pandas
"""
import argparse

import pandas as pd

import toloka.client as toloka
from crowdkit.aggregation import GoldMajorityVote
from crowdkit.metrics.data._classification import consistency, uncertainty


def create_datasets_from_tsv(path_to_tsv, label_field_name):
    """Returns:
       - answers dataset as DataFrame at first
       - golden-set labels as Series at second
    """
    answers_df = pd.read_csv(path_to_tsv, sep='\t')
    answers_df = answers_df.rename(columns={
        'ASSIGNMENT:task_id': 'task',
        f'OUTPUT:{label_field_name}': 'label',
        f'GOLDEN:{label_field_name}': 'truth_label',
        'ASSIGNMENT:worker_id': 'performer',
    })
    column_names = [value for value in answers_df.columns.values.tolist() if value.startswith('INPUT') or value in ('task', 'label', 'truth_label', 'performer')]
    answers_df = answers_df[column_names]

    golden_ser = answers_df[answers_df['truth_label'].notnull()]
    golden_ser = golden_ser[['task', 'truth_label']]
    golden_ser = golden_ser.set_index('task')
    golden_ser = golden_ser.squeeze()
    golden_ser = golden_ser[~golden_ser.index.duplicated(keep='first')]
    return answers_df, golden_ser


def create_datasets_from_pool(pool_id, label_field_name, toloka_client):
    """Returns:
       - answers dataset as DataFrame at first
       - golden-set labels as Series at second
    """
    answers = []
    golden_set = {}
    input_names = []
    for assignment in toloka_client.get_assignments(pool_id=pool_id, status='ACCEPTED'):
        for task, solution in zip(assignment.tasks, assignment.solutions):
            answers.append([task.id, solution.output_values[label_field_name], assignment.user_id, *task.input_values.values()])
            input_names = task.input_values.keys()
            if task.known_solutions:
                golden_set[task.id] = task.known_solutions[0].output_values[label_field_name]
    input_names = [f'INPUT:{val}' for val in input_names]
    return pd.DataFrame(answers, columns=['task', 'label', 'performer', *input_names]), pd.Series(data=golden_set)


def get_aggregated_metrics(answers, skills, name='', by_task=False):
    res_uncertainty = uncertainty(answers, skills, by_task=by_task)
    res_consistency = consistency(answers, skills, by_task=by_task)
    if name:
        print(f'uncertainty for {name}: {res_uncertainty}')
        print(f'consistency for {name}: {res_consistency}')
    return res_uncertainty, res_consistency


def get_suspicious_tasks(tasks_uncertainty, tasks_consistency, uncertainty_more = 1, consistency_less = 0.5):
    suspicious_tasks = set(tasks_uncertainty[tasks_uncertainty > uncertainty_more].index.values.tolist())
    suspicious_tasks = suspicious_tasks | set(tasks_consistency[tasks_consistency < consistency_less].index.values.tolist())
    return suspicious_tasks


def get_dataframe_with_tasks(tasks, answers_df):
        column_names = [value for value in answers_df.columns.values.tolist() if value.startswith('INPUT') or value == 'task']
        result_df = answers_df[column_names].groupby(by=['task']).first().reset_index()
        result_df = result_df[result_df['task'].isin(tasks)]
        return result_df


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Dataset analyze')
    parser.add_argument('--tsv', type=str, help='Gets dataset from this file. Same format as download answers from pool. This parameter or "--pool".')
    parser.add_argument('--pool', type=str, help='ID for pool in Toloka. Gets dataset from this pool. This parametr or "--tsv".')
    parser.add_argument('--field', type=str, help='Field name in outputs by which aggragate answers and calculate all metrics.')
    parser.add_argument('--token', type=str, help='Toloka auth token.')
    parser.add_argument('-G', action='store_true', help='Analyze goldenset. Will use it if -G or -O does not set.')
    parser.add_argument('-O', action='store_true', help='Analyze other tasks.')

    args = parser.parse_args()

    # load datasets
    if args.tsv is not None:
        answers_df, golden_ser = create_datasets_from_tsv(args.tsv, args.field)
    elif args.pool is not None:
        toloka_client = toloka.TolokaClient(args.token, 'PRODUCTION')
        answers_df, golden_ser = create_datasets_from_pool(args.pool, args.field, toloka_client)
    else:
        assert False, 'Sets tsv or pool parametr'
    
    # aggragate answers
    aggregator = GoldMajorityVote()
    result_labels = aggregator.fit_predict(answers_df, golden_ser)  # standart way to aggregate labels if you have golden-set
    performers_skills = aggregator.skills_
    performers_skills.name = 'performer_skill'

    # print summary metrics
    golden_answers = answers_df[answers_df['task'].isin(golden_ser.index)]
    general_answers = answers_df[~answers_df['task'].isin(golden_ser.index)]
    get_aggregated_metrics(golden_answers, performers_skills, name='golden-set')
    get_aggregated_metrics(general_answers, performers_skills, name='other tasks')

    # calcs metrics for all tasks
    tasks_uncertainty, tasks_consistency = get_aggregated_metrics(answers_df, performers_skills, by_task=True) 

    # analyze general tasks
    if args.O:
        general_tasks_uncertainty = tasks_uncertainty[~tasks_uncertainty.index.isin(golden_ser.index)]
        general_tasks_consistency = tasks_consistency[~tasks_consistency.index.isin(golden_ser.index)]
        suspicious_tasks = get_suspicious_tasks(general_tasks_uncertainty, general_tasks_consistency)
        suspicious_df = get_dataframe_with_tasks(suspicious_tasks, answers_df)
        with pd.option_context('display.max_rows', None, 'display.max_columns', None, 'display.width', 400, "max_colwidth", 40):
            print(f'\nSuspicious general tasks\n{suspicious_df}')
        suspicious_df.to_csv('general_suspicious_tasks.tsv', sep='\t')

    if args.G or not args.O:
        golden_tasks_uncertainty = tasks_uncertainty[tasks_uncertainty.index.isin(golden_ser.index)]
        golden_tasks_consistency = tasks_consistency[tasks_consistency.index.isin(golden_ser.index)]
        suspicious_tasks = get_suspicious_tasks(golden_tasks_uncertainty, golden_tasks_consistency)
        suspicious_df = get_dataframe_with_tasks(suspicious_tasks, answers_df)
        with pd.option_context('display.max_rows', None, 'display.max_columns', None, 'display.width', 400, "max_colwidth", 40):
            print(f'\nSuspicious golden tasks\n{suspicious_df}')
        suspicious_df.to_csv('golden_suspicious_tasks.tsv', sep='\t')


