# coding: utf-8

from __future__ import unicode_literals

from django.conf import settings


IDS_DEFAULT_USER_AGENT = getattr(settings, 'IDS_DEFAULT_USER_AGENT', None)
IDS_DEFAULT_ROBOT_UID = getattr(settings, 'IDS_DEFAULT_ROBOT_UID', None)
IDS_DEFAULT_OAUTH_TOKEN = getattr(settings, 'IDS_DEFAULT_OAUTH_TOKEN', None)
