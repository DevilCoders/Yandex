# coding=utf-8
# Утилита для похода за параметрами в Вебмастер и отправки их в админку
import os

import requests
from retry import retry

from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_service_ticket_for, RequestRetryableException, RequestFatalException


logger = create_logger("autoredirect_data")

TVM_ID = int(os.getenv("TVM_ID", "2002631"))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv("TVM_SECRET")

WEBMASTER_TEST_TVM_CLIENT_ID = 2000286
WEBMASTER_PROD_TVM_CLIENT_ID = 2000036

AAB_ADMIN_TEST_TVM_CLIENT_ID = 2000627
AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID = 2000629


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError), logger=logger)
def get_autoredirect_data_from_webmaster(api_url, tvm_client):
    ticket_for_wm = get_service_ticket_for(tvm_client, "webmaster_api", logger)

    logger.info("Get data from: {}".format(api_url))
    response = requests.get(api_url, headers={"X-Ya-Service-Ticket": ticket_for_wm})

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make webmaster request: response code is {}".format(response.status_code))

    if response.status_code != 200:
        raise RequestFatalException("{code} {text}".format(code=response.status_code, text=response.text))

    data = response.json()
    errors = data.get("errors", [])
    if errors:
        raise RequestFatalException("Errors: {text}".format(text=errors[0].get("message", "")))
    return data["data"]["aabTurboUrls"]


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError), logger=logger)
def post_autoredirect_data_to_confis_api(api_url, tvm_client, data):
    ticket_for_configs_api = get_service_ticket_for(tvm_client, "configs_api", logger)

    logger.info("Post data to: {}".format(api_url))
    response = requests.post(api_url, headers={"X-Ya-Service-Ticket": ticket_for_configs_api}, json=data)

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make configs api request: response code is {}".format(response.status_code))

    if response.status_code != 201:
        raise RequestFatalException("{code} {text}".format(code=response.status_code, text=response.text))


if __name__ == "__main__":
    if os.getenv("ENV_TYPE", "PRODUCTION") == "PRODUCTION":
        configs_api_tvm_id = AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID
        webmaster_tvm_id = WEBMASTER_PROD_TVM_CLIENT_ID
        webmaster_host = "webmaster3-internal.prod.in.yandex.net"
        configs_api_host = "api.aabadmin.yandex.ru"
    else:
        configs_api_tvm_id = AAB_ADMIN_TEST_TVM_CLIENT_ID
        webmaster_tvm_id = WEBMASTER_TEST_TVM_CLIENT_ID
        webmaster_host = "webmaster3-internal.test.in.yandex.net"
        configs_api_host = "preprod.aabadmin.yandex.ru"

    configs_api_url = "https://{}/v2/redirect_data".format(configs_api_host)
    webmaster_url = "https://{}/turbo/antiAdvertBlock/listActive.json".format(webmaster_host)

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configs_api_tvm_id, webmaster_api=webmaster_tvm_id)
    )

    tvm_client = TvmClient(tvm_settings)

    data = get_autoredirect_data_from_webmaster(webmaster_url, tvm_client)

    data_to_configs_api = {}

    for service in data:
        if not service["sourceUrls"]:
            continue
        data_to_configs_api[service["domain"].replace(".", "-").replace("_", "-")] = {"domain": service["domain"], "urls": service["sourceUrls"]}

    if data_to_configs_api:
        post_autoredirect_data_to_confis_api(configs_api_url, tvm_client, data_to_configs_api)
    else:
        raise Exception("Empty data to configs_api: {}".format(str(data)))
