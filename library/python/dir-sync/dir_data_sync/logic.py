# coding: utf-8

from __future__ import unicode_literals

import logging
import random
import six

from django.utils import timezone
from django.conf import settings

from ylog.context import log_context

from .dao import sync_started, get_organization
from .dir_client import DirClient
from .models import Organization
from .org_ctx import org_ctx, get_user_orgs, org_user


logger = logging.getLogger(__name__)


def context_from_dir_event(event):
    """
    Попытаться достать хоть какой-нибудь полезный для логгирования контекст

    @type event: dict
    @rtype: dict
    """
    result = {
        'dir_org_id': event.get('org_id', 'unknown'),
        'dir_event_name': event['name'],
        'dir_object_id': event['object'].get('id', 'unknown')
        if isinstance(event['object'], dict) else 'unknown',
    }
    fields = []
    if event['name'].startswith('user_'):
        fields = ['email', 'is_dismissed']
    elif event['name'].startswith('department_') or event['name'].startswith('group_'):
        fields = ['email', 'type', 'label']
    if isinstance(event['object'], dict):
        for field in fields:
            if field in event['object']:
                try:
                    result['dir_' + field] = six.text_type(event['object'][field])
                except Exception:
                    logger.exception('Could not parse field "%s"', field)

    return result


class OrganizationSyncStatistics(object):
    """
    Контекстный менеджер который считает статистику синхронизации Организаций.
    """
    started_at = None
    sync_statistics = None
    _dir_org_id = None

    def __init__(self, dir_org_id):
        self._dir_org_id = dir_org_id

    def __enter__(self):
        self.started_at = timezone.now()
        self.sync_statistics = sync_started(self._dir_org_id)

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is not None:
            self.sync_statistics.last_pull_status = self.sync_statistics.PULL_STATUSES.failed
        else:
            self.sync_statistics.last_pull_status = self.sync_statistics.PULL_STATUSES.success
            self.sync_statistics.successful_attempts += 1
        self.sync_statistics.last_pull_duration = timezone.now() - self.started_at
        self.sync_statistics.save()


def make_sync_random_for_req_id():
    """
    Создать ID для текущей сессии синхронизации.
    """
    return random.randrange(10000000, 100000000)


def is_org_imported(sync_stat, dir_org_ids):
    """
    Возвращает True, если организация была проимпортирована.
    """
    return sync_stat.successful_attempts > 0 or sync_stat.dir_org_id in dir_org_ids


def delete_org_data(dir_org_id):
    # User удаляем отдельно, т.к. они M-to-N к Organization.
    org = Organization.objects.get(dir_id=dir_org_id)
    with org_ctx(org):
        users = org_user()
        for user in users:
            # Если пользователь остался только в одной организации,
            # то его можно удалить.
            if len(get_user_orgs(user)) == 1:
                user.delete()

    # Остальные сущности удалятся каскадно при удалении Organization.
    org.delete()


def is_user_has_access_to_service(dir_org_id, dir_user_id):
    """
    Проверить, подключен ли сервис в организации пользователя.

    @raise UserNotFoundError если пользователь не найден в организации с переданным id или не найдена сама организация
    """
    dir_client = DirClient(
        sync_random=make_sync_random_for_req_id()
    )
    user = dir_client.get_user(dir_org_id, dir_user_id, fields='services')

    for service in user['services']:
        if service['slug'] == settings.DIRSYNC_SERVICE_SLUG:
            return True

    return False


def disable_organization(dir_org_id):
    org = get_organization(dir_org_id)
    if org.status != Organization.ORG_STATUSES.disabled:
        with log_context(dir_org_id=dir_org_id):
            org.status = Organization.ORG_STATUSES.disabled
            org.save()
            logger.info("Organization with dir_id=%s was disabled", dir_org_id)


def enable_organization(dir_org_id):
    org = get_organization(dir_org_id)
    if org.status == Organization.ORG_STATUSES.enabled:
        return

    dir_client = DirClient(
        sync_random=make_sync_random_for_req_id(),
        timeout=30
    )

    with log_context(dir_org_id=dir_org_id, req_id=dir_client.get_req_id(dir_org_id)):
        org.status = Organization.ORG_STATUSES.enabled
        org.save()

        dir_client.post_service_ready(
            service_slug=settings.DIRECTORY_SERVICE_SLUG,
            dir_org_id=dir_org_id,
        )

        logger.info("Organization with dir_id=%s was enabled", dir_org_id)
