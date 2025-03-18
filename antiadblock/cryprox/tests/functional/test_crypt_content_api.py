# -*- coding: utf8 -*-

import gzip
from io import BytesIO
from urlparse import urljoin

import pytest
import requests
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.mark.parametrize('host_header', [system_config.CRYPTED_HOST_HEADER_NAME, 'host'])
def test_crypt_content_api(cryprox_worker_address, get_config, stub_server, set_handler_with_config, host_header):

    orig_data = requests.get(urljoin(stub_server.url, '/config_bk.html')).text
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['PARTNER_TOKENS'] = ['new_token']
    set_handler_with_config(test_config.name, new_test_config)
    proxied_data = requests.post(urljoin("http://" + cryprox_worker_address, '/crypt_content'), data=orig_data,
                                 headers={system_config.PARTNER_TOKEN_HEADER_NAME: new_test_config['PARTNER_TOKENS'][0],
                                          SEED_HEADER_NAME: 'my2007',
                                          host_header: TEST_LOCAL_HOST,
                                          "content-type": "text/html"})
    expected_data = requests.get(urljoin("http://" + cryprox_worker_address, '/config_bk.html'),
                                 headers={system_config.PARTNER_TOKEN_HEADER_NAME: new_test_config['PARTNER_TOKENS'][0],
                                          SEED_HEADER_NAME: 'my2007',
                                          'host': TEST_LOCAL_HOST})
    assert_that(expected_data.text, equal_to(proxied_data.text))


def test_crypt_content_api_with_gzip(cryprox_worker_address, get_config):
    orig_data = "'href': 'http://an.yandex.ru/system/context.js'"
    expected_data = "'href': 'http://test.local/XWtN9S538/my2007kK/Kdf7P9al87f/5dRTH-B/neCp_J/QDEf_/71cQv4B/2_7rltCU1x/finr9/IH_jnnlS/TX_RN4n/VdzOvqfA/a4guRDZdAKc/'"

    def _gzip(data):
        bytesio = BytesIO()
        with gzip.GzipFile(mode='w', fileobj=bytesio) as gzip_file:
            gzip_file.write(data)
        return bytesio.getvalue()

    test_config = get_config('test_local')

    proxied_data = requests.post(urljoin("http://" + cryprox_worker_address, '/crypt_content'), data=_gzip(orig_data),
                                 headers={system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                          SEED_HEADER_NAME: "my2007",
                                          "host": TEST_LOCAL_HOST,
                                          "content-type": "text/html",
                                          "content-encoding": "gzip"})

    assert_that(expected_data, equal_to(proxied_data.text))


@pytest.mark.parametrize('key,content,expected', [
    ('REPLACE_BODY_RE_PER_URL',
     '<div class="some_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>',
     '<div class="replaced_class_1"><div class="replaced_class_2"><div class="some_class_3"></div></div></div>'),
    ('REPLACE_BODY_RE_EXCEPT_URL',
     '<div class="some_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>',
     '<div class="replaced_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>'),
])
def test_crypt_content_api_with_replace_body(cryprox_worker_address, get_config, set_handler_with_config, key, content, expected):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {r'\bsome_class_1\b': 'replaced_class_1'}
    new_test_config[key] = {
        r'test\.local/crypt_content': {r'\bsome_class_2\b': 'replaced_class_2'},
    }
    set_handler_with_config(test_config.name, new_test_config)

    proxied_data = requests.post(urljoin("http://" + cryprox_worker_address, '/crypt_content'), data=content,
                                 headers={system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                          SEED_HEADER_NAME: "my2007",
                                          "host": TEST_LOCAL_HOST})

    assert_that(proxied_data.text, equal_to(expected))


