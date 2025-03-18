# -*- coding: utf8 -*-
from urlparse import urljoin, parse_qs

import cchardet as chardet
import pytest
import requests
import re2
from werkzeug.wrappers import BaseResponse
from hamcrest import assert_that, equal_to

from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, body_replace
from antiadblock.cryprox.cryprox.common.tools.regexp import re_merge
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST

test_data = u"""
    Ya[1520335144833]('{ common:{stationaryConnection:"1",
    linkHead:"https://an.yandex.ru/count/dfdfdfsdfdfdfd",
    syntheticMatch:"",pageLang:"1",userGroup:"0",userRegion:"RU",commonRtbVisibility:"1",
    secondTitle: "", titleHyph: "", body: "Осязаемая атмосфера успеха.&laquo;Исчезающий город&raquo;
     Райта&ndash;мечта, воплощенная в реальность.", bodyHyph: "", domain: "wright-village.ru",
     punyDomain: "wright-village.ru", region: "Москва", address: "Райт Вилладж", workingTime: "пн-вс 8:00-23:00"
    """


class Response(BaseResponse):

    def __init__(self, body_charset, *a, **kw):
        self.charset = body_charset
        super(Response, self).__init__(*a, **kw)


def handler_charset(**request):
    query = parse_qs(request.get('query', ''))
    charset = query.get('charset', [''])[0]

    response_text = test_data
    content_type_charset = query.get('content-type-charset', [None])[0]
    if content_type_charset is not None:
        headers = {'Content-Type': 'text/plain; charset={}'.format(content_type_charset)}
    else:
        headers = {}
    if charset != '':
        return Response(response=response_text, status=200, body_charset=charset, headers=headers)
    else:
        return {'text': 'Charset arg not found', 'code': 400}


@pytest.mark.parametrize('charset, header_charset, expected_content_type_charset', [
    ('utf-8', None, 'utf-8'),
    ('windows-1251', None, 'windows-1251'),
    ('koi8-r', None, 'koi8-r'),
    ('utf-8', 'utf-8', 'utf-8'),
    ('windows-1251', 'utf-7', 'utf-7'),
    ('koi8-r', 'utf-8', 'utf-8')])
def test_charset_handler(stub_server, charset, header_charset, expected_content_type_charset):
    stub_server.set_handler(handler_charset)

    url_path = '/test_charset?charset=' + charset
    if header_charset is not None:
        url_path += '&content-type-charset=' + header_charset

    response = requests.get(urljoin(stub_server.url, url_path), headers={'host': TEST_LOCAL_HOST})

    assert chardet.detect(response.content)['encoding'].lower() == charset
    assert response.headers.get('Content-Type', '') == 'text/plain; charset={}'.format(expected_content_type_charset)
    assert response.apparent_encoding.lower() == charset

    assert response.status_code == 200


@pytest.mark.parametrize('charset, header_charset', [
    ('utf-8', None),
    ('windows-1251', None),
    ('koi8-r', None),
    ('utf-8', 'utf-8'),
    ('utf-8', 'windows-1251'),  # Кейс от liveinternet - передавали в заголовке windows-1251, а верстка была в utf-8
    ('koi8-r', 'utf-8')])
