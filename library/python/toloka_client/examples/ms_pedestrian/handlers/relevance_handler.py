import toloka.client as toloka
from toloka import streaming
from toloka.streaming import cursor
from crowdkit.aggregation import MajorityVote  # pip install crowd-kit==0.0.7

import pandas

from datetime import datetime, timedelta
from decimal import Decimal
import collections
import logging
from typing import List

from .base_handler import BaseHandler, TaskResult, get_assignment_dy_id

logger = logging.getLogger(__name__)


class RelevanceHandler(BaseHandler):
    """Get accepted assignments from ValidationRelevance#2

    Args:
        toloka_client - toloka.client.TolokaClient 
        tracking_pool_id - pool in Validation Relevance Project (not training or exam)
        ruled_pool_id - None
        skill_id - skill from Validation Relevance Project, to measure performers skills in relevance
    """
    def __init__(self, toloka_client, tracking_pool_id, next_pool_id=None, ruled_pool_id=None, skill_id=None, fs=None, tracker=None, task_overlap=None):
        assert skill_id is not None
        super().__init__(toloka_client, tracking_pool_id, next_pool_id=next_pool_id, ruled_pool_id=ruled_pool_id, fs=fs, tracker=tracker)
        self._task_answers = self.load_param(collections.defaultdict(list), 'task_answers', 'relevance')
        assert task_overlap is not None
        self.task_overlap = task_overlap

        self._groups = self.load_param(collections.defaultdict(list), 'groups', 'relevance')
        self._groups_to_decide = []

        self._skill_cursor = cursor.UserSkillCursor(skill_id=skill_id, event_type='MODIFIED', toloka_client=toloka_client)
        self._performers_skill = {}
        self._default_skill_level = 70.123  # we will use it if performer does not have skill
        # result label values for Rejecting and Accepting(next processing) images
        self._result_labels = {'accept': ['1', '2'], 'reject': ['3', '4', '5']}

        self.bonus_id_dollars = Decimal('0.05')
        self._new_bonuses = self.load_param({}, 'new_bonuses', 'relevance')

        self.last_bonuses_flush = datetime.utcnow()
        self.user_frendly_names = {
            'imgs_interior': 'business interior',
            'imgs_covid_policy': 'COVID policies',
            'imgs_outdoor_seating': 'outdoors seating',
            'imgs_menu': 'menu in the restaurant',
        }


    def process_new_events(self, assignment_events):
        self._logger.info(f'process_new_events - {len(assignment_events)} new events')
        for event in assignment_events:
            for task, solution in zip(event.assignment.tasks, event.assignment.solutions):
                # Filter golden-sets
                if task.known_solutions:
                    continue
                task_result = TaskResult(
                    task=task.id,
                    label=solution.output_values['ImageRelevance'],
                    performer=event.assignment.user_id,
                    url=task.input_values['ImageUrl'],
                    pedestrian_assignment_id=task.input_values['pedestrian_assignment_id'],
                    group_id=task.input_values['group_id'],
                    group_count=task.input_values['group_count'],
                    image_field=task.input_values['image_field'],
                    main_task=task.input_values['main_task'],
                )
                self._task_answers[task.id].append(task_result)
        self._logger.info(f'process_new_events - task_answers count: {len(self._task_answers)}, ')

    async def process_new_skills(self):
        self._logger.info(f'process_new_skills - start')
        async for skill_event in self._skill_cursor:
            self._performers_skill[skill_event.user_skill.user_id] = float(skill_event.user_skill.exact_value)
        self._logger.info(f'process_new_skills - end')

    def aggregate_answers(self):
        new_to_aggregate = []
        for answers in self._task_answers.values():
            if len(answers) >= self.task_overlap:
                for tmp_task_res in answers:
                    new_to_aggregate.append((
                        tmp_task_res.task,
                        tmp_task_res.label,
                        tmp_task_res.performer,
                        # All fields below added only for logging reason
                        tmp_task_res.pedestrian_assignment_id,
                        'main' if tmp_task_res.main_task else tmp_task_res.image_field,
                        tmp_task_res.url,
                    ))

        if not new_to_aggregate:
            return

        self._logger.info(f'aggregate_answers - aggregate something new')
        to_aggregate_df = pandas.DataFrame(new_to_aggregate, columns=['task', 'label', 'performer', 'pedestrian_assignment_id', 'group', 'url'])
        skill_series = pandas.Series(data=self._performers_skill.values(), index=self._performers_skill.keys())
        default_skills = pandas.Series(self._default_skill_level, index=(set(to_aggregate_df.performer.unique()) - set(skill_series.index)))
        skill_series = skill_series.append(default_skills)
        aggregated: pandas.Series = MajorityVote().fit_predict(to_aggregate_df, skill_series)
        self._tracker.add_answers('relevance', pandas.merge(to_aggregate_df, aggregated.reset_index(name='result'), on='task'), skill_series)

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

    async def make_decisions_for_groups(self):
        if not self._groups_to_decide:
            return

        self._logger.info('make_decisions_for_groups - start')
        urls_for_delete = []
        full_group_ids = set(self._groups_to_decide)
        for current_group_id in full_group_ids:
            sub_groups = collections.defaultdict(list)

            main_task = self._groups[current_group_id][0].main_task
            pedestrian_assignment_id = self._groups[current_group_id][0].pedestrian_assignment_id
            pedestrian_assignment = await get_assignment_dy_id(pedestrian_assignment_id, self._toloka_client)

            dicision_dict = {
                'name': pedestrian_assignment.tasks[0].input_values['name'],
                'address': pedestrian_assignment.tasks[0].input_values['address'],
                'pedestrian_assignment_id':  pedestrian_assignment_id,
                'handler': 'relevance',
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

            if main_task:
                # if we have't good images for every subgroup, we must reject assignment in Pedestrian Project
                must_have_fields = {'imgs_facade', 'name_business', 'imgs_name_address'}
                if len(set(sub_groups.keys()) & must_have_fields) < len(must_have_fields):
                    # reject assignment in Pedestrian Project
                    self._logger.info(f'DECISION -> reject assignment. - Starts rejecting pedestrian assignment "{pedestrian_assignment_id}". Not relevant photos.')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': 'main',
                        'event': 'REJECT assignment',
                        'comment': str(must_have_fields - set(sub_groups.keys())),
                    })
                    try:
                        await self._toloka_client.reject_assignment(pedestrian_assignment_id, 'Not relevant photos')
                    except toloka.exceptions.IncorrectActionsApiError as ex:
                        self._logger.error(f'Can\'t reject padestrian assignment({pedestrian_assignment_id}): {str(ex)}')
                    # del all images from S3
                    for sub_group in sub_groups.values():
                        for task_result in sub_group:
                            urls_for_delete.append(task_result.url)
                else: 
                    self._logger.info(f'DECISION -> accept assignment, all validations pass. - Pedestrian assignment "{pedestrian_assignment_id}" - ok, with good photos.')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': 'main',
                        'event': 'ACCEPT pedestrian assignment',
                        'comment': '',
                    })
                    try:
                        # accept assignment in Pedestrian Project
                        await self._toloka_client.accept_assignment(pedestrian_assignment_id, 'Good job')
                    except toloka.exceptions.IncorrectActionsApiError as ex:
                        self._logger.error(f'Can\'t accept padestrian assignment({pedestrian_assignment_id}): {str(ex)}')
                    # publish somwhere result - all fields from padestrian input and links to good images
                    self._logger.info(f'Store result - Pedestrian task - done')
                    named_urls = collections.defaultdict(list)
                    for field_name, task_results in sub_groups.items():
                        named_urls[field_name] = [tr.url for tr in task_results]
                    await self._fs.store_main_results(
                        pedestrian_assignment.tasks[0].id,
                        pedestrian_assignment.tasks[0].input_values,
                        pedestrian_assignment.solutions[0].output_values,
                        named_urls,
                    )

            else:
                if not sub_groups:
                    # do nothing, because we have only bonus here
                    self._logger.info(f'DECISION -> reject bonus in pedestrian assignment "{pedestrian_assignment_id}"". Field "{self._groups[current_group_id][0].image_field}" - Not relevant photos in this bonus field.')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': self._groups[current_group_id][0].image_field,
                        'event': 'STOP validate bonus',
                        'comment': 'Not relevant photos in this bonus field',
                    })
                else:
                    self._logger.info(f'Store result - Bonus ""{self._groups[current_group_id][0].image_field}" in pedestrian assignment "{pedestrian_assignment_id}" - done.')
                    self._tracker.add_event({
                        **dicision_dict,
                        'group': self._groups[current_group_id][0].image_field,
                        'event': 'PAY bonus',
                        'comment': '',
                    })
                    # publish somewhere results
                    field_name = self._groups[current_group_id][0].image_field
                    await self._fs.store_bonus_results(
                        pedestrian_assignment.tasks[0].id,
                        pedestrian_assignment.tasks[0].input_values,
                        field_name,
                        [task_result.url for task_result in sub_groups[field_name]],
                    )
 
                    user_id = pedestrian_assignment.user_id
                    bonus_id = f'{pedestrian_assignment_id} {user_id}'
                    image_field = self.user_frendly_names[self._groups[current_group_id][0].image_field]

                    if bonus_id in self._new_bonuses:
                        bonus = self._new_bonuses[bonus_id]
                        bonus.amount = bonus.amount + self.bonus_id_dollars
                        bonus.public_message = f'{bonus.public_message}\n  - {image_field}'
                    else:
                        self._new_bonuses[bonus_id] = toloka.UserBonus(
                            user_id=user_id,
                            amount=self.bonus_id_dollars,
                            public_title={'EN': 'Bonus for pedestrian project'},
                            public_message={'EN': f'Accepted bonuses:\n  - {image_field}'},
                        )

            del self._groups[current_group_id]

        self._groups_to_decide = []
        #await self._fs.del_files_by_urls(urls_for_delete)

        self._logger.info(f'make_decisions_for_groups - after: groups - {len(self._groups)}, groups_to_decide - {len(self._groups_to_decide)}')
        self._logger.info(f'make_decisions_for_groups - prepared bonuses: {len(self._new_bonuses)}')

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
        await self.make_decisions_for_groups()

        # pay bonuses
        now = datetime.utcnow()
        if ((now - self.last_bonuses_flush) > timedelta(hours=1) and self._new_bonuses) or len(self._new_bonuses) > 1000:
            self.last_bonuses_flush = datetime.utcnow()
            bonuses_to_create = [bonus for bonus in self._new_bonuses.values()]
            self._logger.info(f'Starting to pay {len(bonuses_to_create)} bonuses')
            # TODO get operation and track errors
            await self._toloka_client.create_user_bonuses_async(bonuses_to_create)
            self._new_bonuses = {}

        self.save_param(self._task_answers, 'task_answers', 'relevance')
        self.save_param(self._groups, 'groups', 'relevance')
        self.save_param(self._new_bonuses, 'new_bonuses', 'relevance')
