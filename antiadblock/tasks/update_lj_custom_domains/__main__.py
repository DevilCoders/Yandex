# coding=utf-8
# Утилита для автоматической изменения значения PROXY_URL_RE в конфиге ЖЖ
import os
import socket
from copy import deepcopy

import re2
import requests
from retry.api import retry

from tvmauth import TvmClient, TvmApiClientSettings
from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_service_ticket_for


logger = create_logger("update_lj_custom_domains")


TVM_ID = int(os.getenv("TVM_ID", "2002631"))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv("TVM_SECRET")
CONFIG_STATUS = os.getenv("CONFIG_STATUS", "test")  # "test" or "active"
CONFIGS_API_TVM_ID = int(os.getenv("CONFIGSAPI_TVM_ID", "2000627"))
CONFIGS_API_HOST = os.getenv("CONFIGS_API_HOST", "test.aabadmin.yandex.ru")

VALID_IP_ADDRESSES = {
    '81.19.74.0',
    '81.19.74.1',
    '81.19.74.2',
    '81.19.74.3',
    '81.19.74.4',
    '81.19.74.5',
    '81.19.74.6',
    '81.19.74.7',
    '81.19.74.24',
    '81.19.74.25',
    '81.19.74.26',
    '64:ff9b::5113:4a00',
    '64:ff9b::5113:4a01',
    '64:ff9b::5113:4a02',
    '64:ff9b::5113:4a03',
    '64:ff9b::5113:4a04',
    '64:ff9b::5113:4a05',
    '64:ff9b::5113:4a06',
    '64:ff9b::5113:4a18',
    '64:ff9b::5113:4a19',
    '64:ff9b::5113:4a1a',
    '64:ff9b::5113:4a07',
}


@retry(tries=3, delay=1, backoff=3, exceptions=(requests.exceptions.ConnectionError, requests.exceptions.HTTPError))
def get_lj_domains(url="https://www.livejournal.com/aab/custom_domains.txt"):
    response = requests.get(url)
    response.raise_for_status()
    return [domain.strip() for domain in response.text.split()]


@retry(tries=3, delay=1, backoff=3, exceptions=(requests.exceptions.ConnectionError, requests.exceptions.HTTPError))
def get_lj_config(configs_api_host, tvm_client, status="test"):
    url = "https://{host}/label/livejournal/config/{status}".format(host=configs_api_host, status=status)
    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api', logger=logger)
    response = requests.get(url, headers={'X-Ya-Service-Ticket': tvm_ticket})
    response.raise_for_status()
    return response.json()


def make_new_lj_config(configs_api_host, tvm_client, config_data):
    url = "https://{host}/label/livejournal/config".format(host=configs_api_host)
    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api', logger=logger)
    response = requests.post(url, json=config_data, headers={'X-Ya-Service-Ticket': tvm_ticket})
    response.raise_for_status()
    return response.json()


def mark_lj_config(configs_api_host, tvm_client, config_id, old_config_id, status="test"):
    url = "https://{host}/label/livejournal/config/{config_id}/{status}".format(host=configs_api_host, config_id=config_id, status=status)
    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api', logger=logger)
    response = requests.put(url, json={"old_id": old_config_id}, headers={'X-Ya-Service-Ticket': tvm_ticket})
    response.raise_for_status()
    return response.json()


if __name__ == "__main__":
    logger.info("Task started")
    assert CONFIG_STATUS in ("active", "test")

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGS_API_TVM_ID)
    )

    tvm_client = TvmClient(tvm_settings)

    logger.info("Trying get LJ domains")
    lj_domains = get_lj_domains()
    logger.info("Results {}".format(lj_domains))

    new_proxy_url_set = set()
    is_failed = False
    for domain in lj_domains:
        try:
            ip = socket.getaddrinfo(domain, None)[0][4][0]
            if ip not in VALID_IP_ADDRESSES:
                logger.warning("Bad domain: {}, ip: {}".format(domain, ip))
                is_failed = True
                continue
        except Exception as e:
            logger.error("Error occurred when resolved domain {}: {}".format(domain, str(e)))
            is_failed = True
            continue

        if domain != "varlamov.ru":
            new_proxy_url_set.add("{}/.*?".format(re2.escape(domain)))
        else:
            # на varlamov.ru статика забирается с другого домена, его тоже нужно проксировать
            new_proxy_url_set.add(r'(?:www\.)?varlamov\.(?:ru|me)/.*?')

    if is_failed:
        logger.error("Some LJ domains is bad")
        raise Exception("Some LJ domains is bad")
    if len(new_proxy_url_set) == 0:
        logger.error("Empty list LJ domains")
        raise Exception("Empty list LJ domains")

    logger.info("Make proxy_url_re: {}".format(new_proxy_url_set))

    logger.info("Get LJ {} config".format(CONFIG_STATUS))
    lj_config = get_lj_config(CONFIGS_API_HOST, tvm_client, CONFIG_STATUS)
    curr_proxy_url_re = lj_config["data"]["PROXY_URL_RE"]
    update_proxy_url_re = []
    # make diff PROXY_URL_RE
    diff = False
    for url_re in curr_proxy_url_re:
        if "livejournal" in url_re:
            update_proxy_url_re.append(url_re)
            continue
        if url_re in new_proxy_url_set:
            update_proxy_url_re.append(url_re)
            new_proxy_url_set.remove(url_re)
        else:
            diff = True
    if len(new_proxy_url_set) > 0:
        diff = True
        for url_re in new_proxy_url_set:
            update_proxy_url_re.append(url_re)

    if diff:
        new_data = deepcopy(lj_config["data"])
        new_data["PROXY_URL_RE"] = update_proxy_url_re
        logger.info("Post new LJ config")
        config_data = {"comment": "Auto create from update_lj_custom_domains",
                       "parent_id": lj_config["id"],
                       "data_settings": lj_config["data_settings"],
                       "data": new_data}
        new_lj_config = make_new_lj_config(CONFIGS_API_HOST, tvm_client, config_data)
        logger.info("Mark {} new config".format(CONFIG_STATUS))
        mark_lj_config(CONFIGS_API_HOST, tvm_client, new_lj_config["id"], lj_config["id"], CONFIG_STATUS)
    else:
        logger.info("No diff in domains")

    logger.info("Task finished success")
