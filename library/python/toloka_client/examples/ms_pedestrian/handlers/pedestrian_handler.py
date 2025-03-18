import asyncio
import multiprocessing
from configparser import ConfigParser
#from file_store import FileStorer
import keyring
import logging
from logging.handlers import TimedRotatingFileHandler
from multiprocessing import Process, Queue
from pathlib import Path
import time
from toloka import client as toloka
from toloka.client.assignment import Assignment
from toloka import streaming
from toloka.util.async_utils import AsyncMultithreadWrapper
from typing import List


from .base_handler import BaseHandler, NewTask
from .base_handler import LevelFilter


class PedestrianHandler(BaseHandler):
    """ Get submitted assignment from pedestrian project. (init by last submit time)
    And send it to ValidationVisit project"""

    _task_generator: NewTask

    def __init__(self, toloka_client, tracking_pool_id, next_pool_id=None, ruled_pool_id=None, fs=None, tracker=None, n_of_procs=0):
        super().__init__(toloka_client, tracking_pool_id, next_pool_id=next_pool_id, ruled_pool_id=ruled_pool_id, fs=fs, tracker=tracker)
        self.processed_ids = None

        self.event_queue = Queue()
        assert n_of_procs > 0
        for i in range(n_of_procs):
            task_creator_process = Process(target=PedestrianHandler.task_creator_process,
                                           args=(self.event_queue, next_pool_id, tracking_pool_id))
            task_creator_process.start()



    @staticmethod
    async def async_task_creator(event_queue, my_task_generator, toloka_client, current_logger):
        current_logger.debug('start main loop')
        while True:
            if not event_queue:
                time.sleep(1)
                continue

            new_visit_tasks = []
            event = event_queue.get()
            current_logger.info('========== ASSIGNMENT  ID' + str(event.assignment.id) + '==========')
            new_visit_tasks += await my_task_generator.create_new_tasks(
                event.assignment.pool_id,
                event.assignment.tasks[0],
                event.assignment.solutions[0],
                task_input={'pedestrian_assignment_id': event.assignment.id}
            )

            create_result = await toloka_client.create_tasks(new_visit_tasks, allow_defaults=True, open_pool=True)
            current_logger.info(f'Created {len(create_result.items)} new tasks in Validation Visit Project (assignment_id = {event.assignment.id})')
            if hasattr(create_result, 'validation_errors') and create_result.validation_errors:
                current_logger.error(f'Problems on creating tasks: {str(create_result.validation_errors)}')

    @staticmethod
    def task_creator_process(event_queue, next_pool_id, tracking_pool_id):
        process_id = multiprocessing.current_process().ident
        logs_folder = f'logs/task_creator_process_{tracking_pool_id}_{process_id}'
        Path(logs_folder).mkdir(parents=True, exist_ok=True)
        current_logger = logging.getLogger(f'task_creator_process_{tracking_pool_id}')
        #current_logger.setLevel(logging.INFO)  # if you want DEBUG level, change it here
        current_logger.setLevel(logging.DEBUG)  # if you want DEBUG level, change it here
        current_logger.propagate = False
        fh0 = TimedRotatingFileHandler(f'{logs_folder}/debug.log', when='midnight', backupCount=14, utc=True)
        fh0.setLevel(logging.DEBUG)
        fh0.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        current_logger.addHandler(fh0)

        fh1 = TimedRotatingFileHandler(f'{logs_folder}/info.log', when='midnight', backupCount=14, utc=True)
        fh1.setLevel(logging.INFO)
        fh1.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        fh1.addFilter(LevelFilter([logging.INFO, logging.WARNING, logging.ERROR]))
        current_logger.addHandler(fh1)

        CONFIG = ConfigParser()
        CONFIG.read('toloka.ini')

        def get_secret(key):
            token_name = CONFIG.get('Secrets', key)
            return keyring.get_password('TOLOKA', token_name)

        my_toloka_client = toloka.TolokaClient(get_secret('toloka_token'), 'PRODUCTION')
        async_toloka_client = AsyncMultithreadWrapper(my_toloka_client)

        if CONFIG.has_section('S3'):
            from file_store_s3 import S3FileStorer
            file_storer = S3FileStorer(
                key_id=get_secret('s3_key_id'),
                access_key=get_secret('s3_access_key'),
                bucket_name=CONFIG.get('S3', 'bucket_name'),
                s3_url=CONFIG.get('S3', 'url'),
            )
        else:
            from file_store_abs import AzureFileStorer
            file_storer = AzureFileStorer(
                account_key=get_secret('account_key'),
                account_name=CONFIG.get('AzureBlobStorage', 'account_name'),
                container_name=CONFIG.get('AzureBlobStorage', 'container_name'),
                sas_valid_days=CONFIG.getint('AzureBlobStorage', 'sas_url_valid_days'),
            )

        import shutil
        shutil.copytree('files/blur', f'files/files_{process_id}/blur')

        my_task_generator = NewTask(
            {
                'input.name': 'name',
                'input.Segment': 'Segment',
                'input.address': 'address',
                'input.coordinates': 'coordinates',
                'output.verdict': 'verdict',
                'output.comment': 'comment',
                'output.comment_hoo': 'reason_no_photos',
                'output.worker_coordinates': 'worker_coordinates',
                # images
                'output.imgs_facade': 'imgs_facade',
                'output.name_business': 'name_business',
                'output.hoo_business': 'hoo_business',
                'output.imgs_name_address': 'imgs_name_address',
                'output.imgs_around': 'imgs_around',
                # images for quality and relevance calidation projects
                'output.imgs_interior': 'imgs_interior',
                'output.imgs_covid_policy': 'imgs_covid_policy',
                'output.imgs_outdoor_seating': 'imgs_outdoor_seating',
                'output.imgs_menu': 'imgs_menu',
            },
            async_toloka_client,
            next_pool_id,
            file_storer,
            current_logger,
            path_to_blur_exe=f'files/files_{process_id}/blur/AzureCognitiveServiceClient.exe',
            before_blur_folder=f'files/files_{process_id}/before_blur',
            after_blur_folder=f'files/files_{process_id}/after_blur',
        )
        asyncio.run(PedestrianHandler.async_task_creator(event_queue, my_task_generator, async_toloka_client, current_logger))

    async def __call__(self, assignment_events: List[streaming.event.AssignmentEvent]) -> None:
        self._logger.info(f'{len(assignment_events)} new assignments')

        if self.processed_ids is None:
            self.processed_ids = set()
            for task in await self._toloka_client.get_tasks(pool_id=self._next_pool_id):
                if task.known_solutions:
                    continue
                self.processed_ids.add(task.input_values['pedestrian_assignment_id'])

        for event in assignment_events:
            if event.assignment.status != Assignment.SUBMITTED:
               continue

            if event.assignment.id in self.processed_ids:
                continue

            inputs = event.assignment.tasks[0].input_values
            self._tracker.add_event({
                'name': inputs['name'],
                'address': inputs['address'],
                'pedestrian_assignment_id': event.assignment.id,
                'handler': 'pedestrian',
                'group': '',
                'event': 'submit',
                'comment': f'organization exist: {event.assignment.solutions[0].output_values["verdict"]}',
            })

            self.event_queue.put(event)
