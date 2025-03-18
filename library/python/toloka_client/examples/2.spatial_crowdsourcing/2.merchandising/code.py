import datetime
import os
import time
from collections import defaultdict, Counter
import math

import pandas as pd
import toloka.client
from toloka.client import TolokaClient
from toloka.client.actions import Restriction
from toloka.client.collectors import GoldenSet
from toloka.client.conditions import CorrectAnswersRate, TotalAnswersCount
from toloka.client.project.field_spec import UrlSpec, StringSpec, ArrayFileSpec
from toloka.client.project.task_spec import TaskSpec
from toloka.client.project.view_spec import ClassicViewSpec
from toloka.client.pool import Pool
from toloka.client.task import Task
from toloka.client.user_restriction import UserRestriction

from toloka.client import TolokaClient
from toloka.client.project import Project
from toloka.client.project.task_spec import TaskSpec
from toloka.client.project.field_spec import CoordinatesSpec, StringSpec, ArrayFileSpec
from toloka.client.project.template_builder import TemplateBuilder
from toloka.client.project.template_builder.data import InputData
from toloka.client.project.template_builder.view import TextViewV1
from toloka.client.project.view_spec import TemplateBuilderViewSpec
from toloka.client.task_suite import TaskSuite
from toloka.client.task import BaseTask
from toloka.client.filter import ClientType
from toloka.client.filter import RegionByPhone


TOLOKA_TOKEN = ''


toloka_client = TolokaClient(
    TOLOKA_TOKEN,
    os.getenv('TOLOKA_ENVIRONMENT', 'production'),
)

# Creating project

project_interface = toloka.client.project.view_spec.ClassicViewSpec(
    script=open(os.path.join('projects', 'pedestrian_template.js')).read().strip(),
    markup=open(os.path.join('projects', 'pedestrian_template.html')).read().strip(),
    styles=open(os.path.join('projects', 'pedestrian_template.css')).read().strip(),
    assets=toloka.client.project.view_spec.ClassicViewSpec.Assets(
        script_urls=['$TOLOKA_ASSETS/js/toloka-handlebars-templates.js'],
        style_urls=[]
    ),
)
prepared_instruction = open(os.path.join('projects', 'instruction.html')).read().strip()

project = Project(
    public_name='Сфотографировать полку с сухофруктами/орехами в магазине',
    public_description='Необходимо прийти в организацию, отмеченную на карте, внутри найти полку с сухофруктами/орехами и сфотографировать её.',
    public_instructions=prepared_instruction,
    assignments_issuing_type=Project.AssignmentsIssuingType.MAP_SELECTOR,
    assignments_issuing_view_config=Project.AssignmentsIssuingViewConfig(
        title_template='{{inputParams["name"]}}',
        description_template='{{inputParams["product"]}}',
    ),
    task_spec=TaskSpec(
        # Why at least one input for map?
        input_spec={
            'name': StringSpec(required=False),
            'image': StringSpec(required=False),
            'address': StringSpec(required=False),
            'product': StringSpec(required=False),
            'coordinates': StringSpec(required=False),
        },
        output_spec={
            'address': StringSpec(required=False),
            'comment': StringSpec(required=False),
            'verdict': StringSpec(required=False),
            'imgs_obj': ArrayFileSpec(required=False),
            'coordinates': StringSpec(required=False),
            'imgs_facade': ArrayFileSpec(required=False),
            'imgs_around_obj': ArrayFileSpec(required=False),
            'imgs_around_org': ArrayFileSpec(required=False),
            'worker_coordinates': CoordinatesSpec(required=False, current_location=True),
            'imgs_plate_or_address': ArrayFileSpec(required=False),
        },
        view_spec=project_interface
    ),
)

project = toloka_client.create_project(project)
print(f'Created project with id {project.id}  https://toloka.yandex.ru/requester/project/{project.id}')

# Creating pool
pool = Pool(
    project_id=project.id,
    private_name='Пул с заданиями',
    may_contain_adult_content=False,
    will_expire=datetime.datetime.utcnow() + datetime.timedelta(days=1),
    reward_per_assignment=0.5,
    assignment_max_duration_seconds=60 * 60 * 12,
    auto_accept_solutions=False,
    filter=((ClientType == ClientType.ClientType.TOLOKA_APP) &(RegionByPhone.in_(225))),
    defaults=Pool.Defaults(
        default_overlap_for_new_task_suites=1,
        default_overlap_for_new_tasks=1,
    ),
)

pool = toloka_client.create_pool(pool)
print(f'Created pool with id {pool.id}  https://toloka.yandex.ru/requester/project/{pool.project_id}/pool/{pool.id}')

task_suites = [
    toloka_client.create_task_suite(
        TaskSuite(
            pool_id=pool.id,
            longitude=round(row['AI:longitude'], 6),
            latitude=round(row['AI:latitude'], 6),
            overlap=1,
            # TODO: input values must be dict by default
            tasks=[
                BaseTask(input_values={
                    'name': row['INPUT:name'],
                    'image': 'https://disk.yandex.ru/i/w_d-lR0BvrLofA',
                    'address': row['INPUT:address'],
                    'product': 'Орехи, сухофрукты всех производителей',
                    'coordinates': row['INPUT:coordinates'],
                })
            ]
        )
    )
    for i, row in data.iterrows()
]
print(f'Created task_suite {task_suites[0]}')

toloka_client.open_pool(pool.id)
print(f'Opened pool with id {pool.id} https://toloka.yandex.ru/requester/project/{pool.project_id}/pool/{pool.id}')