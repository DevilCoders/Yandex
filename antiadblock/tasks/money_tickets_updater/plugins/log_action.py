# coding=utf-8
import os
from datetime import timedelta, datetime

from antiadblock.tasks.tools.configs_api import get_configs_from_api
from tvmauth import TvmClient, TvmApiClientSettings


LOG_ACTION_API_URL = 'https://api.aabadmin.yandex.ru/audit/service/{service_id}?offset=0&limit=150'
CONFIGS_DIFF = 'https://antiblock.yandex.ru/service/{service_id}/configs/diff/{new_config}/{old_config} '


def to_url(service_id, new_config, old_config):
    return "((" + CONFIGS_DIFF.format(service_id=service_id, new_config=new_config, old_config=old_config) + \
           "#{}".format(new_config) + "))"


def get_log_action_info(service_id, start_dt):
    date_min = start_dt - timedelta(weeks=2)
    tvm_id = int(os.getenv('TVM_ID', '2002631'))  # SB jobs
    configsapi_tvm_id = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))
    tvm_secret = os.getenv('TVM_SECRET')
    tvm_client = TvmClient(TvmApiClientSettings(
        self_tvm_id=tvm_id,
        self_secret=tvm_secret,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configsapi_tvm_id)
    ))
    result = get_configs_from_api(LOG_ACTION_API_URL.format(service_id=service_id), tvm_client)["items"]
    result = filter(lambda item: item["action"] == "config_mark_active", result)

    return_string = ""

    for item in result:
        item["date"] = datetime.strptime(item["date"], '%a, %d %b %Y %H:%M:%S GMT')
        if item["date"] < date_min:
            break
        return_string += " | ".join(
            filter(None, [item["date"].strftime("%Y-%m-%d %H:%M"),
                          to_url(service_id, item["params"]["config_id"], item["params"]["old_config_id"]),
                          item.get("label_id", None),
                          item["params"]["config_comment"]])
        ) + "\n"

    return return_string
