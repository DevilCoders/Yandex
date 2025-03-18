# coding: utf-8
from __future__ import unicode_literals

from django.conf import settings

IDM_URL_PREFIX = getattr(settings, 'IDM_URL_PREFIX', 'idm/')
IDM_API_TVM_SETTINGS = getattr(settings, 'IDM_API_TVM_SETTINGS', {})
