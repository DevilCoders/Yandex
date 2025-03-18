# coding: utf-8
from __future__ import unicode_literals

from appconf import AppConf

# noinspection PyUnresolvedReferences
from django.conf import settings


class PGAASAppConf(AppConf):
    class Meta:
        prefix = 'PGAAS'

    COLLATION = 'COLLATE "en_US"'
    RETRY_ATTEMPTS = 3
    RETRY_SLOT = .125  # seconds
    USE_ZDM = False
