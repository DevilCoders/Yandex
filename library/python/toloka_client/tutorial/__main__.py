import datetime
import os
import time
from collections import defaultdict, Counter

import pandas
import toloka.client
from toloka.client import TolokaClient
from toloka.client.actions import Restriction
from toloka.client.collectors import GoldenSet
from toloka.client.conditions import CorrectAnswersRate, TotalAnswersCount
from toloka.client.project.field_spec import UrlSpec, StringSpec
from toloka.client.project.task_spec import TaskSpec
from toloka.client.project.view_spec import ClassicViewSpec
from toloka.client.pool import Pool
from toloka.client.task import Task
from toloka.client.user_restriction import UserRestriction

toloka_client = TolokaClient(
    os.getenv('TOLOKA_TOKEN'),
    os.getenv('TOLOKA_ENVIRONMENT', 'sandbox')
)

# Creating project
project = toloka.client.project.Project(
    assignments_issuing_type=toloka.client.project.Project.AssignmentsIssuingType.AUTOMATED,
    public_name='Есть ли на картинке дорожный знак?',
    public_description='Посмотрите на картинку и определите, есть ли на ней дорожный знак',
    # TODO add templates for spec
    task_spec=TaskSpec(
        input_spec={'image': UrlSpec()},
        output_spec={'result': StringSpec()},
        view_spec=ClassicViewSpec(
            script=open('script.js').read().strip(),
            markup=open('markup.hbs').read().strip(),
            styles='',
            assets=ClassicViewSpec.Assets(script_urls=[
                '$TOLOKA_ASSETS/js/toloka-handlebars-templates.js'
            ]),
            settings={
                'showSkip': True,
                'showTimer': True,
                'showTitle': True,
                'showSubmit': True,
                'showFullscree': True,
                'showInstructions': True,
            }
        ),
    ),
)

project = toloka_client.create_project(project)
print(f'Created project with id {project.id}  https://sandbox.toloka.yandex.ru/requester/project/{project.id}')

# Creating pool
pool = Pool(
    project_id=project.id,
    private_name='Pool 1',
    may_contain_adult_content=False,
    will_expire=datetime.datetime.utcnow() + datetime.timedelta(days=1),
    reward_per_assignment=0.01,
    assignment_max_duration_seconds=600,
    defaults=Pool.Defaults(
        # for sandbox
        default_overlap_for_new_task_suites=1,
        default_overlap_for_new_tasks=1,
    ),
)

# Configuring mixer config
pool.set_mixer_config(real_tasks_count=6, golden_tasks_count=1, training_tasks_count=0)

# Configuring quality control
pool.quality_control.add_action(
    collector=GoldenSet(history_size=10),
    conditions=[TotalAnswersCount > 3, CorrectAnswersRate < 60],
    action=Restriction(
        private_comment='Did not pass golden set',
        scope=UserRestriction.PROJECT,
        duration_days=1,
    ),
)

pool = toloka_client.create_pool(pool)
print(f'Created pool with id {pool.id}  https://sandbox.toloka.yandex.ru/requester/project/{pool.project_id}/pool/{pool.id}')

data = pandas.read_csv('dataset.tsv', sep='\t')
tasks = [
    Task(input_values={'image': url}, pool_id=pool.id)
    for url in data['image'].values[:20]
]
golden_tasks = [
    Task(
        input_values={'image': url},
        pool_id=pool.id,
        infinite_overlap=True,
        known_solutions=[Task.KnownSolution(
            output_values={'result': 'OK'},
            correctness_weight=1
        )]
    ) for url in data['image'].values[20:25]
]

toloka_client.create_tasks(tasks + golden_tasks, allow_defaults=True)
print(f'populated pool {pool.id} with {len(tasks)} tasks and {len(golden_tasks)} golden tasks')

operation = toloka_client.open_pool(pool.id)
operation = toloka_client.wait_operation(operation)
assert operation.status == operation.SUCCESS, operation.status
print(f'opened pool {pool.id}')

print('waiting for pool to be closed')
sleep_time = 30
pool = toloka_client.get_pool(pool.id)
while not pool.is_closed():
    print(
        f'{datetime.datetime.now().strftime("%H:%M:%S")}\t'
        f'Pool {pool.id} has status {pool.status}. Sleeping for {sleep_time} seconds'
    )
    time.sleep(sleep_time)
    pool = toloka_client.get_pool(pool.id)

print(f'Pool {pool.id} was finally closed!')

task_id_to_solutions = defaultdict(list)
task_id_to_task = {}

for assignment in toloka_client.find_all_assignments(pool_id=pool.id):
    for task, solution in zip(assignment.tasks, assignment.solutions):
        task_id_to_solutions[task.id].append(solution)
        task_id_to_task[task.id] = task


labeled_data = []
for i, (task_id, solutions) in enumerate(task_id_to_solutions.items()):
    task = task_id_to_task[task_id]
    (majority_solution, _), = Counter(solution.output_values['result'] for solution in solutions).most_common(1)
    labeled_data.append({'image': task.input_values['image'], 'label': majority_solution})

pandas.DataFrame(labeled_data).to_csv('labeled.tsv', sep='\t')
