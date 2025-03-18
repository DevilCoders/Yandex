# coding=utf8
import os
import logging
import traceback

from json import loads, load, dump
from library.python import resource

from tornado import gen
from tornado.httpclient import AsyncHTTPClient, HTTPError
from tornado.gen import coroutine, Return
from tvmauth import (
    TvmClient,
    TvmApiClientSettings,
)

from antiadblock.cryprox.cryprox.config.service import ENV_TYPE, CRYPROX_TVM2_CLIENT_ID, CRYPROX_TVM2_SECRET, ADMIN_TVM2_CLIENT_ID, \
    FETCH_CONFIG_WAIT_INITIAL, FETCH_CONFIG_ATTEMPTS, FETCH_CONFIG_WAIT_MULTIPLIER, PERSISTENT_VOLUME_PATH

from antiadblock.cryprox.cryprox.common.tools.internal_experiments import InternalExperiment
from antiadblock.cryprox.cryprox.common.tools.misc import ConfigName

CONFIG_CACHE = os.path.join(PERSISTENT_VOLUME_PATH, "cached_configs.json")
# internal service ticket from antiadblock.cryprox.cryprox.to configs_admin_api. Only for test purpose. Not valid on prod env
TEST_TVM_TICKET = resource.find("/cryprox/common/test_tvm_ticket.key")


class objview(object):
    def __init__(self, dict):
        self.__dict__ = dict


class ValidationError(Exception):
    pass


def get_tvm_client():
    if ENV_TYPE in ['development', 'staging', 'production', 'load_testing']:
        tvm_settings = TvmApiClientSettings(
            self_tvm_id=CRYPROX_TVM2_CLIENT_ID,
            self_secret=CRYPROX_TVM2_SECRET,
            enable_service_ticket_checking=False,
            dsts=dict(configs_admin=ADMIN_TVM2_CLIENT_ID))

        tvm = TvmClient(tvm_settings)

    else:
        # no ipv6 support on mac for local or test environment. Have no access to tvm-api.yandex.net:443 =(
        class TVM2Mock(object):
            def get_service_ticket_for(self, _):
                return TEST_TVM_TICKET
        tvm = TVM2Mock()
    return tvm


@coroutine
def read_configs_from_api(url, tvm_client=None):
    """
    Read configs from api and update cache if everything went well
    :param url: url to read config from, duh
    :param tvm_client: tvm client
    :return: dict with cryprox configuration
    """
    try:
        configs = yield read_configs_from_url(url, tvm_client)
    except Exception:
        err_msg = "Error getting configs from {}".format(url)
        logging.exception(err_msg)
        raise Exception(err_msg)
    else:
        write_configs_to_file_cache(configs)

    raise Return(configs)


class FetchConfigsException(Exception):
    pass


# TODO: write retry_async decorator
@coroutine
def read_configs_from_url(url, tvm_client, attempts=FETCH_CONFIG_ATTEMPTS, initial_wait_time=FETCH_CONFIG_WAIT_INITIAL, multiplier=FETCH_CONFIG_WAIT_MULTIPLIER):
    from antiadblock.cryprox.cryprox.config import service as service_config
    """
    :param multiplier: wait time multiplier between retries
    :param initial_wait_time:
    :param attempts: how many attempts to fetch configs will be done
    :param url: url to read config from, duh
    :param tvm_client: tvm client
    :return: dict with cryprox configuration
    """
    http_client = AsyncHTTPClient(force_instance=True)
    wait = initial_wait_time
    headers = {}
    if url == service_config.CONFIGSAPI_URL:
        if tvm_client is None:
            raise FetchConfigsException('TvmClient is not inited')
        headers = {'X-Ya-Service-Ticket': tvm_client.get_service_ticket_for('configs_admin')}

    try:
        for i in xrange(1, attempts + 1):
            try:
                response = yield http_client.fetch(url, headers=headers)
                configs = get_validate_configs(loads(response.body))
                raise Return(configs)
            except HTTPError:
                # shouldn't wait on last attempt
                if i < attempts:
                    yield gen.sleep(wait)
                    wait *= multiplier

        raise FetchConfigsException('All tries to fetch configs have failed! Last exception was ({})'.format(traceback.format_exc()))
    finally:
        http_client.close()


