# -*- coding: utf8 -*-
import json

import pytest
import requests

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME


TO_CRYPT_AS_JSON = {
    'urls': {
        "relative": [
            "/relative/scripts.js",
            "/relative/picture.jpeg",
        ],
        "partner_urls": [
            "http://test.local/css/cssss.css",
            "http://test.local/scripts/res.js",
            "http://test.local/images/image.png",
            "http://avatars.mds.yandex.net/get-autoru/880749/4b107a3b91144a54bc25037ca76728fd/thumb_m",
        ],
        "bk": [
            "http://avatars.mds.yandex.net/get-direct/135341/DRPqm0sH4Jdr6AtpnjJ8MQ/y180",
            "http://avatars.mds.yandex.net/get-direct/35835321.jpeg",
            "//yastatic.net/static/jquery-1.11.0.min.js",
            "//yastat.net/static/jquery-1.11.0.min.js",
        ],
    },
    "crypt_body": ["_one_crypt_something_", "_two_crypt_something_"],
    "replace_body": ["_one_replace_something_", "_two_replace_something_"],
}


def handler(**_):
    return {'text': json.dumps(TO_CRYPT_AS_JSON, sort_keys=True), 'code': 200, 'headers': {'Content-Type': 'text/html'}}


@pytest.mark.parametrize('user_agent, is_crawler', [
    ('Googlebot/2.1 (+http://www.google.com/bot.html)', True),
    ('Mozilla/5.0 (compatible; YandexDirect/3.0; +http://yandex.com/bots)', True),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:66.0) Gecko/20100101 Firefox/66.0', False),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36', False),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.109 YaBrowser/19.3.0.2489 Yowser/2.5 Safari/537.36', False),
])
def test_not_crypting_for_crawlers(stub_server, user_agent, is_crawler, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RE'] = ['test\.local/.*?', 'avatars\.mds\.yandex\.net/get-autoru/.*?']
    new_test_config['CRYPT_RELATIVE_URL_RE'] = ['/relative/.*?']
    new_test_config['CRYPT_BODY_RE'] = ['_\w+_crypt_something_']
    new_test_config['REPLACE_BODY_RE'] = {'_\w+_replace_something_': '_'}

    set_handler_with_config(test_config.name, new_test_config)
    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/page', key, False)
    proxied = requests.get(crypted_link,
                           headers={"host": 'test.local',
                                    PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                    'User-Agent': user_agent}).json()

    if is_crawler:
        assert TO_CRYPT_AS_JSON == proxied
    else:
        for url_class in TO_CRYPT_AS_JSON['urls'].keys():
            for raw_url, proxied_url in zip(TO_CRYPT_AS_JSON['urls'][url_class], proxied['urls'][url_class]):
                assert raw_url != proxied_url
        for content in ('crypt_body', 'replace_body'):
            for raw_content, proxied_content in zip(TO_CRYPT_AS_JSON[content], proxied[content]):
                assert raw_content != proxied_content
