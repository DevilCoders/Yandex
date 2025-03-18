# coding: utf-8

from __future__ import unicode_literals

import logging
from datetime import timedelta

from .errors import UnknownOperatingModeError
from .models import Organization, SyncStatistics, OperatingMode


logger = logging.getLogger(__name__)


def sync_started(dir_org_id):
    """
    @rtype: SyncStatistics
    """
    sync = SyncStatistics.objects.filter(dir_org_id=dir_org_id).first()
    if not sync:
        sync = SyncStatistics(
            dir_org_id=dir_org_id,
            last_pull_status=SyncStatistics.PULL_STATUSES.failed,
            last_pull_duration=timedelta(seconds=0),
        )
        sync.save()
    return sync


def get_organization(dir_id):
    return Organization.objects.filter(dir_id=dir_id).first()


def get_operating_mode(name):
    try:
        return OperatingMode.objects.get(name=name)
    except OperatingMode.DoesNotExist:
        msg = 'Operating Mode with name=%s does not exist'
        logger.error(msg, name)
        raise UnknownOperatingModeError(msg, name)