def read_configs_from_file_cache(cache=CONFIG_CACHE):
    """
    :return: dict with cryprox configuration
    """
    with open(cache, "r") as f:
        configs = load(f)
    return configs


def write_configs_to_file_cache(configs, cache=CONFIG_CACHE):
    """
    :param configs: dict with cryprox configuration
    :param cache: path to configs cache file
    """
    with open(cache, "w") as f:
        dump(configs, f)


@coroutine
def get_configs(configs_api_url=None, tvm_client=None):
    """
    :param configs_api_url: URL for configs API
    :param tvm_client: tvm client
    :return: {key: cryprox.config.config.Config}
    In hierarchical scheme key is 'service_id::status::device_type::exp_id', in old scheme key is service_id
    """
    if configs_api_url is None:
        raise ValueError("Configs api url was not set")

    from antiadblock.cryprox.cryprox.config.config import Config

    resulting_configs = {}
    configs = yield read_configs_from_api(configs_api_url, tvm_client)
    for key, config in configs.iteritems():
        service_id = ConfigName(key).service_id
        resulting_configs[key] = Config(service_id, config=objview(config['config']), version=config['version'])

    raise Return(resulting_configs)


def get_validate_configs(configs, cache=CONFIG_CACHE):
    """
    Rudimentary config validation
    :param configs: mapping of {key: cryprox.config.config.Config}
    :param cache: path to configs cache file
    :return: valid_configs: mapping of {key: cryprox.config.config.Config}
    In hierarchical scheme key is 'service_id::status::device_type::exp_id', in old scheme key is service_id
    """
    from antiadblock.cryprox.cryprox.config.config import Config
    valid_configs = {}
    try:
        cached_configs = read_configs_from_file_cache(cache)
    except Exception:
        cached_configs = {}
    for key, config in configs.iteritems():
        service_id = ConfigName(key).service_id
        try:
            Config(service_id, config=objview(config['config']), version=config['version'])
        except (TypeError, KeyError, IndexError, AttributeError) as e:
            logging.error("Exception during configs validation", action='config_updated', service_id=service_id, cfg_version=config['version'], message=str(e))
            # если конфиг не валидный, то пробуем забрать из кэша
            if key in cached_configs:
                valid_configs[key] = cached_configs[key]
        #  в случае отсутствия ошибок, сохраним конфиг сервиса
        else:
            valid_configs[key] = config

    if not valid_configs:
        raise ValidationError("Empty configs are not allowed")
    return valid_configs


def select_configs(configs, device_type, exp_id=None, is_internal=False):
    # если конфиг для сервиса только один, то его возвращаем безусловно
    if len(configs) == 1:
        return configs.values()[0]
    service_id = configs.values()[0].name
    status = "test" if ENV_TYPE == "staging" else "active"

    if is_internal:
        exp_id = InternalExperiment().get_exp_id(service_id) or exp_id

    tmpl_key = "{service_id}::{status}::{device}::{exp_id}"
    if exp_id == "testing":
        status = "test"
    if exp_id is None or exp_id == "testing":
        keys = [
            (device_type, None),
            (None, None),
        ]
        for key in keys:
            result = configs.get(tmpl_key.format(service_id=service_id, status=status, device=key[0], exp_id=key[1]))
            if result:
                break
    else:
        keys = [
            (None, device_type, exp_id),
            (None, None, exp_id),
            (status, device_type, None),
            (status, None, None),
        ]
        for key in keys:
            result = configs.get(tmpl_key.format(service_id=service_id, status=key[0], device=key[1], exp_id=key[2]))
            if result:
                break

    logging.debug(None, action='select_config', service_id=service_id, status=status, device_type=device_type, exp_id=exp_id)
    return result
