# -*- coding: utf-8 -*-

import time

from celery.task import Task
from django.core.exceptions import ObjectDoesNotExist

from core.models import BackgroundTask


class SteamTask(Task):
    abstract = True

    def __call__(self, *args, **kwargs):
        self.bg_task = self.wait_task()
        return super(SteamTask, self).__call__(*args, **kwargs)

    def wait_task(self):
        while True:
            time.sleep(1)
            try:
                return BackgroundTask.objects.get(pk=str(self.request.id))
            except ObjectDoesNotExist:
                pass
