# -*- coding: utf8 -*-
import json
from urlparse import urljoin
from datetime import datetime, timedelta

import pytest
import requests
from yatest.common import source_path
from hamcrest import assert_that, has_entries

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import (PARTNER_TOKEN_HEADER_NAME, YAUID_COOKIE_NAME, CRYPTED_YAUID_COOKIE_NAME, EXPERIMENT_START_TIME_FMT,
                                                       Experiments, UserDevice, DETECT_LIB_PATHS, DETECT_LIB_HOST)
from antiadblock.encrypter import CookieEncrypter
from antiadblock.cryprox.tests.lib.util import update_uids_file


TO_CRYPT_AS_JSON = {
    'urls': [
        "/relative/scripts.js",
        "http://test.local/css/cssss.css",
        "http://avatars.mds.yandex.net/get-direct/35835321.jpeg",
        "//yastatic.net/static/jquery-1.11.0.min.js",
    ],
    "crypt_body": ["_one_crypt_something_", "_two_crypt_something_"],
    "replace_body": ["_one_replace_something_", "_two_replace_something_"],
}
CRYPTING_CONFIG = {
    'CRYPT_URL_RE': ['test\.local/.*?'],
    'CRYPT_RELATIVE_URL_RE': ['/relative/.*?'],
    'CRYPT_BODY_RE': ['_\w+_crypt_something_'],
    'REPLACE_BODY_RE': {'_\w+_replace_something_': '_'},
}


def handler(**_):
    return {'text': json.dumps(TO_CRYPT_AS_JSON, sort_keys=True), 'code': 200, 'headers': {'Content-Type': 'text/html'}}


def timestring(value):
    if isinstance(value, timedelta):
        return (datetime.utcnow() + value).strftime(EXPERIMENT_START_TIME_FMT)
    return value


def test_bypass_for_experimental_uids(stub_server, cryprox_worker_address, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    keys_path = source_path("antiadblock/encrypter/tests/test_keys.txt")
    encrypter = CookieEncrypter(keys_path)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)

    experiment = {
        'EXPERIMENT_TYPE': Experiments.BYPASS,
        'EXPERIMENT_PERCENT': 10,
        'EXPERIMENT_START': '1970-01-01T00:00:00',
        'EXPERIMENT_DURATION': 365 * 24 * 100,  # 100 years
    }
    new_test_config['EXPERIMENTS'] = [experiment]

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    not_crypted_url = 'http://{}/page'.format(cryprox_worker_address)
    crypted_url = crypt_url(binurlprefix, 'http://test.local/page', key, False, origin='test.local')  # never bypassing content of the crypted urls

    experiment_uids = ['1000010051558371334', '1009000171558371334', '9009000211558371334']
    ordinary_uids = [None, '123', '7009000171558371332', '8009000211558371335', '7709000211558371300']

    for uid in experiment_uids + ordinary_uids:
        proxied_url = requests.get(
            not_crypted_url,
            headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
            cookies={YAUID_COOKIE_NAME: uid} if uid else None
        ).json()
        proxied_url_crookie = requests.get(
            not_crypted_url,
            headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
            cookies={CRYPTED_YAUID_COOKIE_NAME: encrypter.encrypt_cookie(uid)} if uid else None
        ).json()
        proxied_crypted_url = requests.get(
            crypted_url,
            headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
            cookies={YAUID_COOKIE_NAME: uid} if uid else None,
        ).json()
        # assert content is not crypted
        if uid in experiment_uids:
            assert TO_CRYPT_AS_JSON == proxied_url
            assert TO_CRYPT_AS_JSON == proxied_url_crookie
        # assert content is crypted
        for content in TO_CRYPT_AS_JSON:
            for (raw_content,
                 proxied_url_content,
                 proxied_url_crookie_content,
                 proxied_crypted_url_content) in zip(TO_CRYPT_AS_JSON[content],
                                                     proxied_url[content],
                                                     proxied_url_crookie[content],
                                                     proxied_crypted_url[content]):
                assert raw_content != proxied_crypted_url_content
                if uid in ordinary_uids:
                    assert raw_content != proxied_url_content
                    assert raw_content != proxied_url_crookie_content


@pytest.mark.parametrize('experiment_device', [[UserDevice.DESKTOP, UserDevice.MOBILE], [UserDevice.MOBILE]])
def test_bypass_for_device(experiment_device, stub_server, cryprox_worker_address, set_handler_with_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)

    experiment = {
        'EXPERIMENT_TYPE': Experiments.BYPASS,
        'EXPERIMENT_PERCENT': 10,
        'EXPERIMENT_START': '1970-01-01T00:00:00',
        'EXPERIMENT_DURATION': 365 * 24 * 100,  # 100 years
        'EXPERIMENT_DEVICE': experiment_device,
    }
    new_test_config['EXPERIMENTS'] = [experiment]

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    url = 'http://{}/page'.format(cryprox_worker_address)

    experiment_uids = ['1000010051558371334', '1009000171558371334', '9009000211558371334']
    ordinary_uids = [None, '123', '7009000171558371332', '8009000211558371335', '7709000211558371300']

    for uid in experiment_uids + ordinary_uids:
        proxied_desktop = requests.get(
            url,
            headers={
                'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                'user-agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0',
            },
            cookies={YAUID_COOKIE_NAME: uid} if uid else None
        ).json()
        proxied_mobile = requests.get(
            url,
            headers={
                'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                'user-agent': 'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1',
            },
            cookies={YAUID_COOKIE_NAME: uid} if uid else None
        ).json()
        if uid in experiment_uids:
            if UserDevice.DESKTOP in experiment_device:
                assert proxied_desktop == TO_CRYPT_AS_JSON
            else:
                assert proxied_desktop != TO_CRYPT_AS_JSON
            if UserDevice.MOBILE in experiment_device:
                assert proxied_mobile == TO_CRYPT_AS_JSON
            else:
                assert proxied_mobile != TO_CRYPT_AS_JSON
        else:
            assert proxied_desktop != TO_CRYPT_AS_JSON
            assert proxied_mobile != TO_CRYPT_AS_JSON


