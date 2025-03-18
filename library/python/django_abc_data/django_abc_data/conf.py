# -*- coding: utf-8 -*-
from django.conf import settings  # noqa

from appconf import AppConf


class AbcDataAppConf(AppConf):
    API_VERSION = 3
    ABC_DATA_IDS_OAUTH_TOKEN = None
    ABC_DATA_TVM_CLIENT_ID = None
    IDS_PAGE_SIZE = 500
    IDS_TIMEOUT = 10
    MIN_LOCK_TIME = 0
    SYNCER = 'django_abc_data.syncer.AbcServiceSyncer'
    GENERATOR = 'django_abc_data.generator.AbcServiceGenerator'

    class Meta:
        prefix = u'abc_data'
