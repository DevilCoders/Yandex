import os
import random
import time

from sandbox.common.proxy import OAuth
from sandbox.common.rest import Client as NativeSandboxClient
from sandbox.common.types.task import Status


_SANDBOX_URL = 'https://sandbox.yandex-team.ru/'


class SandboxClient:

    class Error(Exception):
        pass

    class Task:

        def __init__(self, id: int):
            self.id = id

        @property
        def url(self):
            return os.path.join(_SANDBOX_URL, 'task', str(self.id))

    def __init__(self, token, logger):
        self._sandbox_client = NativeSandboxClient(
            base_url=os.path.join(_SANDBOX_URL, 'api/v1.0'),
            auth=OAuth(token),
            logger=logger
        )

    def create_task(self, params) -> Task:
        return self.Task(self._sandbox_client.task(params)['id'])

    def update_task_custom_fields(self, task: Task, custom_fields: dict) -> None:
        self._sandbox_client.task[task.id] = {
            'custom_fields': [{'name': n, 'value': v} for n, v in custom_fields.items()]
        }

    def start_task(self, task: Task) -> None:
        response = self._sandbox_client.batch.tasks.start.update(task.id)[0]
        if response['status'] not in ['SUCCESS', 'WARNING']:
            raise self.Error('failed to start task (id=%s): %s' % (task.id, response['message']))

    def wait_task(self, task: Task) -> bool:
        status = None
        while status not in Status.Group.FINISH | Status.Group.BREAK:
            status = self._sandbox_client.task[task.id].read()['status']
            time.sleep(10 + random.randint(-5, 5))
        return status == Status.SUCCESS

    def fetch_resources(self, task: Task):
        return self._sandbox_client.task[task.id].resources.read()['items']
