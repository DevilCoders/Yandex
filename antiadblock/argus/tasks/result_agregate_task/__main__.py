# -*- coding: utf8 -*-
import os
from datetime import datetime
from collections import defaultdict

import requests
from retry.api import retry
from tvmauth import TvmClient, TvmApiClientSettings
from sandbox.common import rest, proxy

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_service_ticket_for, RequestRetryableException, RequestFatalException


logger = create_logger("argus_result_agregate_task")

TVM_ID = int(os.getenv("TVM_ID", "2002631"))
TVM_SECRET = os.getenv("TVM_SECRET")

CONFIGSAPI_TEST_TVM_CLIENT_ID = 2000627
CONFIGSAPI_PROD_TVM_CLIENT_ID = 2000629
ARGUS_RESULT_TMP_URL = "http://argus.s3.mds.yandex.net/result_{}.json"


class SandboxTaskViewer:
    def __init__(self, limit=10):
        logger.info("Initialization SandboxApiClient...")
        self.client = rest.Client(
            base_url="https://sandbox.yandex-team.ru/api/v1.0",
            auth=proxy.OAuth(os.getenv("SB_TOKEN"))
        )
        logger.info("Client well initialed.")
        self.limit = limit
        self._task_ids = defaultdict(list)

    @property
    def task_ids(self):
        task_ids = []
        for status in self._task_ids:
            current_status = "success" if status == "SUCCESS" else "fail"
            for current_id in self._task_ids[status]:
                task_ids.append((current_status, current_id))
        return task_ids

    @retry(tries=3, delay=1, backoff=3, logger=logger, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
    def get_information_from_argus(self, task_id):
        response = requests.get(ARGUS_RESULT_TMP_URL.format(task_id))
        result = {
            "start_time": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S"),
            "end_time": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S"),
            "sandbox_id": task_id,
            "cases": [],
            "filters_lists": []
        }

        if response.status_code in (502, 503, 504):
            raise RequestRetryableException("Unable to make argus s3 request: response code is {}".format(response.status_code))

        logger.info("SandboxTask id: {}. Status code: {}".format(task_id, response.status_code))
        if response.status_code != 200:
            if task_id in self._task_ids['SUCCESS']:
                logger.error("Failed get request from Argus!")
            else:
                logger.warning("Task FAILED!")
        else:
            logger.info("Downloaded information from argus.")
            result = response.json()

        return result

    def mark_processed_task_done(self, task_id):
        self.client.task[task_id].update(tags=["DONE"])

    def mark_processed_task_fail(self, task_id):
        self.client.task[task_id].update(tags=["FAIL"])

    def collect_finished_argus_tasks(self, api_url, tvm_client):
        logger.info("Getting in_progress argus tasks from configs api...")
        items = get_sbs_runs_from_configs_api(api_url, tvm_client)["items"].get("in_progress", [])
        for item in items:
            try:
                status = self.client.task[item["sandbox_id"]].read()["status"]
                if status in ("SUCCESS", "EXCEPTION", "STOPPED", "FAILED", "TIMEOUT"):
                    self._task_ids[status].append(item["sandbox_id"])
            except rest.Client.HTTPError as ex:
                if ex.status == 404:
                    logger.info(ex)
                    self._task_ids["TIMEOUT"].append(item["sandbox_id"])
            except Exception as e:
                logger.info(str(e))
                continue


@retry(tries=3, delay=1, backoff=3, logger=logger, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
def get_sbs_runs_from_configs_api(api_url, tvm_client):
    ticket_for_configs_api = get_service_ticket_for(tvm_client, "configs_api", logger)
    response = requests.get(api_url, headers={'X-Ya-Service-Ticket': ticket_for_configs_api})

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make configs api request: response code is {}".format(response.status_code))

    if response.status_code != 200:
        raise RequestFatalException("Can not to get SBSRuns objects {code} {text}".format(code=response.status_code,
                                                                                          text=response.text))

    return response.json()


@retry(tries=3, delay=1, backoff=3, logger=logger, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
def save_argus_result_to_api(api_url, tvm_client, json_data):
    ticket_for_configs_api = get_service_ticket_for(tvm_client, "configs_api", logger)
    response = requests.post(api_url, headers={'X-Ya-Service-Ticket': ticket_for_configs_api}, json=json_data)

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make configs api request: response code is {}".format(response.status_code))

    if response.status_code != 201:
        logger.info("Failed post data {code} {text}".format(code=response.status_code, text=response.text))
        return True

    return False


if __name__ == "__main__":
    if os.getenv("ENV_TYPE", "PRODUCTION") == "PRODUCTION":
        configs_api_tvm_id = CONFIGSAPI_PROD_TVM_CLIENT_ID
        configs_api_host = "api.aabadmin.yandex.ru"
    elif os.getenv("ENV_TYPE") == "TESTING":  # TODO DELETE
        configs_api_tvm_id = CONFIGSAPI_TEST_TVM_CLIENT_ID
        configs_api_host = "api-aabadmin44.n.yandex-team.ru"
    else:
        configs_api_tvm_id = CONFIGSAPI_TEST_TVM_CLIENT_ID
        configs_api_host = "preprod.aabadmin.yandex.ru"

    configs_api_url = "https://{}/sbs_check".format(configs_api_host)
    logger.info("ConfigsApiUrl {}".format(configs_api_url))
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configs_api_tvm_id)
    )

    tvm_client = TvmClient(tvm_settings)

    sandbox_task = SandboxTaskViewer()
    sandbox_task.collect_finished_argus_tasks(configs_api_url + '/runs', tvm_client)
    do_fail = False
    for (current_status, current_id) in sandbox_task.task_ids:
        data = sandbox_task.get_information_from_argus(current_id)
        data.update({"status": current_status})

        is_failed_post = save_argus_result_to_api(configs_api_url + '/results', tvm_client, data)

        if is_failed_post:
            logger.info("Mark processed task {} FAIL.".format(current_id))
            sandbox_task.mark_processed_task_fail(current_id)
        else:
            logger.info("Mark processed task {} DONE.".format(current_id))
            sandbox_task.mark_processed_task_done(current_id)

        do_fail = do_fail or is_failed_post

    if do_fail:
        raise Exception("Some tasks failed.")