def test_body_replace_charset(stub_server, get_key_and_binurlprefix_from_config, get_config, charset, header_charset):
    """
    Тикет-родитель теста и проблемы в целом: https://st.yandex-team.ru/ANTIADB-760
    :param charset: кодировка, в которой тестовая заглушка-партнер отдаст тестовый текст
    :param header_charset: кодировка, котовая будет передана при этом заглушкой-партнером в заголовке Content-Type
    """
    test_config = get_config('test_local')
    stub_server.set_handler(handler_charset)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    url_path = '/test_charset?charset=' + charset
    if header_charset is not None:
        url_path += '&content-type-charset=' + header_charset

    crypted_link = crypt_url(binurlprefix,
                             urljoin('http://{}'.format(TEST_LOCAL_HOST), url_path),
                             key,
                             enable_trailing_slash=True)

    original_data = requests.get(urljoin(stub_server.url, url_path)).text
    proxied_response = requests.get(crypted_link,
                                    headers={"host": TEST_LOCAL_HOST,
                                             system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    expected_data = test_data.replace('Ya[', 'Ya [')
    proxied_data = proxied_response.text

    expected_charset = header_charset if header_charset else charset
    assert chardet.detect(proxied_response.content)['encoding'].lower() == expected_charset
    assert proxied_data == expected_data and proxied_data != original_data
    assert proxied_response.status_code == 200


replace_in_order_html_template = '''
<!DOCTYPE html>
<html>
<body>
    <div>This is {}!</div>
</body>
</html>
'''


@pytest.mark.parametrize('crypt_list, crypt',
                         [([r'\bAdvertising\b'], True),
                          ([r'\bReklama\b'], False)])
def test_crypt_and_replace_in_order(get_config, set_handler_with_config, stub_server, get_key_and_binurlprefix_from_config, crypt_list, crypt):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    replace_dict = {r'\bReklama\b': 'Advertising'}

    new_test_config['REPLACE_BODY_RE'] = replace_dict
    new_test_config['CRYPT_BODY_RE'] = crypt_list
    new_test_config['CRYPT_URL_RE'] = ['test\.local/html.*?']

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/html', key, False)

    set_handler_with_config(test_config.name, new_test_config)

    def handler(**_):
        return {'text': replace_in_order_html_template.format('Reklama'), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)
    expected_word = body_replace('Advertising', key, {re2.compile(re_merge(crypt_list)): None}) if crypt else 'Advertising'
    # Replace and then crypt
    response = requests.get(crypted_link, headers={"HOST": 'test.local',
                                                   system_config.SEED_HEADER_NAME: 'my2007',
                                                   system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.text == replace_in_order_html_template.format(expected_word)


def test_replace_with_nothing(get_config, set_handler_with_config, stub_server, get_key_and_binurlprefix_from_config):
    content_to_replace = 'some_content12345'

    def handler(**_):
        return {'text': replace_in_order_html_template.format(content_to_replace), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {content_to_replace: ''}
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/html', key, False)

    crypted = requests.get(crypted_link, headers={"HOST": 'test.local', system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert crypted == replace_in_order_html_template.format('')


def test_crypt_url_and_content_order(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = '<div class="adfox"></div><script src="//yastatic.net/adfox/loader.js"></script>'
    expected_content = '<div class="ombKPYPE"></div><script src="//test.local/XWtN7S903/my20071_/mQbvqmJEo8M/sBeR2_L/hKq95s/NfBOP/u1MxyrR/iixYNUPFBP/VRmM2/bvupV3Xf/0TcefQBcM3l/"></script>'

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    url = "http://test.local/1.html"

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_BODY_RE'] = [r'\badfox\b']
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, url, key, False, origin='test.local'), headers={'host': 'test.local', system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied.status_code == 200
    assert_that(proxied.text, equal_to(expected_content))


def test_replace_body_re_per_url(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = '<div class="some_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>'
    expected_content = {
        'default': '<div class="replaced_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>',
        'path1': '<div class="replaced_class_1"><div class="replaced_class_2"><div class="some_class_3"></div></div></div>',
        'path2': '<div class="replaced_class_1"><div class="replaced_class_2"><div class="some__4"></div></div></div>',
    }

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {r'\bsome_class_1\b': 'replaced_class_1'}
    new_test_config['REPLACE_BODY_RE_PER_URL'] = {
        r'test\.local/path1': {r'\bsome_class_2\b': 'replaced_class_2'},
        r'test\.local/path2': {r'\bsome_class_2\b': 'replaced_class_2', r'\bsome_(class_3)\b': '_4'},
    }
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    for path, expected in expected_content.items():
        url = urljoin("http://test.local", path)
        crypted_link = crypt_url(binurlprefix, url, key, False)
        proxied = requests.get(crypted_link, headers={'host': 'test.local', system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
        assert proxied.status_code == 200
        assert_that(proxied.text, equal_to(expected))


def test_replace_body_re_except_url(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = '<div class="some_class_1"><div class="some_class_2"><div class="some_class_3"></div></div></div>'
    expected_content = {
        'default': '<div class="replaced_class_1"><div class="replaced_class_2"><div class="some__4"></div></div></div>',
        'path1': '<div class="replaced_class_1"><div class="some_class_2"><div class="some__4"></div></div></div>',
        'path2': '<div class="replaced_class_1"><div class="replaced_class_2"><div class="some_class_3"></div></div></div>',
    }

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['REPLACE_BODY_RE'] = {r'\bsome_class_1\b': 'replaced_class_1'}
    new_test_config['REPLACE_BODY_RE_EXCEPT_URL'] = {
        r'test\.local/path1': {r'\bsome_class_2\b': 'replaced_class_2'},
        r'test\.local/path2': {r'\bsome_(class_3)\b': '_4'},
    }
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    for path, expected in expected_content.items():
        url = urljoin("http://test.local", path)
        crypted_link = crypt_url(binurlprefix, url, key, False)
        proxied = requests.get(crypted_link, headers={'host': 'test.local', system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
        assert proxied.status_code == 200
        assert_that(proxied.text, equal_to(expected))
