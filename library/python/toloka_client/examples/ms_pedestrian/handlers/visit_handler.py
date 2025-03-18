#from datetime import datetime
import toloka.client as toloka
from toloka import streaming
from toloka.streaming import cursor

from crowdkit.aggregation import MajorityVote  # pip install crowd-kit==0.0.7

import pandas

import collections
from io import BytesIO
import logging
import os
from typing import List
import uuid

from .base_handler import BaseHandler, get_assignment_dy_id

logger = logging.getLogger(__name__)


class VisitHandler(BaseHandler):
    ### Get accepted assignment from ValidationVisit#1
    def __init__(self, toloka_client, tracking_pool_id, next_pool_id=None, ruled_pool_id=None, skill_id=None, fs=None, tracker=None, task_overlap=None):
        assert skill_id is not None
        super().__init__(toloka_client, tracking_pool_id, next_pool_id=next_pool_id, ruled_pool_id=ruled_pool_id, fs=fs, tracker=tracker)

        self._skill_cursor = cursor.UserSkillCursor(skill_id=skill_id, event_type='MODIFIED', toloka_client=toloka_client)
        self._performers_skill = {}

        self._task_answers = self.load_param(collections.defaultdict(list), 'task_answers', 'visit')
        assert task_overlap is not None
        self.task_overlap = task_overlap
        self._default_skill_level = 70.123  # we will use it if performer does not have skill

        # result label values for Rejecting and Accepting(next processing) pedestrian task
        self._result_labels = {'accept': 'yes', 'reject': 'no'}

    def process_new_events(self, assignment_events):
        self._logger.info(f'process_new_events - {len(assignment_events)} new events')
        for event in assignment_events:
            for task, solution in zip(event.assignment.tasks, event.assignment.solutions):
                # Filter golden-sets
                if task.known_solutions:
                    continue
                answer = (task.id, solution.output_values['assignmentPhotosQuality'], event.assignment.user_id, task.input_values['pedestrian_assignment_id'])
                self._task_answers[task.id].append(answer)
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
                new_to_aggregate += answers
        
        if new_to_aggregate:
            self._logger.info(f'aggregate_answers - aggregate something new')
            to_aggregate_df = pandas.DataFrame(new_to_aggregate, columns=['task', 'label', 'performer', 'pedestrian_assignment_id'])
            skill_series = pandas.Series(data=self._performers_skill.values(), index=self._performers_skill.keys())
            default_skills = pandas.Series(self._default_skill_level, index=(set(to_aggregate_df.performer.unique()) - set(skill_series.index)))
            skill_series = skill_series.append(default_skills)
            aggregated_df = MajorityVote().fit_predict(to_aggregate_df, skill_series).reset_index(name='result')
            self._tracker.add_answers('visit', pandas.merge(to_aggregate_df, aggregated_df, on='task'), skill_series)
            aggregated_df = pandas.merge(aggregated_df, to_aggregate_df.drop_duplicates(subset='task'), on='task')
            self._logger.info(f'aggregate_answers - aggregated count: {len(aggregated_df)}')
            return aggregated_df
        return None

    def prepare_general_input(self, pedestrian_assignment, row):
        # prepare same input_fields for next Project
        organization_name = pedestrian_assignment.tasks[0].input_values['name']
        general_input = {
            #'ImageUrl': '',  # main field, assign it later
            'Name': organization_name,
            'Segment': pedestrian_assignment.tasks[0].input_values['Segment'],
            'Address': pedestrian_assignment.tasks[0].input_values['address'],
            'BingUrl': f'https://www.bing.com/search?q={organization_name}',
            'GoogleUrl': f'https://www.google.ru/search?q={organization_name}',
            'pedestrian_assignment_id': row.pedestrian_assignment_id,
        }
        if 'PWSURL' in pedestrian_assignment.tasks[0].input_values:
            general_input['PrimaryWebsite'] = pedestrian_assignment.tasks[0].input_values['PWSURL']
        if 'PhoneNumber' in pedestrian_assignment.tasks[0].input_values:
            general_input['PhoneNumber'] = pedestrian_assignment.tasks[0].input_values['Phonumber']
        return general_input

    def add_main_tasks(self, current_task, row, task_input, dicision_dict):
        self._logger.debug('add_main_tasks - start')
        new_tasks = []
        # count images - create list of tuples (img_id, img_field_name)
        image_list = []
        visit_to_pedestrian_fields = {
            'imgs_facade': 'imgs_facade',
            'name_business': 'name_business',
            'hoo_business': 'hoo_business',
            'imgs_name_address': 'imgs_name_address',
        }
        for field_visit_name, field_pedestrian_name  in visit_to_pedestrian_fields.items():
            if field_visit_name not in current_task.input_values:
                continue
            urls = current_task.input_values[field_visit_name]
            if not isinstance(urls, list):
                urls = [urls]
            image_list += [(url, field_pedestrian_name) for url in urls]
        # create tasks
        self._logger.debug('add_main_tasks - create tasks')
        group_id = str(uuid.uuid4())
        group_count = len(image_list)
        for url, image_field in image_list:
            new_tasks.append(
                toloka.Task(
                    pool_id=self._next_pool_id,
                    input_values={
                        **task_input,
                        'ImageUrl': url,
                        'group_id': group_id,
                        'group_count': group_count,
                        'image_field': image_field,
                        'main_task': True,
                    }
                )
            )
        self._logger.info(f'add_main_tasks - Add {len(new_tasks)} new tasks for main group')
        self._tracker.add_event({
            **dicision_dict,
            'group': 'main',
            'event': 'to quality validation',
            'comment': f'{group_count} new tasks',
        })
        return new_tasks

    async def add_bonus_tasks(self, current_task, pedestrian_assignment, task_input, dicision_dict):
        new_tasks = []
        possible_bonuses = {
            'can_take_interior': 'imgs_interior',
            'can_take_covid': 'imgs_covid_policy',
            'can_take_outdoors': 'imgs_outdoor_seating',
            'can_take_menu': 'imgs_menu',
        }
        for can_take, imgs_field_name in possible_bonuses.items():
            if can_take not in pedestrian_assignment.solutions[0].output_values:
                continue
            if pedestrian_assignment.solutions[0].output_values[can_take].lower() != 'yes':
                continue
            if imgs_field_name not in current_task.input_values:
                continue
            images_url = current_task.input_values[imgs_field_name]
            if not isinstance(images_url, list):
                images_url = [images_url]
            # create group tasks
            group_id = str(uuid.uuid4())
            group_count = len(images_url)
            for url in images_url:
                new_tasks.append(
                    toloka.Task(
                        pool_id=self._next_pool_id,
                        input_values={
                            **task_input,
                            'ImageUrl': url,
                            'group_id': group_id,
                            'group_count': group_count,
                            'image_field': imgs_field_name,
                            'main_task': False,
                        }
                    )
                )
            self._tracker.add_event({
                **dicision_dict,
                'group': imgs_field_name,
                'event': 'to quality validation',
                'comment': f'{group_count} new tasks',
            })
        self._logger.info(f'add_bonus_tasks - Add {len(new_tasks)} new tasks')
        return new_tasks

    async def del_task_images_from_s3(self, task, field_list) -> None:
        self._logger.info(f'del_task_images_from_s3 - start')
        urls_for_delete = []
        for field_name in field_list:
            add_urls = task.input_values[field_name]
            if isinstance(add_urls, list):
                urls_for_delete += add_urls
            else:
                urls_for_delete.append(add_urls)
        await self._fs.del_files_by_urls(urls_for_delete)

    async def prepare_new_tasks(self, aggregated_df, assignment_events) -> List[toloka.Task]:
        self._logger.info(f'prepare_new_tasks - start')

        new_tasks = []

        if aggregated_df is None:
            return new_tasks

        for row in aggregated_df.itertuples():
            if row.result.lower() not in self._result_labels['accept'] and row.result.lower() not in self._result_labels['reject']:
                self._logger.error(f'Aggregation problems. Task "{row.task}" has aggregated result "{row.result.lower()}", that does not exist in possible result labels: {self._result_labels}')

            current_task = None
            for event in assignment_events:
                for task in event.assignment.tasks:
                    if task.id == row.task:
                        current_task = task
                        break

            pedestrian_assignment = await self._toloka_client.get_assignment(row.pedestrian_assignment_id)

            dicision_dict = {
                'name': pedestrian_assignment.tasks[0].input_values['name'],
                'address': pedestrian_assignment.tasks[0].input_values['address'],
                'pedestrian_assignment_id':  row.pedestrian_assignment_id,
                'handler': 'visit',
                'group': '',
                'event': 'aggregated',
                'comment': f'result - {row.result}',
            }
            self._tracker.add_event(dicision_dict)

            if row.result.lower() == self._result_labels['reject']:
                # if "result == no", then reject assignment by id in PedestrianProj (assignment_id in input fields).
                # Exit.
                # Add overlap via quality control in PedestrianProj(pool).
                self._logger.info(f'DECISION -> reject assignment. - Starts rejecting pedestrian assignment "{row.pedestrian_assignment_id}"')
                self._tracker.add_event({**dicision_dict, 'event': 'REJECT assignment and all bonuses'})
                try:
                    await self._toloka_client.reject_assignment(row.pedestrian_assignment_id, 'Incorrect object')
                except toloka.exceptions.IncorrectActionsApiError as ex:
                    self._logger.error(f'Can\'t reject padestrian assignment({row.pedestrian_assignment_id}): {str(ex)}')
            else: # if "result == yes" (performer in pedestrian project visited valid place)
                # get assignment from Pedestrian Project and task from Validation Visit Project
                task_input = self.prepare_general_input(pedestrian_assignment, row)
                if pedestrian_assignment.solutions[0].output_values['verdict'] == 'ok':
                    self._logger.info(f'DECISION -> validate next - Pedestrian assignment "{row.pedestrian_assignment_id}" - ok, organisation exists. Creating tasks for next validation projects.')
                    # Start next validation on main task
                    new_tasks += self.add_main_tasks(current_task, row, task_input, dicision_dict)
                    # create tasks for Validate main bonuses
                    new_tasks += await self.add_bonus_tasks(current_task, pedestrian_assignment, task_input, dicision_dict)
                else:  # 'verdict' == 'no_org' - accept assignment in PedestrianProj.
                    self._logger.info(f'DECISION -> accept assignment, stop validating. - Pedestrian assignment "{row.pedestrian_assignment_id}" - ok, NO ORG.')
                    self._tracker.add_event({**dicision_dict, 'event': 'ACCEPT assignment. No validation bonus org right now'})
                    try:
                        await self._toloka_client.accept_assignment(row.pedestrian_assignment_id, 'Well done!')
                    except toloka.exceptions.IncorrectActionsApiError as ex:
                        self._logger.error(f'Can\'t accept padestrian assignment({row.pedestrian_assignment_id}): {str(ex)}')
                    self._logger.info(f'Store result - there is no organization')
                    await self._fs.store_main_no_org_results(pedestrian_assignment.tasks[0].id, pedestrian_assignment.tasks[0].input_values)

                # TODO: create tasks for Validate other bonuses

            # delete information about task
            del self._task_answers[row.task]

        self._logger.info(f'prepare_new_tasks - after: task_answers - {len(self._task_answers)}')
        self._logger.info(f'prepare_new_tasks - prepared tasks: {len(new_tasks)}')
        return new_tasks

    async def __call__(self, assignment_events: List[streaming.event.AssignmentEvent]) -> None:
        # store new events and find finished tasks
        self.process_new_events(assignment_events)

        # get new skill values
        await self.process_new_skills()

        # aggregate answers on finished tasks
        aggregated_df = self.aggregate_answers()

        # prepare tasks for next project
        new_tasks = await self.prepare_new_tasks(aggregated_df, assignment_events)

        if new_tasks:
            self._logger.info('start creating new tasks')
            create_result = await self._toloka_client.create_tasks(new_tasks, allow_defaults=True, open_pool=True)
            self._logger.info(f'Created {len(create_result.items)} new tasks in Quality Validation Project')
            if hasattr(create_result, 'validation_errors') and create_result.validation_errors:
                self._logger.error(f'Problems on creating tasks: {str(create_result.validation_errors)}')

        # save inner params
        self.save_param(self._task_answers, 'task_answers', 'visit')
