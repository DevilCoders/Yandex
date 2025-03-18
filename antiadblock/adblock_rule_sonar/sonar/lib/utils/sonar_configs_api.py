from os import getenv
from antiadblock.tasks.tools.configs_api import get_configs


SONAR_TVM_ID = 2002631
CONFIGSAPI_TVM_ID = 2000629
CONFIGSAPI_TVM_ID_TEST = 2000627


def get_service_ids_and_current_cookies():
    response = get_configs(tvm_id=SONAR_TVM_ID, tvm_secret=getenv("SONAR_TVM_SECRET", None),
                           configsapi_tvm_id=CONFIGSAPI_TVM_ID, monitorings_enabled=False)
    cookies_of_the_day = [c['config'].get(u'CURRENT_COOKIE') for c in response.values()]
    cookies_of_the_day = set(filter(None, cookies_of_the_day))
    return response.keys(), list(cookies_of_the_day)