@pytest.mark.parametrize('csp_header', ['Content-Security-Policy', 'Content-Security-Policy-Report-Only'])
@pytest.mark.parametrize('secon_domaim', ['naydex.net', 'yastatic.net'])
def test_crypt_content_api_with_headers(cryprox_worker_address, get_config, set_handler_with_config, csp_header, secon_domaim):
    csp_string_template = "default-src 'none'; connect-src 'self' *.adfox.ru mc.admetrica.ru *.auto.ru auto.ru *.google-analytics.com *.yandex.net *.yandex.ru yandex.ru yastatic.net " \
                          "wss://push-sandbox.yandex.ru wss://push.yandex.ru{second_domain_placeholder};font-src 'self' data: fonts.googleapis.com *.gstatic.com an.yandex.ru yastatic.net journalcdn.storage.yandexcloud.net " \
                          "autoru-mag.s3.yandex.net{second_domain_placeholder};form-action 'self' money.yandex.ru auth.auto.ru;frame-src *.adfox.ru *.auto.ru auto.ru i.autoi.ru *.avto.ru *.yandex.net *.yandex.ru yandex.ru " \
                          "*.yandexadexchange.net yandexadexchange.net yastatic.net *.youtube.com *.vertis.yandex-team.ru *.tinkoff.ru *.spincar.com{second_domain_placeholder};img-src 'self' data: blob: ad.adriver.ru *.adfox.ru " \
                          "*.auto.ru auto.ru *.autoi.ru *.tns-counter.ru mc.webvisor.org *.yandex.net yandex.net *.yandex.ru yandex.ru yastatic.net *.ytimg.com mc.admetrica.ru journalcdn.storage.yandexcloud.net " \
                          "autoru-mag.s3.yandex.net t.co analytics.twitter.com *.google.com *.google.ru *.google-analytics.com *.googleadservices.com *.googlesyndication.com *.gstatic.com{second_domain_placeholder};media-src " \
                          "data: *.adfox.ru *.yandex.net *.yandex.ru yandex.st yastatic.net{second_domain_placeholder}; object-src data: *.adfox.ru *.googlesyndication.com{second_domain_placeholder};script-src 'unsafe-eval' 'unsafe-inline' " \
                          "*.auto.ru auto.ru *.adfox.ru *.yandex.ru yandex.ru *.yandex.net yastatic.net static.yastatic.net *.youtube.com *.ytimg.com journalcdn.storage.yandexcloud.net autoru-mag.s3.yandex.net " \
                          "static.ads-twitter.com *.twitter.com *.google-analytics.com *.googleadservices.com *.googlesyndication.com *.gstatic.com{second_domain_placeholder};style-src 'self' 'unsafe-inline' *.adfox.ru " \
                          "*.auto.ru auto.ru yastatic.net static.yastatic.net journalcdn.storage.yandexcloud.net autoru-mag.s3.yandex.net{second_domain_placeholder};report-uri " \
                          "https://csp.yandex.net/csp?from=autoru-frontend-desktop&version=201904.16.121024&yandexuid=4192397941554369671"
    csp_string = csp_string_template.format(second_domain_placeholder='')

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['PARTNER_TOKENS'] = ['new_token']
    new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
    new_test_config['PARTNER_COOKIELESS_DOMAIN'] = secon_domaim
    set_handler_with_config(test_config.name, new_test_config)
    test_config_edited = get_config('test_local')
    headers = {
        system_config.PARTNER_TOKEN_HEADER_NAME: new_test_config['PARTNER_TOKENS'][0],
        SEED_HEADER_NAME: 'my2007',
        'host': TEST_LOCAL_HOST,
        'content-type': 'text/html',
        csp_header: csp_string
    }
    if secon_domaim == 'yastatic.net':
        expected_csp_string = csp_string
    else:
        expected_csp_string = csp_string_template.format(second_domain_placeholder=' ' + test_config_edited.second_domain)
    proxied_data = requests.post(
        urljoin("http://" + cryprox_worker_address, '/crypt_content'),
        data="",
        headers=headers,
    )
    assert proxied_data.headers[csp_header] == expected_csp_string