@pytest.mark.parametrize('experiment_start', [
    -timedelta(hours=100),  # experiment ended 99 hours ago
    timedelta(hours=100),  # experiment will start 100 hours from now
    None,
])
def test_no_experiment_bypass_out_of_time_range(experiment_start, stub_server, cryprox_worker_address, set_handler_with_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)

    experiment = {
        'EXPERIMENT_TYPE': Experiments.BYPASS,
        'EXPERIMENT_PERCENT': 100,  # all uids are experimental
        'EXPERIMENT_START': timestring(experiment_start),
        'EXPERIMENT_DURATION': 1,
    }
    new_test_config['EXPERIMENTS'] = [experiment]
    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    not_crypted_url = 'http://{}/page'.format(cryprox_worker_address)
    for uid in [None] + map(str, range(0, 100, 30)):
        proxied_not_crypted_url = requests.get(
            not_crypted_url,
            headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
            cookies={YAUID_COOKIE_NAME: uid} if uid else None
        ).json()
        # assert content is crypted as usual:
        for content in TO_CRYPT_AS_JSON:
            for raw_content, proxied_not_crypted_url_cont in zip(TO_CRYPT_AS_JSON[content], proxied_not_crypted_url[content]):
                assert raw_content != proxied_not_crypted_url_cont


@pytest.mark.parametrize('start, duration, forcecry_on', [
    ('1970-01-01T00:00:00', 365 * 24 * 100, True),
    (-timedelta(hours=100), 1, False),  # experiment ended 99 hours ago
    (timedelta(hours=100), 1, False),  # experiment will start 100 hours from now
    (None, 365 * 24 * 100, False),
])
@pytest.mark.parametrize('experiment_device', [[UserDevice.DESKTOP, UserDevice.MOBILE], [UserDevice.DESKTOP], [UserDevice.MOBILE]])
def test_forcecry_detect_lib_params(start, duration, forcecry_on, experiment_device, stub_server, cryprox_worker_address, set_handler_with_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)
    percent = 10
    experiment = {
        'EXPERIMENT_TYPE': Experiments.FORCECRY,
        'EXPERIMENT_PERCENT': percent,
        'EXPERIMENT_START': timestring(start),
        'EXPERIMENT_DURATION': duration,
        'EXPERIMENT_DEVICE': experiment_device,
    }
    new_test_config['EXPERIMENTS'] = [experiment]
    set_handler_with_config(test_config.name, new_test_config)

    forcecry_on = forcecry_on and UserDevice.DESKTOP in experiment_device

    response = requests.get(urljoin('http://' + cryprox_worker_address, DETECT_LIB_PATHS[0]) + '?pid=test_local', headers={'host': DETECT_LIB_HOST})
    assert response.status_code == 200
    text = response.text
    # ADB_CONFIG = |{some valid json ...|,"fn": {js function which is not valid json}}
    forcecry_config = json.loads(text.split(' = ')[1].split(',"fn"')[0] + '}').get('forcecry', {})
    if forcecry_on:
        forcecry_expected = {
            "enabled": True,
            "expires": int((datetime.strptime(timestring(start), EXPERIMENT_START_TIME_FMT) - datetime(1970, 1, 1)).total_seconds() + duration * 3600000),
            "percent": percent,
        }
        assert_that(forcecry_config, has_entries(forcecry_expected))
    else:
        assert not forcecry_config.get('enabled', False)


def test_experiment_not_bypass_for_bypass_uids(cryprox_service_url, stub_server, cryprox_worker_address, set_handler_with_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)

    new_test_config['BYPASS_BY_UIDS'] = True
    experiment = {
        'EXPERIMENT_TYPE': Experiments.NOT_BYPASS_BYPASS_UIDS,
        'EXPERIMENT_PERCENT': 10,
        'EXPERIMENT_START': '1970-01-01T00:00:00',
        'EXPERIMENT_DURATION': 365 * 24 * 100  # 100 years,
    }
    new_test_config['EXPERIMENTS'] = [experiment]

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    experiment_uids = ['1000010051558371334', '1009000171558371334', '9009000211558371334']
    ordinary_uids = ['123', '7009000171558371332', '8009000211558371335', '7709000211558371300']

    update_uids_file(experiment_uids + ordinary_uids)
    requests.get("{}/control/update_bypass_uids".format(cryprox_service_url))

    url = 'http://{}/page'.format(cryprox_worker_address)

    for uid in experiment_uids + ordinary_uids:
        proxied_desktop = requests.get(
            url,
            headers={
                'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                'user-agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0',
            },
            cookies={YAUID_COOKIE_NAME: uid}
        ).json()
        proxied_mobile = requests.get(
            url,
            headers={
                'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                'user-agent': 'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1',
            },
            cookies={YAUID_COOKIE_NAME: uid}
        ).json()
        if uid in ordinary_uids:  # для неэкспериментальных уидов проверяем, что верстка нешифрованная (все уиды добавлены в списки BYPASS_UIDS
            assert proxied_desktop == TO_CRYPT_AS_JSON
            assert proxied_mobile == TO_CRYPT_AS_JSON
        else:
            assert proxied_desktop != TO_CRYPT_AS_JSON
            assert proxied_mobile != TO_CRYPT_AS_JSON
