# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django_abc_data.compat import import_string
from django_abc_data.conf import settings


def sync_services(**kwargs):
    defaults = {
        'create': True,
        'update': True,
        'delete': False,
    }
    defaults.update(kwargs)
    syncer_class = import_string(settings.ABC_DATA_SYNCER)
    syncer = syncer_class(**defaults)
    syncer.execute()
