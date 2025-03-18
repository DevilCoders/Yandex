# coding: utf-8

from celery.task import task
from celery.utils.log import get_task_logger

from ylog.context import log_context

from .lock import get_lock_or_do_nothing
from ..logic import make_sync_random_for_req_id
from ..utils import import_dir_organization
from ..dir_client import DirClient
from ..models import Organization


logger = get_task_logger(__name__)


@task(name='dir_data_sync.create_new_organization', ignore_result=True)
def create_new_organization(dir_org_obj):
    """
    Задача создания новой организации на основе данных организации из Директории.
    """
    with log_context(celery_task_name='create_new_org', dir_org_id=dir_org_obj['id']):
        logger.info('New organization with id=%s enabled service in Directory. Try import it', dir_org_obj['id'])
        import_dir_organization(dir_org_obj)


@task(name='dir_data_sync.sync_new_organizations', ignore_result=True, time_limit=600)
@get_lock_or_do_nothing('sync_new_organizations')
def sync_new_organizations():
    """
    Найти организации в Директории, которые уже подключили у себя сервис, но про которые сам сервис еще не знает.
    Поставить для каждой такой организации отложенную задачу по импорту ее данных в базу сервиса.
    """
    dir_client = DirClient(
        sync_random=make_sync_random_for_req_id()
    )

    with log_context(celery_task_name='sync_new_organizations', req_id=dir_client.get_req_id()):
        dir_org_ids = Organization.objects.values_list('dir_id', flat=True)
        org_list_from_dir = dir_client.get_organizations(**{"service.ready": False})
        org_map_from_dir = {str(org['id']): org for org in org_list_from_dir}

        not_existed_ids = set(org_map_from_dir.keys()) - set(dir_org_ids)

        for dir_org_id in not_existed_ids:
            dir_org_obj = org_map_from_dir[dir_org_id]
            create_new_organization.delay(dir_org_obj)
