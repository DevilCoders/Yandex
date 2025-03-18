import toloka.client as toloka
from toloka import streaming
from toloka.streaming import cursor
from crowdkit.aggregation import MajorityVote  # pip install crowd-kit==0.0.7

import pandas

import collections
import logging
from typing import List

from .base_handler import BaseHandler, TaskResult

logger = logging.getLogger(__name__)


class QualityHandler(BaseHandler):
    ### Get accepted assignments from Validation Quality Project
    ### accept/reject main tasks
    ### pay bonuses

    def __init__(self, toloka_client, tracking_pool_id, next_pool_id=None, ruled_pool_id=None, skill_id=None, fs=None, tracker=None, task_overlap=None):
        assert skill_id is not None
        super().__init__(toloka_client, tracking_pool_id, next_pool_id=next_pool_id, ruled_pool_id=ruled_pool_id, fs=fs, tracker=tracker)
        self._task_answers = self.load_param(collections.defaultdict(list), 'task_answers', 'quality')
        assert task_overlap is not None
        self.task_overlap = task_overlap

        self._groups = self.load_param(collections.defaultdict(list), 'groups', 'quality')
        self._groups_to_decide = []

        self._skill_cursor = cursor.UserSkillCursor(skill_id=skill_id, event_type='MODIFIED', toloka_client=toloka_client)
        self._performers_skill = {}
        self._default_skill_level = 70.123  # we will use it if performer does not have skill
        # result label values for Rejecting and Accepting(next processing) images
        self._result_labels = {'accept': ['1', '2'], 'reject': ['3', '4']}

    def process_new_events(self, assignment_events):
        self._logger.info(f'process_new_events - {len(assignment_events)} new events')
        for event in assignment_events:
            for task, solution in zip(event.assignment.tasks, event.assignment.solutions):
                # Filter golden-sets
                if task.known_solutions:
                    continue
                task_result = TaskResult(
                    task=task.id,
                    label=solution.output_values['ImageQuality'],
                    performer=event.assignment.user_id,
                    url=task.input_values['ImageUrl'],
                    pedestrian_assignment_id=task.input_values['pedestrian_assignment_id'],
                    group_id=task.input_values['group_id'],
                    group_count=task.input_values['group_count'],
                    image_field=task.input_values['image_field'],
                    main_task=task.input_values['main_task'],
                    name = task.input_values['Name'] if 'Name' in task.input_values else None,
                    segment = task.input_values['Segment'] if 'Segment' in task.input_values else None,
                    address = task.input_values['Address'] if 'Address' in task.input_values else None,
                    bing_url = task.input_values['BingUrl'] if 'BingUrl' in task.input_values else None,
                    google_url = task.input_values['GoogleUrl'] if 'GoogleUrl' in task.input_values else None,
                    pws_url = task.input_values['PWSURL'] if 'PWSURL' in task.input_values else None,
                    phone_number = task.input_values['PhoneNumber'] if 'PhoneNumber' in task.input_values else None,
                )
                self._task_answers[task.id].append(task_result)

        self._logger.info(f'process_new_events - task_answers count: {len(self._task_answers)}')

    async def process_new_skills(self):
        self._logger.info(f'process_new_skills - start')
        async for skill_event in self._skill_cursor:
            self._performers_skill[skill_event.user_skill.user_id] = float(skill_event.user_skill.exact_value)
        self._logger.info(f'process_new_skills - end')

    def aggregate_answers(self):
        new_to_aggregate = []
        for answers in self._task_answers.values():
            if len(answers) >= self.task_overlap:
                for answer in answers:
                    new_to_aggregate.append((
                        answer.task,
                        answer.label,
                        answer.performer,
                        # All fields below added only for logging reason
                        answer.pedestrian_assignment_id,
                        'main' if answer.main_task else answer.image_field,
                        answer.url,
                    ))

        if not new_to_aggregate:
            return

        self._logger.info(f'aggregate_answers - aggregate something new')
        to_aggregate_df = pandas.DataFrame(new_to_aggregate, columns=['task', 'label', 'performer', 'pedestrian_assignment_id', 'group', 'url'])
        skill_series = pandas.Series(data=self._performers_skill.values(), index=self._performers_skill.keys())
        default_skills = pandas.Series(self._default_skill_level, index=(set(to_aggregate_df.performer.unique()) - set(skill_series.index)))
        skill_series = skill_series.append(default_skills)
        aggregated: pandas.Series = MajorityVote().fit_predict(to_aggregate_df, skill_series)
        self._tracker.add_answers('quality', pandas.merge(to_aggregate_df, aggregated.reset_index(name='result'), on='task'), skill_series)

        for task_id, result in aggregated.items():
            if result not in self._result_labels['accept'] and result not in self._result_labels['reject']:
                self._logger.error(f'Aggregation problems. Task "{task_id}" has aggregated result "{result}", that does not exist in possible result labels: {self._result_labels}')
            task_result = self._task_answers[task_id][0]
            task_result.label = result
            self._groups[task_result.group_id].append(task_result)
            if len(self._groups[task_result.group_id]) >= task_result.group_count:
                self._groups_to_decide.append(task_result.group_id)
            del self._task_answers[task_id]

        self._logger.info(f'aggregate_answers - groups count: {len(self._groups)}')
        self._logger.info(f'aggregate_answers - groups_to_decide count: {len(self._groups_to_decide)}')


    async def make_decisions_for_groups(self) -> List[toloka.Task]:
        new_tasks = []
        if not self._groups_to_decide:
            return new_tasks

        self._logger.info('make_decisions_for_groups - start')
        urls_for_delete = []
        full_group_ids = set(self._groups_to_decide)

        for current_group_id in full_group_ids:
            sub_groups = collections.defaultdict(list)
            main_task = self._groups[current_group_id][0].main_task
            pedestrian_assignment_id = self._groups[current_group_id][0].pedestrian_assignment_id

            pedestrian_assignment = await self._toloka_client.get_assignment(pedestrian_assignment_id)
            dicision_dict = {
                'name': pedestrian_assignment.tasks[0].input_values['name'],
                'address': pedestrian_assignment.tasks[0].input_values['address'],
                'pedestrian_assignment_id':  pedestrian_assignment_id,
                'handler': 'quality',
                'group': '',
                'event': '',
                'comment': '',
            }
            
            for task_result in self._groups[current_group_id]:
                if task_result.label in self._result_labels['accept']:
                    sub_groups[task_result.image_field].append(task_result)
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': 'main' if main_task else task_result.image_field,
                        'event': 'GOOD image',
                        'comment': task_result.url,
                    })
                else:
                    urls_for_delete.append(task_result.url)
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': 'main' if main_task else task_result.image_field,
                        'event': 'BAD image',
                        'comment': task_result.url,
                    })

            group_count = sum(len(val) for val in sub_groups.values())
            create_new_tasks = True

            additional_fields = {
                'segment': 'Segment',
                'bing_url': 'BingUrl',
                'google_url': 'GoogleUrl',
                'pws_url': 'PWSURL',
                'phone_number': 'PhoneNumber',
            }
            if main_task:
                # if we have't good images for every subgroup, we must reject assignment in Pedestrian Project
                must_have_fields = {'imgs_facade', 'name_business', 'imgs_name_address'}
                if len(set(sub_groups.keys()) & must_have_fields) < len(must_have_fields):
                    # reject assignment in Pedestrian Project
                    self._logger.info(f'DECISION -> reject assignment. - Starts rejecting pedestrian assignment "{pedestrian_assignment_id}"')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': 'main',
                        'event': 'REJECT assignment',
                        'comment': str(must_have_fields - set(sub_groups.keys())),
                    })
                    try:
                        await self._toloka_client.reject_assignment(pedestrian_assignment_id, 'Low quality photos')
                    except toloka.exceptions.IncorrectActionsApiError as ex:
                        self._logger.error(f'Can\'t reject padestrian assignment({pedestrian_assignment_id}): {str(ex)}')
                    # del images from S3
                    for sub_group in sub_groups.values():
                        for task_result in sub_group:
                            urls_for_delete.append(task_result.url)
                    create_new_tasks = False
            else:
                if not sub_groups:
                    # do nothing, because we have only bonus here
                    self._logger.info(f'DECISION -> reject bonus in pedestrian assignment "{pedestrian_assignment_id}"". Field "{self._groups[current_group_id][0].image_field}" - Low quality photos in this bonus field.')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': self._groups[current_group_id][0].image_field,
                        'event': 'STOP validate bonus',
                        'comment': 'Low quality photos in this bonus field',
                    })
                    create_new_tasks = False

            if create_new_tasks:
                for image_field, tasks_results in sub_groups.items():
                    for task_result in tasks_results:
                        input_values = {
                            'ImageUrl': task_result.url,
                            'pedestrian_assignment_id': task_result.pedestrian_assignment_id,
                            'group_id': current_group_id,
                            'group_count': group_count,
                            'image_field': image_field,
                            'main_task': main_task,
                            'Name': task_result.name,
                            'Address': task_result.address,
                        }
                        for task_field, input_field in additional_fields.items():
                            if getattr(task_result, task_field) is not None: 
                                input_values[input_field] = getattr(task_result, task_field)
                        new_tasks.append(toloka.Task(pool_id=self._next_pool_id, input_values=input_values))
                self._tracker.add_event({
                    **dicision_dict,
                    'group': 'main' if main_task else self._groups[current_group_id][0].image_field,
                    'event': 'to relevance validation',
                    'comment': f'{group_count} new tasks',
                })

            del self._groups[current_group_id]

        self._groups_to_decide = []
        # await self._fs.del_files_by_urls(urls_for_delete)

        self._logger.info(f'make_decisions_for_groups - after: groups - {len(self._groups)}, groups_to_decide - {len(self._groups_to_decide)}')
        self._logger.info(f'make_decisions_for_groups - prepared tasks: {len(new_tasks)}')
        return new_tasks

    async def __call__(self, assignment_events: List[streaming.event.AssignmentEvent]) -> None:
        # store new events and find finished tasks
        self.process_new_events(assignment_events)

        # get new skill values
        await self.process_new_skills()

        # aggregate answers on finished tasks
        # find finished groups
        self.aggregate_answers()

        # make decisions on finished groups
        # prepare tasks for next project
        new_tasks = await self.make_decisions_for_groups()

        # create new tasks
        if new_tasks:
            self._logger.info('start creating new tasks')
            create_result = await self._toloka_client.create_tasks(new_tasks, allow_defaults=True, open_pool=True)
            self._logger.info(f'Created {len(create_result.items)} new tasks in Relevance Validation Project')
            if hasattr(create_result, 'validation_errors') and create_result.validation_errors:
                self._logger.error(f'Problems on creating tasks: {str(create_result.validation_errors)}')

        self.save_param(self._task_answers, 'task_answers', 'quality')
        self.save_param(self._groups, 'groups', 'quality')
