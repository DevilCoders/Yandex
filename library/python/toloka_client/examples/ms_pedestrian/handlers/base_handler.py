import toloka.client as toloka
from toloka import streaming

from async_lru import alru_cache  # pip install async_lru
#from fresh_async_lru import alru_cache
import collections
from io import BytesIO
#from functools import lru_cache
import logging
from logging.handlers import TimedRotatingFileHandler
import os
from pathlib import Path
import pickle
import subprocess
import sys
from typing import Dict, List
import uuid

if sys.version_info[:2] >= (3, 8):
    from functools import cached_property
else:
    from cached_property import cached_property  # pip install cached_property

from .decision_tracker import DecisionTracker


@alru_cache(maxsize=128)
async def get_task_spec(pool_id: str, toloka_client: toloka.TolokaClient, logger) -> toloka.project.task_spec.TaskSpec:
    """cached task_spec with empty view_spec inside"""
    logger.debug(f'Start get_pool({pool_id})')
    pool = await toloka_client.get_pool(pool_id)
    logger.debug(f'Start get_project({pool.project_id})')
    project = await toloka_client.get_project(pool.project_id)
    logger.debug(f'Done get_project')
    result_task_spec = project.task_spec
    result_task_spec.view_spec = None
    return result_task_spec


@alru_cache(maxsize=512)
async def get_user_id_from_assignment(assignment_id: str, toloka_client: toloka.TolokaClient) -> str:
    assignment = await toloka_client.get_assignment(assignment_id)
    return assignment.user_id


@alru_cache(maxsize=512)
async def get_assignment_dy_id(assignment_id: str, toloka_client: toloka.TolokaClient) -> str:
    assignment = await toloka_client.get_assignment(assignment_id)
    return assignment


class LevelFilter(logging.Filter):
    def __init__(self, filter_level):
        super().__init__()
        self._filter_level = filter_level

    def filter(self, record):
        if record.levelno in self._filter_level:
            return True
        return False


class BaseHandler:
    # tracking_pool_id - pool that we are tracking
    # next_pool_id - on what pool we will send somthing
    # ruled_pool_id - pool in what we want to accept/reject 
    def __init__(self, toloka_client, tracking_pool_id, next_pool_id=None, ruled_pool_id=None, fs=None, tracker:DecisionTracker=None):
        self._toloka_client = toloka_client
        self._tracking_pool_id = tracking_pool_id
        self._next_pool_id = next_pool_id
        self._ruled_pool_id = ruled_pool_id
        self._fs = fs
        self._tracker = tracker
        self.prepare_logger()

    def prepare_logger(self):
        logs_folder = f'logs/{self.__class__.__name__}_{self._tracking_pool_id}'
        Path(logs_folder).mkdir(parents=True, exist_ok=True)
        self._logger = logging.getLogger(f'{self.__class__.__name__}_{self._tracking_pool_id}')
        #self._logger.setLevel(logging.INFO)  # if you want DEBUG level, change it here
        self._logger.setLevel(logging.DEBUG)  # if you want DEBUG level, change it here

        # create log for debug messages in runtime
        fh0 = TimedRotatingFileHandler(f'{logs_folder}/debug.log', when='midnight', backupCount=14, utc=True)
        fh0.setLevel(logging.DEBUG)
        fh0.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        self._logger.addHandler(fh0)


        # create log for simple messages in runtime
        fh1 = TimedRotatingFileHandler(f'{logs_folder}/info.log', when='midnight', backupCount=14, utc=True)
        fh1.setLevel(logging.INFO)
        fh1.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        fh1.addFilter(LevelFilter([logging.INFO, logging.WARNING]))
        self._logger.addHandler(fh1)

        # create log for extremely crutial messages in runtime
        fh2 = TimedRotatingFileHandler(f'{logs_folder}/error.log', when='midnight', backupCount=14, utc=True)
        fh2.setLevel(logging.ERROR)
        fh2.setFormatter(logging.Formatter('%(asctime)s - %(message)s'))
        self._logger.addHandler(fh2)

    async def __call__(assignment_events: List[streaming.event.AssignmentEvent]) -> None:
        ### main method to handle new events. Pipeline call this function
        raise NotImplementedError

    def save_param(self, value, name, folder):
        Path(os.path.join('saves', f'{folder}_{self._tracking_pool_id}')).mkdir(parents=True, exist_ok=True)
        file_name = os.path.join('saves', f'{folder}_{self._tracking_pool_id}', f'{name}.pickle')
        with open(file_name, 'wb') as f:
            pickle.dump(value, f)

    def load_param(self, defaul_value, name, folder):
        file_name = os.path.join('saves', f'{folder}_{self._tracking_pool_id}', f'{name}.pickle')
        if not os.path.exists(file_name):
            return defaul_value
        with open(file_name, 'rb') as f:
            return pickle.load(f)


