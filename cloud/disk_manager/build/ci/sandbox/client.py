import logging
import random
import time

import sandbox.common.types.task as ctt

from sandbox.common import rest
from sandbox.common.proxy import OAuth


class SandboxClient:
    def __init__(self, oauth):
        self.client = rest.Client(auth=OAuth(oauth))

    def run_task(self, task_params, **custom_fields):
        task_id = self.client.task.create(**task_params)['id']
        logging.info('Task id: https://sandbox.yandex-team.ru/task/%s', task_id)

        self.client.task[task_id] = {
            'custom_fields': [dict(name=key, value=value) for (key, value) in custom_fields.items()]
        }
        task = self.client.task[task_id]

        start_result = self.client.batch.tasks.start.update([task_id])[0]
        if start_result["status"] != "SUCCESS":
            raise Exception("Failed to start task: {}".format(start_result))
        logging.info('Task has started successfully')

        status = None
        while status not in ctt.Status.Group.FINISH | ctt.Status.Group.BREAK:
            status = task.read()["status"]
            time.sleep(10 + random.randint(-5, 5))

        if status != ctt.Status.SUCCESS:
            raise Exception("Task failed: {}".format(task_id))

        logging.info('Task has completed successfully')
        return task_id
