# coding=utf-8
import os
from antiadblock.tasks.tools.configs_api import post_tvm_request
from tvmauth import TvmClient, TvmApiClientSettings

SBS_RUN_API_URL = 'https://api.aabadmin.yandex.ru/service/{service_id}/sbs_check/run'
SBS_DIFF_URL = 'https://antiblock.yandex.ru/service/{service_id}/screenshots-checks/diff/{run_id}/{run_id}'


def get_argus_info(service_id, start_dt):
    tvm_id = int(os.getenv('TVM_ID', '2002631'))  # Sandbox jobs
    configsapi_tvm_id = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))
    tvm_secret = os.getenv('TVM_SECRET')
    tvm_client = TvmClient(TvmApiClientSettings(
        self_tvm_id=tvm_id,
        self_secret=tvm_secret,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configsapi_tvm_id)
    ))
    result = post_tvm_request(SBS_RUN_API_URL.format(service_id=service_id), tvm_client, data={})
    return SBS_DIFF_URL.format(service_id=service_id, run_id=result.json()['run_id'])
