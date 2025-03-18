from getpass import getuser
from time import sleep
from copy import deepcopy

from . import SweeperApi, FMLViewApi, FMLDownloadApi
from .exceptions import SweeperError
from .utils import BaseSerializable, is_array


class SweeperTaskStatus(object):
    completed = 'COMPLETED'
    running = 'RUNNING'
    canceled = 'CANCELED'
    failed = 'FAILED'
    created = 'CREATED'
    deleted = 'DELETED'


class SweeperTaskResult(BaseSerializable):
    def __init__(self, result_dict):
        self._result_dict = result_dict

    def to_dict(self):
        return deepcopy(self._result_dict)

    def _generic_optimize(self, target, minimize=True, constraints=None):
        target_function_definition = self.to_dict()['target-function-definition']
        if target not in target_function_definition['output-space'].keys():
            raise ValueError('Unknown target: {}'.format(target))

        # merge all points from different passes
        all_points = []
        for p in self.to_dict()['passes']:
            all_points += p['points']

        # apply constraints if any
        if constraints is not None:
            all_points = [p for p in all_points if constraints(p['inputs'], p['outputs'])]

        if minimize:
            optimal_target = min([p['outputs'][target] for p in all_points])
        else:
            optimal_target = max([p['outputs'][target] for p in all_points])

        optimal_target_arg = [p['inputs'] for p in all_points if p['outputs'][target] == optimal_target]
        return optimal_target, optimal_target_arg

    def min(self, target, constraints=None):
        return self._generic_optimize(target, minimize=True, constraints=constraints)[0]

    def argmin(self, target, constraints=None):
        return self._generic_optimize(target, minimize=True, constraints=constraints)[1]

    def max(self, target, constraints=None):
        return self._generic_optimize(target, minimize=False, constraints=constraints)[0]

    def argmax(self, target, constraints=None):
        return self._generic_optimize(target, minimize=False, constraints=constraints)[1]


class SweeperTask(object):
    def __init__(self, sweeper_client, id, progress_id, polling_period=30):
        self._client = sweeper_client
        self._id = id
        self._progress_id = progress_id
        self._polling_period = polling_period
        self._result = None

    @property
    def id(self):
        return self._id

    @property
    def url(self):
        return "https://fml.yandex-team.ru/sweeper/task/{}".format(self.id)

    @property
    def progress_id(self):
        return self._progress_id

    @property
    def progress_url(self):
        return "https://fml.yandex-team.ru/view/experiment/graph?progress-id={}".format(self.progress_id)

    @property
    def result(self):
        return self._result

    def _get_progress(self):
        return self._client._fml_view_api.do_request({"progress-id": self.progress_id})

    def wait_result(self, verbose=False):
        """
        Throws exception if task is failed or cancelled
        :return: None
        """
        while True:
            progress = self._get_progress()
            if verbose:
                print progress
            status = progress['status']
            if status == SweeperTaskStatus.completed:
                self._result = SweeperTaskResult(self._client._fml_download_api.do_request({"id": self.id}))
                return
            elif status in (SweeperTaskStatus.canceled, SweeperTaskStatus.failed, SweeperTaskStatus.deleted):
                raise SweeperError("Task {id} failed, status: {status}. See result in {url}".format(id=self.id, status=status, url=self.url))
            sleep(self._polling_period)


class SweeperClient(object):
    def __init__(self, token):
        self._api = SweeperApi(token)
        self._fml_view_api = FMLViewApi(token)
        self._fml_download_api = FMLDownloadApi(token)

    def create_task(
        self,
        function,
        passes,
        allowed_failed_rate=0.05,
        max_number_of_parallel_trials=40,
        max_duration_hours=100,
        is_async=True,
        owner=None,
        description=None,
        notify=None
    ):
        create_task_dict = {}
        create_task_dict.update(function.to_dict())
        create_task_dict["sweeps"] = [p.to_dict() for p in passes]
        create_task_dict["allowed-failed-rate"] = allowed_failed_rate
        create_task_dict["max-number-of-parallel-trials"] = max_number_of_parallel_trials
        create_task_dict["max-duration-hours"] = max_duration_hours

        meta = {}
        if owner:
            meta["owner"] = owner
        else:
            meta["owner"] = getuser()

        if description:
            meta["description"] = description

        create_task_dict["meta"] = meta

        cc_users = []
        if notify is None:
            cc_users = [meta["owner"]]
        elif not notify:
            pass
        elif is_array(notify):
            cc_users = notify
        create_task_dict["notifications"] = {
            'events': [SweeperTaskStatus.completed, SweeperTaskStatus.failed],
            "channels": ["EMAIL"],
            "ccUsers": cc_users,
        }

        r = self._api.do_request(create_task_dict)
        task = SweeperTask(self, id=r.get('parentId'), progress_id=r.get('id'))
        if not is_async:
            task.wait_progress()
        return task
