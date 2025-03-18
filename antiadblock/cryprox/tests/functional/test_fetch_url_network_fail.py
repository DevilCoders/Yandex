# -*- coding: utf8 -*-
import time

import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME


def handler(**request):
    path = request.get('path', '/')
    if path == '/retryable':
        return {'text': 'Really fast fail', 'code': 599}
    else:
        time.sleep(0.1)
        return {'text': 'Really slow fail', 'code': 599}


def test_fetch_network_fail(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['NETWORK_FAILS_RETRY_THRESHOLD'] = 90  # 90ms

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    url = 'http://test.local/retryable'
    crypted_link = crypt_url(binurlprefix, url, key, False)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert proxied.status_code == 598

    url = 'http://test.local/not_retryable'
    crypted_link = crypt_url(binurlprefix, url, key, False)
    proxied = requests.get(crypted_link, headers={"host": 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert proxied.status_code == 599
