# coding=utf-8
from json import dumps
from retry import retry

import requests
from sandbox.common import rest
from sandbox.common.proxy import OAuth


class ArgusClient:

    ARGUS_S3_HOST = "argus.s3.mds.yandex.net"

    def __init__(self, oauth_token):
        self.client = rest.Client(auth=OAuth(oauth_token))

    def run_argus_task(self, profile, argus_run_id, argus_resource_id=None):
        # Создание задачи, метод POST /task
        task_id = self.client.task({"type": "ANTIADBLOCK_ARGUS_SCREENSHOTER"})["id"]
        # Заполнение свойств и входных параметров задачи, метод PUT /task/{id}
        task_data = {
            "owner": "ANTIADBLOCK",
            "kill_timeout": 40 * 60,  # seconds
            "important": False,
            "fail_on_any_error": False,
            "tags": ['NEW'],
            "custom_fields": [
                {"name": "url_profile", "value": dumps(profile)},
                {"name": "argus_run_id", "value": argus_run_id},
                {"name": "run_selenoid_as_subtask", "value": True},
                {"name": "one_subtask_per_browser", "value": True}
            ]
        }
        if argus_resource_id is not None:
            task_data["custom_fields"].append({"name": "argus_resource_id", "value": argus_resource_id})
        self.client.task[task_id] = task_data

        # Запуск задач, метод PUT /batch/tasks/start
        start_result = self.client.batch.tasks.start.update([task_id])[0]
        if start_result["status"] != "SUCCESS":
            raise Exception("Failed to start task: {}".format(start_result))

        return task_id

    @retry(tries=3, delay=1, backoff=3, exceptions=requests.exceptions.ConnectTimeout)
    def _get_json_from_s3(self, result_id):
        url = "http://{}/log_{}.json".format(self.ARGUS_S3_HOST, result_id)
        response = requests.get(url)
        response.raise_for_status()
        return response.json()

    def get_logs_list_from_s3(self, result_id, logs_type, request_id):
        sbs_logs = self._get_json_from_s3(result_id)
        return sbs_logs.get(logs_type, {}).get(request_id, [])
