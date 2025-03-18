# coding=utf-8
# Утилита для загрузки в секретницу параметров для рашифровки урлов
import os
import json
from base64 import b64encode

import re2
import requests
from tenacity import retry, stop_after_attempt, wait_exponential

from tvmauth import TvmClient, TvmApiClientSettings
from library.python.vault_client.instances import Production as VaultClient

from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('decrypt_url_configs')

configs_api_host = os.getenv("CONFIGS_API_HOST", 'api.aabadmin.yandex.ru')
configs_api_url = "https://{}/v2/configs_handler?status=active".format(configs_api_host)

TVM_ID = int(os.getenv('TVM_ID', '2002631'))  # use SANDBOX monitoring tvm_id
TVM_SECRET = os.getenv('TVM_SECRET')

CONFIGSAPI_TVM_ID = int(os.getenv('CONFIGSAPI_TVM_ID', '2000629'))  # Adminka production
VAULT_TOKEN = os.getenv('YAV_TOKEN')
SECRET_UUID = os.getenv('SECRET_UUID', 'sec-01dbz0grcabghreabrhjr7chg7')


@retry(stop=stop_after_attempt(3), wait=wait_exponential(multiplier=3, min=1))
def get_configs_from_api(tvm_ticket):
    """
    :return: List of active configs for active service
    """
    response = requests.get(configs_api_url, headers={'X-Ya-Service-Ticket': tvm_ticket})
    if response.status_code != 200:
        raise Exception('{code} {text}'.format(code=response.status_code, text=response.text))
    return response.json()


if __name__ == "__main__":
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGSAPI_TVM_ID)
    )

    tvm_client = TvmClient(tvm_settings)
    client = VaultClient(rsa_auth=False, authorization='OAuth {}'.format(VAULT_TOKEN), decode_files=True)

    logger.info('Get configs from: {}'.format(configs_api_url))
    configs = get_configs_from_api(tvm_client.get_service_ticket_for('configs_api'))
    token_map = {}

    for service_id, config in configs.items():
        config = config.get('config', None)
        if config is None:
            logger.warning('Config for {} is empty'.format(service_id))
            continue
        crypt_url_preffix = re2.escape(config.get('CRYPT_URL_PREFFIX', '/'))
        crypt_url_old_preffix = [re2.escape(preffix) for preffix in config.get('CRYPT_URL_OLD_PREFFIXES', [])]
        preffixes = "|".join([crypt_url_preffix] + crypt_url_old_preffix)
        crypt_secret_key = config.get('CRYPT_SECRET_KEY')
        crypt_enable_trailing_slash = config.get('CRYPT_ENABLE_TRAILING_SLASH', False)
        params = {
            "service_id": service_id,
            "crypt_secret_key": crypt_secret_key,
            "crypt_preffixes": preffixes,
            "crypt_enable_trailing_slash": crypt_enable_trailing_slash,
            }
        for token in config.get('PARTNER_TOKENS', []):
            token_map[token] = params
    logger.info('Getting configs is succesfully')
    if token_map:
        logger.info('Get previous version params from vault')
        head_version = client.get_version(SECRET_UUID).get('value', {})
        old_params = head_version.get('decrypt-params', None)
        if old_params is not None:
            old_params = json.loads(old_params)
        # параметры будем обновлять только если они изменились
        if old_params != token_map:
            logger.info('Params is change')
            logger.info('Push params in vault')
            client.create_secret_version(
                SECRET_UUID,
                value=[
                    {
                        'key': 'decrypt-params',
                        'value': b64encode(json.dumps(token_map, sort_keys=True)),
                        'encoding': 'base64',
                    },
                ],
            )
            logger.info('Pushing params is successfully')
        else:
            logger.info('Params do not change')
    else:
        logger.warning('Configs is empty')
