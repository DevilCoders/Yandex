import requests

from retry.api import retry
from tvmauth import TvmClient, TvmApiClientSettings
from tvmauth .exceptions import TvmException


class RequestException(Exception):
    pass


class RequestFatalException(RequestException):
    pass


class RequestRetryableException(RequestException):
    pass


def get_service_ticket_for(tvm_client, service, logger=None):
    try:
        return tvm_client.get_service_ticket_for(service)
    except TvmException:
        if logger is not None:
            logger.exception("Fetching ticket for {} failed".format(service))
        raise RequestRetryableException("Fetching ticket for {} failed".format(service))


@retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
def get_configs_from_api(api_url, tvm_client, params=None):
    """
    :return: List of active configs for active service
    """
    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api')
    response = requests.get(api_url, headers={'X-Ya-Service-Ticket': tvm_ticket}, params=params)

    if response.status_code in (502, 503, 504):
        raise RequestRetryableException("Unable to make configs api request: response code is {}".format(response.status_code))

    if response.status_code != 200:
        raise RequestFatalException('{code} {text}'.format(code=response.status_code, text=response.text))
    return response.json()


def post_tvm_request(api_url, tvm_client, data=None):
    data = data or dict()
    tvm_ticket = get_service_ticket_for(tvm_client, 'configs_api')
    response = requests.post(api_url, json=data, headers={'X-Ya-Service-Ticket': tvm_ticket})
    response.raise_for_status()
    return response


def get_configs(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host='api.aabadmin.yandex.ru', monitorings_enabled=False, hierarchical=False):
    tvm_client = TvmClient(TvmApiClientSettings(
        self_tvm_id=tvm_id,
        self_secret=tvm_secret,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configsapi_tvm_id)
    ))
    if hierarchical:
        handler = "configs_hierarchical_handler"
    else:
        handler = "configs_handler"
    configs_api_url = "https://{host}/v2/{handler}?status=active{monitorings_enabled}".format(
        host=configs_api_host,
        handler=handler,
        monitorings_enabled='&monitorings_enabled=true' if monitorings_enabled else ""
    )
    configs = get_configs_from_api(configs_api_url, tvm_client)
    return configs


def get_monitoring_settings(tvm_id, tvm_secret, configsapi_tvm_id, configs_api_host='api.aabadmin.yandex.ru'):
    tvm_client = TvmClient(TvmApiClientSettings(
        self_tvm_id=tvm_id,
        self_secret=tvm_secret,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=configsapi_tvm_id)
    ))
    configs_api_url = "https://{host}/v2/monitoring_settings".format(
        host=configs_api_host,
    )
    services = get_configs_from_api(configs_api_url, tvm_client)
    return services
