# coding=utf-8
# Утилита для тестового запуска Аргуса
import os
import time

import requests

from retry.api import retry
from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_service_ticket_for, RequestRetryableException, RequestFatalException

logger = create_logger('argus_test_run')

TVM_ID = int(os.getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv('TVM_SECRET')

CONFIGS_API_HOST = os.getenv('CONFIGS_API_HOST', 'api.aabadmin.yandex.ru')
CONFIGS_API_TVM_ID = int(os.getenv('CONFIGS_API_TVM_ID', '2000629'))  # Adminka production

ARGUS_RESOURCE_ID = os.getenv('ARGUS_RESOURCE_ID')
ARGUS_PROFILE_TAG = os.getenv('ARGUS_PROFILE_TAG')
SERVICE_ID = os.getenv('SERVICE_ID')

NUM_OF_LAUNCHES = int(os.getenv('NUM_OF_LAUNCHES', '1'))
LAUNCH_BACKOFF = float(os.getenv('LAUNCH_BACKOFF', '0.5'))


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
def run_argus(service_id, tvm_client, argus_resource_id, argus_profile_tag=None):
    assert service_id is not None, 'Service id must be specified'

    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api')
    url = 'https://{configs_api_host}/service/{service_id}/sbs_check/run'.format(
        configs_api_host=CONFIGS_API_HOST,
        service_id=service_id,
    )
    data = {'argus_resource_id': argus_resource_id}
    if argus_profile_tag is not None:
        data['tag'] = argus_profile_tag
    logger.info('POST {} with data: {}'.format(url, data))
    r = requests.post(url, json=data, headers={'X-Ya-Service-Ticket': tvm_ticket})
    if r.status_code in (502, 503, 504):
        raise RequestRetryableException('Unable to make configs api request: response code is {}'.format(r.status_code))
    if r.status_code != 201:
        raise RequestFatalException('{code} {text}'.format(code=r.status_code, text=r.text))
    return r.json()['run_id']

if __name__ == '__main__':
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGS_API_TVM_ID)
    )
    tvm_client = TvmClient(tvm_settings)

    failures = 0
    for i in range(1, NUM_OF_LAUNCHES + 1):
        logger.info('Trying to run argus with bin id {}. Launch {}'.format(ARGUS_RESOURCE_ID, i))
        try:
            run_id = run_argus(
                SERVICE_ID,
                tvm_client,
                argus_resource_id=ARGUS_RESOURCE_ID,
                argus_profile_tag=ARGUS_PROFILE_TAG,
            )
        except Exception as e:
            logger.error(e)
            failures += 1
            if isinstance(e, RequestFatalException):
                break
        else:
            logger.info('Run argus with run id {}'.format(run_id))
            logger.info('https://antiblock.yandex.ru/service/{}/screenshots-checks/diff/{}/{}'.format(SERVICE_ID, run_id, run_id))
        if NUM_OF_LAUNCHES > 1:
            time.sleep(LAUNCH_BACKOFF)
    exit(failures)
