# -*- coding: utf-8 -*-
from django.apps import AppConfig
from model_utils import Choices


class DirDataSyncAppConfig(AppConfig):
    name = 'dir_data_sync'

    def ready(self):
        from . import signals


default_app_config = 'dir_data_sync.DirDataSyncAppConfig'


OPERATING_MODE_NAMES = Choices('free', 'paid')
