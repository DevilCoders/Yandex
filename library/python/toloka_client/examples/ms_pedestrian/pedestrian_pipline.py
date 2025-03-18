import keyring # pip install keyring
import toloka.client as toloka  # pip install toloka-kit==0.1.17
from toloka import streaming
from toloka.streaming.event import AssignmentEvent
from toloka.util.async_utils import AsyncMultithreadWrapper

import asyncio
from configparser import ConfigParser
import datetime
import os

from handlers import PedestrianHandler, VisitHandler, QualityHandler, RelevanceHandler, CursorsSaver, DecisionTracker

SECRET_NAMESPACE = 'TOLOKA'

CONFIG = ConfigParser()
CONFIG.read('toloka.ini')

def get_secret(key):
    token_name = CONFIG.get('Secrets', key)
    return keyring.get_password(SECRET_NAMESPACE, token_name)

if __name__ == '__main__':
    # Create TolokaClient instance
    toloka_token = get_secret('toloka_token')
    TOLOKA_CLIENT = toloka.TolokaClient(toloka_token, 'PRODUCTION')
    print('\nStart with requester:', TOLOKA_CLIENT.get_requester().public_name['EN'])

    # Create class to store files
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

    async_toloka_client = AsyncMultithreadWrapper(TOLOKA_CLIENT)

    pedestrian_pool_id = CONFIG.get('PedestrianProject', 'pool_id')
    pedestrian_process_count = CONFIG.getint('PedestrianProject', 'process_count')
    visit_pool_id =  CONFIG.get('ValidationVisit', 'pool_id')
    visit_skill_id = CONFIG.get('ValidationVisit', 'skill_id')
    visit_task_overlap = CONFIG.getint('ValidationVisit', 'overlap')
    relevance_pool_id = CONFIG.get('ValidationRelevance', 'pool_id')
    relevance_skill_id = CONFIG.get('ValidationRelevance', 'skill_id')
    relevance_task_overlap = CONFIG.getint('ValidationRelevance', 'overlap')
    quality_pool_id = CONFIG.get('ValidationQuality', 'pool_id')
    quality_skill_id = CONFIG.get('ValidationQuality', 'skill_id')
    quality_task_overlap = CONFIG.getint('ValidationQuality', 'overlap')

    # All checks:
    # Check everything before start
    def check_update_pool(pool_id, tasks_count_on_page=0, golden_tasks_on_page=0):
        try:
            pool = TOLOKA_CLIENT.get_pool(pool_id)
            if tasks_count_on_page != 0 and pool.is_closed() and pool.mixer_config.real_tasks_count == 0:
                print('!!! rewrite mixer_config')
                pool.set_mixer_config(
                    real_tasks_count=tasks_count_on_page,
                    golden_tasks_count=golden_tasks_on_page,
                )
                TOLOKA_CLIENT.update_pool(pool_id, pool)
        except Exception as e:
            print(e)
            return False
        return True

    def check_skill(skill_id):
        try:
            TOLOKA_CLIENT.get_skill(skill_id)
        except:
            return False
        return True

    print('Check pools existing:')
    pedestrian_pool_exist = check_update_pool(pedestrian_pool_id)
    print(f'   Pedestrian pool - {pedestrian_pool_exist}')

    visit_pool_exist = check_update_pool(
        visit_pool_id,
        CONFIG.getint('ValidationVisit', 'tasks_on_page'),
        CONFIG.getint('ValidationVisit', 'golds_on_page'),
    )
    print(f'   Visit pool - {visit_pool_exist}')

    relevance_pool_exist = check_update_pool(
        relevance_pool_id,
        CONFIG.getint('ValidationRelevance', 'tasks_on_page'),
        CONFIG.getint('ValidationRelevance', 'golds_on_page'),
    )
    print(f'   Relevance pool - {relevance_pool_exist}')

    quality_pool_exist = check_update_pool(
        quality_pool_id,
        CONFIG.getint('ValidationQuality', 'tasks_on_page'),
        CONFIG.getint('ValidationQuality', 'golds_on_page'),
    )
    print(f'   Quality pool - {quality_pool_exist}')

    print('Check skills existing:')
    visit_skill_exist = check_skill(visit_skill_id)
    print(f'   Visit skill - {visit_skill_exist}')

    relevance_skill_exist = check_skill(relevance_skill_id)
    print(f'   Relevance skill - {relevance_skill_exist}')

    quality_skill_exist = check_skill(quality_skill_id)
    print(f'   Quality skill - {quality_skill_exist}')

    assert all([pedestrian_pool_exist, visit_pool_exist, relevance_pool_exist, quality_pool_exist, quality_pool_exist, 
                visit_skill_exist, relevance_skill_exist, quality_skill_exist])

    # Build pipeline.
    # Sample pipeline sheme:
    #   pedestrian -> visit -> relevance -> quality
    # More detailed description here: https://miro.com/app/board/o9J_kziXwOU=/?invite_link_id=731259125202
    print('\nCreate pipeline')
    pipeline = streaming.Pipeline(
        period=datetime.timedelta(seconds=60),  # you can specify any time delta between pipeline steps
    )

    cursors_saver = CursorsSaver()

    decision_tracker = DecisionTracker()

    pedestrian_observer = streaming.AssignmentsObserver(async_toloka_client, pool_id=pedestrian_pool_id)
    pedestrian_handler = PedestrianHandler(
        toloka_client=async_toloka_client,
        tracking_pool_id=pedestrian_pool_id,
        next_pool_id=visit_pool_id,
        fs=file_storer,
        tracker=decision_tracker,
        n_of_procs=pedestrian_process_count,
    )
    pedestrian_observer.on_submitted(pedestrian_handler.__call__)

    visit_observer = streaming.AssignmentsObserver(async_toloka_client, pool_id=visit_pool_id)
    visit_handler = VisitHandler(
        toloka_client=async_toloka_client,
        tracking_pool_id=visit_pool_id,
        next_pool_id=quality_pool_id,
        skill_id=visit_skill_id,
        fs=file_storer,
        tracker=decision_tracker,
        task_overlap=visit_task_overlap,
    )
    visit_observer.on_accepted(visit_handler.__call__)
    cursors_saver.track_cursor(f'visit_cursor_{visit_pool_id}', visit_observer._callbacks[AssignmentEvent.Type.ACCEPTED].cursor, 'accepted_gte')

    quality_observer = streaming.AssignmentsObserver(async_toloka_client, pool_id=quality_pool_id)
    quality_handler = QualityHandler(
        toloka_client=async_toloka_client,
        tracking_pool_id=quality_pool_id,
        next_pool_id=relevance_pool_id,
        skill_id=quality_skill_id,
        fs=file_storer,
        tracker=decision_tracker,
        task_overlap=quality_task_overlap,
    )
    quality_observer.on_accepted(quality_handler.__call__)
    cursors_saver.track_cursor(f'quality_cursor_{quality_pool_id}', quality_observer._callbacks[AssignmentEvent.Type.ACCEPTED].cursor, 'accepted_gte')

    relevance_observer = streaming.AssignmentsObserver(async_toloka_client, pool_id=relevance_pool_id)
    relevance_handler = RelevanceHandler(
        toloka_client=async_toloka_client,
        tracking_pool_id=relevance_pool_id,
        skill_id=relevance_skill_id,
        fs=file_storer,
        tracker=decision_tracker,
        task_overlap=relevance_task_overlap,
    )
    relevance_observer.on_accepted(relevance_handler.__call__)
    cursors_saver.track_cursor(f'relevance_cursor_{relevance_pool_id}', relevance_observer._callbacks[AssignmentEvent.Type.ACCEPTED].cursor, 'accepted_gte')


    pipeline.register(pedestrian_observer)
    pipeline.register(visit_observer)
    pipeline.register(quality_observer)
    pipeline.register(relevance_observer)

    pipeline.register(cursors_saver)
    pipeline.register(decision_tracker)


    import logging
    logging.basicConfig(filename='logs/debug.log', level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

    print('\nRun pipeline')
    asyncio.run(pipeline.run())
