# coding: utf-8

from __future__ import unicode_literals

from itertools import groupby
import logging
import six

from celery.task import task

from django.db import transaction
from django.utils import timezone
from ylog.context import log_context

from .lock import get_lock_or_do_nothing
from ..utils import import_dir_organization, ensure_service_ready
from ..dir_client import (DirClient, DirClientError, OrganizationNotFoundError, OrganizationAlreadyDeletedError,
                          ServiceIsNotEnabledError)
from .. import signals  # не надо удалять этот импорт, он используется в коде отправки сигналов
from ..models import ChangeEvent, Organization
from ..logic import context_from_dir_event, OrganizationSyncStatistics, make_sync_random_for_req_id, \
    disable_organization
from wiki.utils.tasks.base import LockedCallableTask


logger = logging.getLogger(__name__)


@transaction.atomic
def process_change_event(revision_num, events, dir_org_id):
    last_change_event = ChangeEvent.objects.select_for_update().get(org__dir_id=dir_org_id)
    if last_change_event.revision and last_change_event.revision >= revision_num:
        # получили событие которое уже обрабатывали.
        return
    try:
        for event in events:
            signal_name = event['name'].lower()

            with log_context(celery_task_name='sync_dir_data_changes', **context_from_dir_event(event)):
                logger.info('Process change event')

                try:
                    signal = getattr(signals, signal_name)
                except AttributeError:
                    msg = 'There is no signal for event "%s"'
                    logger.warn(msg, event['name'])
                else:
                    # WIKI-10608: костыль для обработки устаревшего формата события
                    # TODO: можно удалить костыль в марте 2018 года, так как события в базе Директории живут 180 дней
                    if signal_name == 'domain_master_changed' and isinstance(event['object'], six.string_types):
                        event['object'] = {'org_id': dir_org_id, 'name': event['object']}

                    # к нам сейчас события прилетают без атрибута org_id, поэтому чтобы вписаться в наш общий формат
                    # обработки событий - добавим в объект этот атрибут
                    if signal_name in ('service_enabled', 'service_disabled'):
                        event['object']['org_id'] = dir_org_id

                    # https://st.yandex-team.ru/DIR-4513
                    # У события с типом group_membership_changed может отсутствовать секция object,
                    # поэтому сейчас чтобы не падать при обработке таких событий добавим object с id орг-ции
                    if signal_name == 'group_membership_changed' and not 'object' in event:
                        event['object'] = {'org_id': dir_org_id, 'id': event['id']}

                    signal.send(
                        sender=sync_dir_data_changes,
                        object=event['object'],
                        content=event['content'],
                    )
                    logger.info('The change event was processed')
    except Exception:
        raise
    else:
        last_change_event.revision = revision_num
        last_change_event.last_pull_at = timezone.now()
        last_change_event.save()


class ProcessChangeEventForOrgTask(LockedCallableTask):
    """
    Проверить наличие событий об изменениях в структуре организации в Директории с необработанными номерами ревизий
    и при наличии таковых импортировать изменения.
    Задача запускается для организации с переданным dir_org_id.
    """
    name = 'wiki.sync_dir_data_changes_for_org'
    logger = logging.getLogger(name)
    time_limit = 60 * 30  # 1800 сек
    lock_name_tpl = 'sync_dir_data_changes_{dir_org_id}'

    def run(self, dir_org_id, req_id_from_dir=None, *args, **kwargs):
        dir_client = DirClient(
            sync_random=make_sync_random_for_req_id(),
            req_id_from_dir=req_id_from_dir
        )

        with log_context(celery_task_name='sync_dir_data_changes_for_org', req_id=dir_client.get_req_id()):
            logger.info('Sync changes of data from Directory for organization: %s', dir_org_id)

            try:
                org = dir_client.get_organization(dir_org_id)

                # проверим, что организация не заблокирована в Директории
                if org['is_blocked'] == True:
                    # блокируем организацию и у нас и обновления не обрабатываем.
                    logger.info('Organization with dir_id={} is blocked in Directory.'.format(dir_org_id))
                    disable_organization(dir_org_id)
                    return

                # проверим, что организация существует в базе Вики
                if not Organization.objects.filter(dir_id=dir_org_id).exists():
                    organization = import_dir_organization(org)
                    if not organization:
                        logger.error('Could not import organization %s', dir_org_id)
                        return
            except (OrganizationAlreadyDeletedError, ServiceIsNotEnabledError):
                disable_organization(dir_org_id)
                return
            except DirClientError as err:
                logger.warning('Can\'t check status of organization with dir_id=%s: %s', dir_org_id, repr(err))
                return

            revision = ChangeEvent.objects.get(org__dir_id=dir_org_id).revision

            with log_context(dir_org_id=str(dir_org_id), last_revision=str(revision),
                             req_id=dir_client.get_req_id(str(dir_org_id))):
                try:
                    params = {}
                    if revision:
                        params['revision__gt'] = revision
                    events = dir_client.get_events(dir_org_id, **params)
                except OrganizationNotFoundError:
                    return
                except DirClientError as err:
                    content = err.response.json()
                    if err.response.status_code == 403 and (
                                    content.get('code') == 'service_was_disabled' or
                                    content.get('code') == 'service_is_not_enabled'):
                        # у организации в Директории отключен наш сервис. Отключим организацию в нашей базе,
                        # если она не отключена
                        disable_organization(dir_org_id)
                    else:
                        # TODO: синхронизация организации сломана, отражать это в статистике
                        logger.exception(
                            'Error while retrieving events for organization dir_id=%s: "%s"', dir_org_id, repr(err))
                    return
                except Exception as err:
                    # TODO: синхронизация организации сломана, отражать это в статистике
                    logger.exception(
                        'Error while retrieving events for organization dir_id=%s: "%s"', dir_org_id, repr(err))
                    return

                if events:
                    try:
                        with OrganizationSyncStatistics(dir_org_id):
                            for revision_num, events_iter in groupby(events, key=lambda e: e['revision']):
                                process_change_event(revision_num, list(events_iter), dir_org_id)
                    except Exception as err:
                        logger.exception('Error processing of change events of organization %s: "%s"',
                                         dir_org_id, repr(err))


@task(name='dir_data_sync.sync_dir_data_changes')
@get_lock_or_do_nothing('sync_dir_data_changes')
def sync_dir_data_changes(dir_org_ids_list=None, req_id_from_dir=None):
    """
    Проверить наличие событий об изменениях в структуре организации в Директории с необработанными номерами ревизий
    и при наличии таковых импортировать изменения.
    Задача запускается для всех организаций.
    """
    logger.info('Sync changes of data from Directory for organizations: %s', (dir_org_ids_list or 'all'))

    if dir_org_ids_list is None:
        dir_org_ids_list = Organization.objects.all().values_list('dir_id', flat=True)
    else:
        dir_client = DirClient(
            sync_random=make_sync_random_for_req_id(),
            req_id_from_dir=req_id_from_dir
        )

        try:
            dir_org_ids_list = [dir_org_id for dir_org_id in dir_org_ids_list
                                if dir_client.get_organization(dir_org_id)['is_blocked'] == False]
        except DirClientError as err:
            logger.warning('Can\'t check status of organization with dir_id=%s', repr(err))
            return

    for dir_org_id in dir_org_ids_list:
        ProcessChangeEventForOrgTask().delay(dir_org_id=dir_org_id, req_id_from_dir=req_id_from_dir)


@task(name='dir_data_sync.ensure_service_ready')
def ensure_service_ready():
    ensure_service_ready()
