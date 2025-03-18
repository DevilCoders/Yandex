import os

import requests
from retry import retry

from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.common_configs import INNER_SERVICE_IDS, ADDITIONAL_COOKIES_FOR_COOKIE_MANAGER
from antiadblock.tasks.tools.configs_api import get_configs_from_api, get_service_ticket_for, RequestRetryableException, \
    RequestFatalException

logger = create_logger("gdpr_cookies_data")

TVM_ID = int(os.getenv("TVM_ID", "2002631"))
TVM_SECRET = os.getenv("TVM_SECRET")

COOKIE_MANAGER_PRODUCTION_TVM_CLIENT_ID = 2000270
COOKIE_MANAGER_TEST_TVM_CLIENT_ID = 2000269

AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID = 2000629
AAB_ADMIN_TEST_TVM_CLIENT_ID = 2000627


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError),
       logger=logger)
def post_cookies_to_cookie_manager(api_url, tvm_client, cookies):
    ticket_for_cookie_manager_api = get_service_ticket_for(tvm_client, "cookie_manager_api", logger)

    logger.info("Post data to: {}".format(api_url))
    response = requests.post(api_url, headers={"X-Ya-Service-Ticket": ticket_for_cookie_manager_api,
                                               "Content-Type": "application/json"}, json=dict(cookies=cookies))

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException(
            "Unable to make cookie manager request: response code is {}".format(response.status_code))

    if response.status_code != 200:
        raise RequestFatalException("{code} {text}".format(code=response.status_code, text=response.text))


if __name__ == "__main__":
    if os.getenv("ENV_TYPE", "PRODUCTION") == "PRODUCTION":
        configs_api_tvm_id = AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID
        configs_api_host = "api.aabadmin.yandex.ru"
        cookie_manager_tvm_id = COOKIE_MANAGER_PRODUCTION_TVM_CLIENT_ID
        cookie_manager_url = "http://internalapi.metrika.yandex.ru:8096/gdpr/cookie_info"
    else:
        configs_api_tvm_id = AAB_ADMIN_TEST_TVM_CLIENT_ID
        configs_api_host = "preprod.aabadmin.yandex.ru"
        cookie_manager_tvm_id = COOKIE_MANAGER_TEST_TVM_CLIENT_ID
        cookie_manager_url = "http://internalapi-test.metrika.yandex.ru:8096/gdpr/cookie_info"

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configs_api_tvm_id, cookie_manager_api=cookie_manager_tvm_id)
    )

    configs_api_url = "https://{host}/v2/configs_handler?status=active".format(host=configs_api_host)

    tvm_client = TvmClient(tvm_settings)
    configs = get_configs_from_api(configs_api_url, tvm_client)

    cookies_names = set()
    for service_id in INNER_SERVICE_IDS:
        cookies_names.update(configs[service_id]['config']['WHITELIST_COOKIES'])

    cookies_names.update(ADDITIONAL_COOKIES_FOR_COOKIE_MANAGER)

    ttl = 1209600  # Two weeks (60 х 60 х 24 х 7 х 2)
    cookies = [dict(name=cookie_name, type="tech", ttl=ttl, descr="cookie") for cookie_name in cookies_names]

    post_cookies_to_cookie_manager(cookie_manager_url, tvm_client, cookies)
