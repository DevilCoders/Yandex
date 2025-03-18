import os
import json

import yatest

from antiadblock.cryprox.cryprox.common.config_utils import get_validate_configs, write_configs_to_file_cache
from antiadblock.cryprox.tests.lib.initial_config import INITIAL_CONFIG

PERSISTENT_VOLUME_PATH = yatest.common.output_path("perm")
if not os.path.exists(PERSISTENT_VOLUME_PATH):
    os.makedirs(PERSISTENT_VOLUME_PATH)
CONFIG_CACHE = os.path.join(PERSISTENT_VOLUME_PATH, "cached_configs.json")


def test_validate_configs():
    configs = json.loads(INITIAL_CONFIG)
    configs = get_validate_configs(configs)
    assert len(configs) == 7
    for config_name in ('yandex_mail', 'yandex_morda', 'autoru', 'test_local', 'test_local_2::active::None::None', 'autoredirect.turbo', 'zen.yandex.ru'):
        assert config_name in configs


def test_validate_broken_configs():
    configs = json.loads(INITIAL_CONFIG)
    del configs['yandex_mail']['config']["PARTNER_TOKENS"]
    del configs['yandex_morda']['config']["CRYPT_URL_RE"]
    del configs['autoru']['config']["PROXY_URL_RE"]
    configs = get_validate_configs(configs)
    assert len(configs) == 4
    for service_id in ('yandex_mail', 'yandex_morda', 'autoru'):
        assert service_id not in configs
    assert 'test_local' in configs
    assert 'test_local_2::active::None::None' in configs
    assert 'autoredirect.turbo' in configs
    assert 'zen.yandex.ru' in configs


def test_validate_broken_configs_get_cache():
    configs = json.loads(INITIAL_CONFIG)
    write_configs_to_file_cache(configs, cache=CONFIG_CACHE)
    del configs['yandex_mail']['config']["PARTNER_TOKENS"]
    del configs['yandex_morda']['config']["CRYPT_URL_RE"]
    del configs['autoru']['config']["PROXY_URL_RE"]
    configs = get_validate_configs(configs, cache=CONFIG_CACHE)
    assert len(configs) == 7
    for config_name in ('yandex_mail', 'yandex_morda', 'autoru', 'test_local', 'test_local_2::active::None::None', 'autoredirect.turbo', 'zen.yandex.ru'):
        assert config_name in configs
