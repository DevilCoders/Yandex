# coding=utf-8
# Утилита для загрузки на балансировщики параметров для расшифровки урлов
import os
import json
import typing as t

import requests

from retry.api import retry
from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.configs_api import get_configs_from_api, RequestRetryableException

logger = create_logger('push_balancer_crypto_settings')

CONFIGS_API_HOST = os.getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
CONFIGS_API_URL = "https://{}/v2/services/balancer_settings".format(CONFIGS_API_HOST)

NANNY_API_HOST = os.getenv('NANNY_API_HOST', 'ext.its.yandex-team.ru')
NANNY_API_URL = 'http://{}/v1'.format(NANNY_API_HOST)

TVM_ID = int(os.getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv('TVM_SECRET')

CONFIGSAPI_TVM_ID = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))  # Adminka production
NANNY_TOKEN = os.getenv("NANNY_TOKEN")


def get_balancer_settings_from_api(tvm_client):
    return get_configs_from_api(CONFIGS_API_URL, tvm_client)


class PreconditionFailedException(RuntimeError):
    pass


class NannyClient:
    def __init__(self, token: str) -> None:
        self.token = token

    @retry(tries=3, delay=1, backoff=3, exceptions=(PreconditionFailedException,))
    def set_ruchka(self, ruchka: str, value: dict) -> None:
        prev_version = None
        r = self._get_ruchka(ruchka)
        if r.status_code == 200:
            old_value_serialized = r.json()['user_value']
            try:
                old_value = json.loads(old_value_serialized)
            except json.JSONDecodeError:
                old_value = None

            if old_value == value:
                logger.info(f'Value at {ruchka} already equals {value}')
                return
            prev_version = r.headers['ETag']
        r = self._set_ruchka(ruchka, value, prev_version)

    @retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
    def _get_ruchka(self, ruchka: str) -> requests.Response:
        headers = self._get_headers()
        url = self._get_ruchka_url(ruchka)

        r = requests.get(url, headers=headers)
        if r.status_code not in {200, 404}:
            logger.warning(f'Getting {url} failed with status code {r.status_code}')
            raise RequestRetryableException(f'GET {r.status_code} {r.text}')

        logger.info(f'Got {ruchka} value with code {r.status_code}')
        return r

    @retry(tries=3, delay=1, backoff=3, exceptions=(RequestRetryableException, requests.exceptions.ConnectionError))
    def _set_ruchka(self, ruchka: str, value: dict, prev_version: t.Optional[str] = None) -> requests.Response:
        headers = self._get_headers()
        if prev_version is not None:
            headers['If-Match'] = prev_version
        url = self._get_ruchka_url(ruchka)

        r = requests.post(url, json={"value": json.dumps(value)}, headers=headers)
        if r.status_code == 412:
            logger.warning(f'Posting {ruchka} precondition failed')
            raise PreconditionFailedException(f'POST {r.status_code} {r.text}')
        if r.status_code != 200:
            logger.warning(f'Posting {ruchka} failed with status code {r.status_code}')
            raise RequestRetryableException(f'POST {r.status_code} {r.text}')

        logger.info(f'Set {ruchka} to {value}')
        return r

    def _get_ruchka_url(self, ruchka: str) -> str:
        ruchka = ruchka.strip('/')
        url = f'{NANNY_API_URL}/values/{ruchka}/'
        return url

    def _get_headers(self, **additional):
        return dict(Authorization=f'OAuth {self.token}', **additional)


if __name__ == "__main__":
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGSAPI_TVM_ID)
    )

    tvm_client = TvmClient(tvm_settings)
    nanny_client = NannyClient(NANNY_TOKEN)

    logger.info(f'Getting settings from: {CONFIGS_API_URL}')
    balancer_settings = get_balancer_settings_from_api(tvm_client)
    logger.info(f'Got settings from: {CONFIGS_API_URL}')

    raised_exceptions = []
    for service_id, balancers in balancer_settings.items():
        for balancer, settings in balancers.items():
            logger.info(f'Setting {balancer} settings')
            try:
                nanny_client.set_ruchka(balancer, settings)
            except Exception as e:
                logger.error(f'Setting value ({settings}) for {balancer} failed')
                raised_exceptions.append(e)

    if raised_exceptions:
        raise RuntimeError('Task failed with exceptions: \n' + '\n'.join(str(e) for e in raised_exceptions))
    logger.info('Successfully set balancer settings')