class NewTask:
    """Describes how to create new tasks

    "keys" in fields:
    output.field_name - field from "solution" in current assignment (or solution.field_name)
    input.field_name - field from input from task in current assignment
    
    "vals" in fields:
    field_name - field name where to put value

    NewTask(
        {
            'input.coordinates': 'coordinates',
            'output.images': 'images_url',
        },
        async_toloka_client,
        pool_id,
    )"""
    _fields_mapping: Dict
    _toloka_client: toloka.TolokaClient
    _pool_id: str  # pool, where to create new tasks

    def __init__(
                    self,
                    fields :Dict, toloka_client: toloka.TolokaClient, pool_id: str, fs=None,
                    logger=None,
                    path_to_blur_exe=None, before_blur_folder=None, after_blur_folder=None) -> None:
        self._fields_mapping = fields
        self._toloka_client = toloka_client
        self._pool_id = pool_id
        self._fs = fs
        self._logger = logger

        self.path_to_blur_exe = path_to_blur_exe
        self.before_blur_folder = before_blur_folder
        self.after_blur_folder = after_blur_folder

    
    @cached_property
    def logger(self):
        if not self._logger is None:
            return self._logger
        return logging.getLogger(__name__)

    async def prepare_field_value(self, source, field_name, pool_id, from_src='output'):
        if field_name not in source:
            return None, None
        value = source[field_name]
        self.logger.debug(f'Start getting spec from {pool_id}')
        task_spec = await get_task_spec(pool_id, self._toloka_client, self.logger)
        self.logger.debug(f'Got spec from {pool_id}')

        src = task_spec.output_spec if from_src == 'output' else task_spec.input_spec
        field_type = src[field_name]
        if isinstance(field_type, toloka.project.field_spec.ArrayFileSpec) and value:
            self.logger.debug(f'Got ArrayFileSpec for "{field_name}". value = {value}')
            return None, value
        elif isinstance(field_type, toloka.project.field_spec.FileSpec) and value:
            return None, [value]
        else:
            self.logger.debug(f'Got other spec for "{field_name}"')
            return value, None

    async def prepare_images(self, images_dict, task_input):
        all_images = set()
        id_to_url = {}
        for image_list in images_dict.values():
            for image_id in image_list:
                all_images.add(image_id)
        
        if not all_images:
            return

        # 1. download all
        Path(self.before_blur_folder).mkdir(parents=True, exist_ok=True)
        file_name_to_id = {}
        self.logger.debug('Start downloading files')
        for id in all_images:
            self.logger.debug('    Before get_attachment')
            attachment_meta = await self._toloka_client.get_attachment(id)
            self.logger.debug('    After get_attachment')
            file_name = attachment_meta.name
            new_file_name = f'{str(uuid.uuid4())}-{file_name}'
            with open(os.path.join(self.before_blur_folder, new_file_name), 'wb') as tmp_file:
                self.logger.debug('    Before download_attachment')
                await self._toloka_client.download_attachment(id, tmp_file)
                self.logger.debug('    After download_attachment')
            file_name_to_id[new_file_name] = id
        
        # 2. blur at once
        Path(self.after_blur_folder).mkdir(parents=True, exist_ok=True)
        self.logger.debug('Start bluring')
        blur_result = subprocess.run([self.path_to_blur_exe, self.before_blur_folder, self.after_blur_folder], check=True)
        self.logger.debug(f'    blur result - {blur_result }')
        self.logger.debug('Finish bluring')

        # 3. upload
        self.logger.debug('Start uploading to storage')
        for file_name, file_id in file_name_to_id.items():
            file_full_path = os.path.join(self.after_blur_folder, file_name)
            if not os.path.exists(file_full_path):
                self.logger.error(f'    File was not blurred: {file_name}')
                file_full_path = os.path.join(self.before_blur_folder, file_name)
            if not os.path.isfile(file_full_path):
                continue
            with open(file_full_path, 'rb') as tmp_file:
                self.logger.debug(f'    Before store_file')
                file_url = await self._fs.store_file(tmp_file, file_name)
                self.logger.debug(f'    After store_file')
                id_to_url[file_id] = file_url
        self.logger.debug('Finish uploading to storage')

        # 5. fill urls in task_input
        for field_name, image_list in images_dict.items():
            task_input[field_name] = [id_to_url[image_id] for image_id in image_list]

        # 4. del files from all folders
        self.logger.debug('Clear temporary folders')
        if os.path.exists(self.after_blur_folder):
            for file_name in os.listdir(self.after_blur_folder):
                file_full_path = os.path.join(self.after_blur_folder, file_name)
                if os.path.isfile(file_full_path):
                    os.remove(file_full_path)

        if os.path.exists(self.before_blur_folder):
            for file_name in os.listdir(self.before_blur_folder):
                file_full_path = os.path.join(self.before_blur_folder, file_name)
                if os.path.isfile(file_full_path):
                    os.remove(file_full_path)

    async def create_new_tasks(self, pool_id, task: toloka.Task, solution: toloka.solution.Solution, assignment: toloka.Assignment = None, task_input={}) -> List[toloka.Task]:
        self.logger.debug(f'Start creating new task')
        val = None
        images_list = None
        all_images = {}
        for field_path, new_field_name in self._fields_mapping.items():

            if field_path.startswith('input.'):
                field_name = field_path[len('input.'):]
                val, images_list = await self.prepare_field_value(
                    task.input_values,
                    field_name,
                    pool_id=pool_id,
                    from_src='input',
                )
            elif field_path.startswith('output.'):
                field_name = field_path[len('output.'):]
                val, images_list = await self.prepare_field_value(
                    solution.output_values,
                    field_name,
                    pool_id=pool_id,
                )
            elif field_path.startswith('solution.'):
                field_name = field_path[len('solution.'):]
                val, images_list = await self.prepare_field_value(
                    solution.output_values,
                    field_name,
                    pool_id=pool_id,
                )
            elif field_path.startswith('assignment.'):
                path_in_assignment = field_path[len('assignment.'):]
                if path_in_assignment.tartswith('input.'):
                    field_name = path_in_assignment[len('input.'):]
                    val, images_list = await self.prepare_field_value(
                        assignment.tasks[0].input_values,
                        field_name,
                        pool_id=assignment.pool_id,
                        from_src='input',
                    )
                elif path_in_assignment.startswith('output.'):
                    field_name = path_in_assignment[len('output.'):]
                    val, images_list = await self.prepare_field_value(
                        assignment.solutions[0].output_values,
                        field_name,
                        pool_id=assignment.pool_id,
                )
                elif hasattr(assignment, path_in_assignment):
                    val = getattr(assignment, path_in_assignment)
                else:
                    assert f'wrong path defenition in assignment "{field_path}"'
            else:
                assert f'wrong path defenition "{field_path}"'

            if val is not None:
                task_input[new_field_name] = val
            if images_list is not None:
                all_images[new_field_name] = images_list

        await self.prepare_images(all_images, task_input)

        tasks = []
        tasks.append(toloka.Task(pool_id=self._pool_id, input_values=task_input))
        return tasks


class TaskResult:
    """
    Args:
        'task',
        'label',
        'performer',
        'url',
        'pedestrian_assignment_id',
        'group_id',
        'group_count',
        'image_field',
        'main_task',
        and all other what you want
    """
    def __init__(self, **kwargs):
        for name, val in kwargs.items():
            setattr(self, name, val)
