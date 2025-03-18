# -*- coding: utf8 -*-
import json
import requests

import pytest

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME, YAUID_COOKIE_NAME, NGINX_SERVICE_ID_HEADER
from antiadblock.cryprox.tests.lib.util import update_uids_file


TEST_UIDS = [None] + range(0, 100, 10)
USER_AGENTS = [
    None,  # without
    'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0',  # DESKTOP
    'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1',  # MOBILE
]
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


@pytest.mark.parametrize('bypass_uids', [
    TEST_UIDS[1::2],
    TEST_UIDS[2::2],
])
def test_bypass_for_experimental_uids(bypass_uids, cryprox_service_url, stub_server, cryprox_worker_address, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config.update(CRYPTING_CONFIG)
    new_test_config['BYPASS_BY_UIDS'] = True

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)
    update_uids_file(bypass_uids)
    requests.get("{}/control/update_bypass_uids".format(cryprox_service_url))

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    not_crypted_url = 'http://{}/page'.format(cryprox_worker_address)
    crypted_url = crypt_url(binurlprefix, 'http://test.local/page', key, False)  # never bypassing content of the crypted urls

    for user_agent in USER_AGENTS:
        for uid in TEST_UIDS:
            response = requests.get(
                not_crypted_url,
                headers={
                    'host': 'test.local',
                    PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                    'user-agent': user_agent,
                },
                cookies={YAUID_COOKIE_NAME: str(uid)} if uid is not None else None
            )
            assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
            proxied_not_crypted_url = response.json()
            response = requests.get(
                crypted_url,
                headers={
                    'host': 'test.local',
                    PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                    'user-agent': user_agent,
                },
                cookies={YAUID_COOKIE_NAME: str(uid)} if uid is not None else None,
            )
            assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
            proxied_crypted_url = response.json()
            # assert content is not crypted
            if uid in bypass_uids:
                assert TO_CRYPT_AS_JSON == proxied_not_crypted_url
            # assert content is crypted
            for content in TO_CRYPT_AS_JSON:
                for raw_content, proxied_not_crypted_url_cont, proxied_crypted_url_cont in zip(TO_CRYPT_AS_JSON[content], proxied_not_crypted_url[content], proxied_crypted_url[content]):
                    assert raw_content != proxied_crypted_url_cont
                    if uid not in bypass_uids:
                        assert raw_content != proxied_not_crypted_url_cont
