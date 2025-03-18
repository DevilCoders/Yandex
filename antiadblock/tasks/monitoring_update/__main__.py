# coding: utf-8
import os

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_monitoring_settings
from antiadblock.tasks.monitoring_update import money_monitoring


def get_services_from_configs_api():
    tvm_id = int(os.getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id as default
    configsapi_tvm_id = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))
    tvm_secret = os.getenv('TVM_SECRET')
    configs_api_host = os.getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
    services = get_monitoring_settings(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host=configs_api_host)
    return services


if __name__ == '__main__':
    logger = create_logger('monitoring_update')
    services = get_services_from_configs_api()
    logger.info(services)
    money_monitoring.run(services)
