# coding: utf-8
from __future__ import unicode_literals

from django.conf import settings


CELERY_MONITORING_WIKI_URL = getattr(settings, 'CELERY_MONITORING_WIKI_URL', None)
CELERY_MONITORING_EXPIRE_DAYS = getattr(settings, 'CELERY_MONITORING_EXPIRE_DAYS', 7)
CELERY_MONITORING_ENABLE_ADMIN_LOGGING = getattr(
    settings,
    'CELERY_MONITORING_ENABLE_ADMIN_LOGGING',
    True,
)
CELERY_MONITORING_ADMIN_TASK_NAME_DISPLAY = getattr(
    settings,
    'CELERY_MONITORING_ADMIN_TASK_NAME_DISPLAY',
    lambda x: x,
)
