# -*- coding: utf8 -*-
import json
import copy

import re2
import pytest
import requests
from urllib import quote, unquote
from urlparse import urljoin, urlparse
from hamcrest import assert_that, equal_to
from base64 import urlsafe_b64encode, urlsafe_b64decode, b64decode, b64encode

from library.python import resource

from antiadblock.libs.decrypt_url.lib import decrypt_url
from antiadblock.cryprox.cryprox.config.service import COOKIELESS_PATH_PREFIX
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, FETCH_URL_HEADER_NAME
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, body_crypt, body_replace, CryptUrlPrefix


bk_rtb_auction_response_template = "Ya [1524124928839]('{}')"

css_decoded = resource.find("resources/test_rtb_auction_response_processing/css_decoded.css")
html_decoded = resource.find("resources/test_rtb_auction_response_processing/html_decoded.html")
adfox_banner_html = resource.find("resources/test_rtb_auction_response_processing/adfox_banner_html.html")

crypted_rambler_banner_html = resource.find("resources/test_rtb_auction_response_processing/crypted_rambler_banner_html.html")
rambler_response_client_side = resource.find("resources/test_rtb_auction_response_processing/rambler_auction_response_client_side.jsp")
rambler_response_server_side = resource.find("resources/test_rtb_auction_response_processing/rambler_auction_response_server_side.jsp")

bk_rtb_auction_ssr_response_json_base64 = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_ssr_response.json") %
               ("\"{}\"".format(urlsafe_b64encode(html_decoded)), "\"{}\"".format(urlsafe_b64encode(css_decoded)), "\"base64\""))

bk_rtb_auction_ssr_response_json_encodeURI = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_ssr_response.json") %
               ("\"{}\"".format(quote(html_decoded, safe="-_.!~*'()")), "\"{}\"".format(quote(css_decoded, safe="-_.!~*'()")), "\"URI\""))

meta_bulk_auction_ssr_response_json = {
    "responseData": {
        "ads": [
            {
                "common": {
                    "canRetry": 0,
                    "isYandexPage": "1",
                    "isYandex": "1",
                    "reloadTimeout": "30"
                },
                "rtbAuctionInfo": {
                    "dspId": 10,
                    "bidReqId": 2096613328257593666
                }
            },
            copy.deepcopy(bk_rtb_auction_ssr_response_json_base64),
            copy.deepcopy(bk_rtb_auction_ssr_response_json_base64),
            copy.deepcopy(bk_rtb_auction_ssr_response_json_base64),
        ]
    }
}

bk_rtb_auction_widget_ssr_response_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_widget_ssr_response.json") %
               ("\"{}\"".format(quote(html_decoded, safe="-_.!~*'()")), "\"{}\"".format(quote(css_decoded, safe="-_.!~*'()"))))

bk_rtb_auction_widget_combo_ssr_response_json = copy.deepcopy(bk_rtb_auction_widget_ssr_response_json)
bk_rtb_auction_widget_combo_ssr_response_json["settings"] = {
    '2021': copy.deepcopy(bk_rtb_auction_widget_ssr_response_json['settings'])
}

bk_rtb_auction_smartbanner_response_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_response.json") % "\"\"")
bk_rtb_auction_smartbanner_response_json["rtb"].pop("html", None)
smartbanner_data = json.loads(resource.find("resources/test_rtb_auction_response_processing/smartbanner_data.json"))
bk_rtb_auction_smartbanner_response_json["rtb"]["data"] = smartbanner_data

bk_rtb_auction_smartbanner_ssr_response_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_response.json") % "\"\"")
bk_rtb_auction_smartbanner_ssr_response_json['rtb']['html'] = html_decoded
bk_rtb_auction_smartbanner_ssr_response_json['rtb']['isSmartBanner'] = True

bk_rtb_auction_response_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_response.json") % "\"{}\"".format(urlsafe_b64encode(html_decoded)))

bk_rtb_auction_response_without_rtb_json = json.loads(resource.find("resources/test_rtb_auction_response_processing/bk_without_rtb_auction_response.json"))
adfox_rtb_getbulk_auction_response_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json") %
               ("\"banner.direct\"", "\"dataBase64\"", "\"{}\"".format(urlsafe_b64encode(json.dumps(bk_rtb_auction_response_json)))))

adfox_rtb_getbulk_auction_response_json_not_base64 = copy.deepcopy(adfox_rtb_getbulk_auction_response_json)
adfox_rtb_getbulk_auction_response_json_not_base64['data'][0]['attributes']['data'] = copy.deepcopy(bk_rtb_auction_response_json)
del adfox_rtb_getbulk_auction_response_json_not_base64['data'][0]['attributes']['dataBase64']

adfox_rtb_getbulk_auction_ssr_response_json = copy.deepcopy(adfox_rtb_getbulk_auction_response_json)
adfox_rtb_getbulk_auction_ssr_response_json['data'][0]['attributes']['data'] = copy.deepcopy(bk_rtb_auction_ssr_response_json_base64)
del adfox_rtb_getbulk_auction_ssr_response_json['data'][0]['attributes']['dataBase64']

adfox_rtb_getcode_auction_response_json = \
    resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getcode_auction_response.js") % "\"{}\"".format(json.dumps(bk_rtb_auction_response_json))

bk_without_html_rtb_auction_response_js = resource.find('resources/test_rtb_auction_response_processing/bk_without_html_rtb_auction_response.js')
adfox_rtb_getbulk_auction_response_without_html_field_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json") %
               ("\"banner.direct\"", "\"dataBase64\"", "\"{}\"".format(urlsafe_b64encode(bk_without_html_rtb_auction_response_js))))
adfox_rtb_getbulk_auction_response_without_rtb_field_json = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json") %
               ("\"banner.direct\"", "\"dataBase64\"", "\"{}\"".format(urlsafe_b64encode(json.dumps(bk_rtb_auction_response_without_rtb_json)))))
adfox_rtb_getbulk_auction_response_json_html_banner_base64 = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json") %
               ("\"banner.html\"", "\"htmlBase64\"", "\"{}\"".format(urlsafe_b64encode(adfox_banner_html))))
adfox_rtb_getbulk_auction_response_json_html_banner_encodeURI = \
    json.loads(resource.find("resources/test_rtb_auction_response_processing/adfox_rtb_getbulk_auction_response.json") %
               ("\"banner.html\"", "\"htmlEncoded\"", "\"{}\"".format(quote(adfox_banner_html, safe="-_.!~*'()"))))

adfox_errors_response = {
    "errors": [{
        "status": "204",
        "id": "3546784114"
    }],
    "meta": {
        "session_id": "784603512",
        "request_id": "1"
    },
    "data": urlsafe_b64encode(json.dumps(bk_rtb_auction_response_json)),
    "jsonapi": {
        "version": "1.0",
        "meta": {
            "protocol_version": "2.0"
        }
    }
}


def escape_chars(text, characters='\\"\n'):
    for character in characters:
        text = text.replace(character, '\\' + character)
    return text


def handler(**request):
    path = request.get('path', '/')
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.direct
    if path in ('/12345/getBulk/html_repack_direct.js', '/adfox/12345/getBulk/html_repack_direct.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_json),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.direct, вместо поля dataBase64 поле data (не завернутое в base64)
    elif path in ('/12345/getBulk/html_repack_direct_not_base64.js', '/adfox/12345/getBulk/html_repack_direct_not_base64.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_json_not_base64),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.direct (SSR)
    elif path in ('/12345/getBulk/html_repack_direct_ssr.js', '/adfox/12345/getBulk/html_repack_direct_ssr.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_ssr_response_json),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.html (base64)
    elif path in ('/12345/getBulk/html_repack_html_base64.js', '/adfox/12345/getBulk/html_repack_html_base64.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_json_html_banner_base64),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.html (encodeURI)
    elif path in ('/12345/getBulk/html_repack_html_encoded.js', '/adfox/12345/getBulk/html_repack_html_encoded.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_json_html_banner_encodeURI),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с типом креатива banner.image (шаблон "Картинка")
    elif path in ('/12345/getBulk/html_repack_image.js', '/adfox/12345/getBulk/html_repack_image.js'):
        modified_answer = adfox_rtb_getbulk_auction_response_json
        modified_answer['data']['type'] = 'direct.image'
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_json),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getCode с типом креатива banner.image (шаблон "Картинка")
    elif path == '/12345/getCode/html_repack.js':
        return {'text': json.dumps(adfox_rtb_getcode_auction_response_json),
                'code': 200,
                'headers': {'content-type': 'application/javascript; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk с креативом лежащим в ответе, без поля html
    elif path in ('/12345/getBulk/html_repack_without_html.js', '/adfox/12345/getBulk/html_repack_without_html.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_without_html_field_json),
                'code': 200,
                'headers': {'content-type': 'application/javascript; charset=utf-8'}}
    # Имитация ответа адфокса по ручке getBulk без поля rtb
    elif path in ('/12345/getBulk/html_repack_without_rtb.js', '/adfox/12345/getBulk/html_repack_without_rtb.js'):
        return {'text': json.dumps(adfox_rtb_getbulk_auction_response_without_rtb_field_json),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    # Имитация ответа адфокса с кодом 200, но с ошибками внутри ответа
    elif path in ('/12345/getBulk/errors', '/adfox/12345/getBulk/errors'):
        return {'text': json.dumps(adfox_errors_response),
                'code': 200,
                'headers': {'content-type': 'application/json; charset=utf-8'}}
    return {'text': 'What are u looking for?', 'code': 404}


def test_meta_bulk_repack(stub_server, get_key_and_binurlprefix_from_config, get_config):

    # Имитация ответа от БК
    def handler(**_):
        return {'text': json.dumps(meta_bulk_auction_ssr_response_json), 'code': 200, 'headers': {'content-type': 'application/javascript; charset=utf-8'}}

    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    original_url = 'http://an.yandex.ru/meta_bulk/123456'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')
    data = {
        "requestData": {
            "imps": [
                {
                    "queryParameters": {
                        "imp-id": 419,
                        "partner-stat-id": "120",
                        "layout-config": "{\"visible\":1,\"ad_no\":0,\"req_no\":1,\"width\":442}"
                    }
                },
                {
                    "queryParameters": {
                        "imp-id": 420,
                        "partner-stat-id": "120",
                        "layout-config": "{\"visible\":1,\"ad_no\":0,\"req_no\":1,\"width\":442}"
                    }
                },
                {
                    "queryParameters": {
                        "imp-id": 421,
                        "partner-stat-id": "120",
                        "layout-config": "{\"visible\":1,\"ad_no\":0,\"req_no\":1,\"width\":442}"
                    }
                }
            ],
            "common": {
                "queryParameters": {
                    "target-ref": "https://zen.yandex.ru",
                    "charset": "utf-8",
                    "duid": "MTU3NTYzNjQ3MDMyMDMyNzE2",
                    "redir-setuniq": 1,
                    "server-side-rendering-enabled-formats": [
                        "zen2",
                        "zen"
                    ]
                }
            }
        }
    }

    proxied_data = requests.post(crypted_url,
                                 headers={'host': 'test.local', SEED_HEADER_NAME: seed, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                                 json=data)
    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    base_path = test_config.html5_base_path_re.search(html_decoded)
    expected_html_data = body_crypt(body=html_decoded,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_html5_relative_url_re,
                                    enable_trailing_slash=False,
                                    file_url=base_path.group(1),
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
    expected_html_data = body_crypt(expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_url_re_with_counts,
                                    enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                    partner_url_re_match=test_config.partner_url_re)

    proxied_data = proxied_data.json()
    count_creatives = 0
    for creative in proxied_data["responseData"]["ads"]:
        if "direct" in creative:
            count_creatives += 1
            data = creative['direct']['ssr']
            data = urlsafe_b64decode(data['html'].encode('utf-8'))
            assert_that(data, equal_to(expected_html_data))
    assert count_creatives == 3


@pytest.mark.parametrize('encoding', [None, 'raw', 'base64'])
def test_bk_rtb_meta_media_ssr_repack(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, encoding):

    content = """{{"rtb": {{"ssr": {{"html": "{}","css": "{}"{}}}}},"common":{{}}}}"""
    if encoding is None or encoding == "raw":
        content = content.format(
            escape_chars(html_decoded.replace("\n", "")),
            escape_chars(css_decoded.replace("\n", "")),
            "" if encoding is None else ",\"encoding\": \"raw\""
        )
    else:
        content = content.format(urlsafe_b64encode(html_decoded), urlsafe_b64encode(css_decoded), ",\"encoding\": \"base64\"")

    # Имитация ответа от БК
    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'content-type': 'application/json; charset=utf-8'}}

    stub_server.set_handler(handler)
    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    original_url = 'http://an.yandex.ru/meta/media_repack.js?woah=bing'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.headers['Content-Type'] == 'application/json'
    proxied_data = proxied_data.json()
    assert "common" in proxied_data

    # check ssr field
    ssr_dict = proxied_data['rtb']['ssr']
    if encoding is None or encoding == "raw":
        expected_html_data = html_decoded.replace("\n", "")
        html_data = ssr_dict['html']
    else:
        expected_html_data = html_decoded
        html_data = urlsafe_b64decode(ssr_dict['html'].encode('utf-8'))

    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    base_path = test_config.html5_base_path_re.search(expected_html_data)
    expected_html_data = body_crypt(body=expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_html5_relative_url_re,
                                    enable_trailing_slash=False,
                                    file_url=base_path.group(1),
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
    expected_html_data = body_crypt(expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_url_re_with_counts,
                                    enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                    partner_url_re_match=test_config.partner_url_re)

    assert_that(html_data, equal_to(expected_html_data))


@pytest.mark.parametrize('vast_key', ['vast', 'vastBase64', 'video'])
def test_bk_rtb_meta_vast_repack(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, vast_key):

    vast_decoded = resource.find("resources/test_rtb_auction_response_processing/vast_decoded.xml").replace("\n", "")
    vast_decoded_crypted = resource.find("resources/test_rtb_auction_response_processing/vast_decoded_crypted.xml").replace("\n", "")
    content = """{{"rtb": {{"{}": "{}","ssr": {{"html": "{}", "css": "{}","encoding": "{}"}}}},"common":{{}}}}"""
    if vast_key == "vast":
        content = content.format(
            vast_key,
            escape_chars(vast_decoded),
            escape_chars(html_decoded.replace("\n", "")),
            escape_chars(css_decoded.replace("\n", "")),
            "raw"
        )
    else:
        content = content.format(vast_key, urlsafe_b64encode(vast_decoded), urlsafe_b64encode(html_decoded), urlsafe_b64encode(css_decoded), "base64")

    # Имитация ответа от БК
    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'content-type': 'application/json; charset=utf-8'}}

    stub_server.set_handler(handler)
    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    original_url = 'http://an.yandex.ru/meta/vast_repack.js?woah=bing'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.headers['Content-Type'] == 'application/json'
    proxied_data = proxied_data.json()
    assert "common" in proxied_data
    if vast_key == 'vast':
        data = proxied_data['rtb'][vast_key]
    else:
        data = urlsafe_b64decode(proxied_data['rtb'][vast_key].encode('utf-8'))
    assert_that(data, equal_to(vast_decoded_crypted))

    # check ssr field
    ssr_dict = proxied_data['rtb']['ssr']
    if vast_key == "vast":
        expected_html_data = html_decoded.replace("\n", "")
        html_data = ssr_dict['html']
    else:
        expected_html_data = html_decoded
        html_data = urlsafe_b64decode(ssr_dict['html'].encode('utf-8'))

    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    base_path = test_config.html5_base_path_re.search(expected_html_data)
    expected_html_data = body_crypt(body=expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_html5_relative_url_re,
                                    enable_trailing_slash=False,
                                    file_url=base_path.group(1),
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
    expected_html_data = body_crypt(expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_url_re_with_counts,
                                    enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                    partner_url_re_match=test_config.partner_url_re)

    assert_that(html_data, equal_to(expected_html_data))


@pytest.mark.parametrize('ssr,widget,comboblock,smartbanner,encoding', [
    (False, False, False, False, ''),  # обычный ответ БК (значения widget,comboblock,encoding не важны)
    (True, True, False, False, ''),  # Widget SSR (значение encoding не важно)
    (True, True, True, False, ''),  # Widget SSR (значение encoding не важно)
    (True, False, False, True, ''),  # smartbanner SSR (значения comboblock, encoding не важно)
    (True, False, False, False, 'URI'),  # PCODE SSR, контент завернут в encodeURIComponent
    (True, False, False, False, 'base64'),  # PCODE SSR, контент завернут в base64
])
def test_bk_rtb_meta_repack(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, ssr, widget, comboblock, smartbanner, encoding):
    if ssr:
        if widget:
            if comboblock:
                content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_widget_combo_ssr_response_json))
            else:
                content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_widget_ssr_response_json))
        elif smartbanner:
            content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_smartbanner_ssr_response_json))
        elif encoding == "URI":
            content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_ssr_response_json_encodeURI))
        else:
            content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_ssr_response_json_base64))
    else:
        content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_response_json))

    # Имитация стандартного ответа от БК
    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'content-type': 'application/javascript; charset=utf-8'}}

    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    original_url = 'http://an.yandex.ru/meta/html_repack.js?woah=bing'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    base_path = test_config.html5_base_path_re.search(html_decoded)
    expected_html_data = body_crypt(body=html_decoded,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_html5_relative_url_re,
                                    enable_trailing_slash=False,
                                    file_url=base_path.group(1),
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
    expected_html_data = body_crypt(expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_url_re_with_counts,
                                    enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                    partner_url_re_match=test_config.partner_url_re)

    assert proxied_data.headers['Content-Type'] == 'application/json'
    proxied_data = proxied_data.json()
    if ssr:
        if widget:
            data = {}
            if comboblock:
                for block_id in proxied_data['settings']:
                    data = proxied_data['settings'][block_id]['ssr']
                    break
            else:
                data = proxied_data['settings']['ssr']
            data = unquote(data['html'].encode('utf-8'))
        elif smartbanner:
            data = proxied_data['rtb']['html']
        else:
            data = proxied_data['direct']['ssr']
            if encoding == 'URI':
                data = unquote(data['html'].encode('utf-8'))
            else:
                data = urlsafe_b64decode(data['html'].encode('utf-8'))
        assert_that(data, equal_to(expected_html_data))
    else:
        assert 'url' not in proxied_data['rtb'].keys()
        assert_that(urlsafe_b64decode(proxied_data['rtb']['html'].encode('utf-8')), equal_to(expected_html_data))
    # Проверяем, что мы пошифровали относительный урл в html5 креативе canvas-а относительно basePath:
    relative_img_src_re = re2.compile('<img .*? src="(.*?)" .*?/>')
    crypted_relative_img_src = relative_img_src_re.search(expected_html_data).group(1)

    crypted_relative_img_src = urlparse(crypted_relative_img_src)._replace(scheme="", netloc='').geturl()

    original_relative_img_src = relative_img_src_re.search(html_decoded).group(1)
    assert_that(decrypt_url(crypted_relative_img_src, str(test_config.CRYPT_SECRET_KEY), str(test_config.CRYPT_PREFFIXES),
                            test_config.CRYPT_ENABLE_TRAILING_SLASH)[0], equal_to(urljoin(base_path.group(1), original_relative_img_src)))


@pytest.mark.parametrize('request_via_script, expected_type', [
    (False, 'application/json'),
    (True, 'application/javascript'),
])
def test_bk_rtb_meta_repack_with_data_json(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, request_via_script, expected_type):
    content = bk_rtb_auction_response_template.format(json.dumps(bk_rtb_auction_smartbanner_response_json))

    # Имитация ответа от БК
    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'content-type': 'application/javascript; charset=utf-8'}}

    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    original_url = 'http://an.yandex.ru/meta/html_repack.js?woah=bing&callback=Ya%5B1524124928839%5D'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.headers['Content-Type'] == expected_type
    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    expected_data = body_crypt(json.dumps(smartbanner_data),
                               binurlprefix=binurlprefix_for_bodycrypt,
                               key=key,
                               crypt_url_re=test_config.crypt_url_re_with_counts,
                               enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                               min_length=test_config.RAW_URL_MIN_LENGTH,
                               crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                               partner_url_re_match=test_config.partner_url_re)

    if not request_via_script:
        data = proxied_data.json()["rtb"]["data"]
    else:
        match_object = re2.match(r'Ya\[1524124928839\]\(\'(.*?)\'\)', proxied_data.text)
        data = json.loads(match_object.group(1).replace('\\\\', '\\'))['rtb']['data']
    assert_that(data, equal_to(json.loads(expected_data)))


@pytest.mark.parametrize('html_field_exists', [True, False])
def test_bk_rtb_meta_repack_html_field_missed(html_field_exists, stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    # Имитация стандартного ответа от БК
    def handler(**_):
        response_json = copy.deepcopy(bk_rtb_auction_response_json)
        if not html_field_exists:
            # Имитация стандартного ответа от БК, но без поля html с base64 кодом креатива
            del response_json['rtb']['html']
        return {'text': bk_rtb_auction_response_template.format(json.dumps(response_json)), 'code': 200, 'headers': {'content-type': 'application/javascript; charset=utf-8'}}

    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    original_url = 'http://an.yandex.ru/meta/html_repack.js?woah=bing'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')
    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    base_path = test_config.html5_base_path_re.search(html_decoded)
    expected_html_data = body_crypt(body=html_decoded,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_html5_relative_url_re,
                                    enable_trailing_slash=False,
                                    file_url=base_path.group(1),
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
    expected_html_data = body_crypt(expected_html_data,
                                    binurlprefix=binurlprefix_for_bodycrypt,
                                    key=key,
                                    crypt_url_re=test_config.crypt_url_re_with_counts,
                                    enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                    min_length=test_config.RAW_URL_MIN_LENGTH,
                                    crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                    partner_url_re_match=test_config.partner_url_re)

    if html_field_exists:
        proxied_data = proxied_data.json()
        assert 'url' not in proxied_data['rtb'].keys()
        assert_that(urlsafe_b64decode(proxied_data['rtb']['html'].encode('utf-8')), equal_to(expected_html_data))
    else:
        bk_rtb_auction_response_json_missed = bk_rtb_auction_response_json
        del bk_rtb_auction_response_json_missed['rtb']['html']
        assert_that(proxied_data.content, equal_to(json.dumps(bk_rtb_auction_response_json_missed)))


@pytest.mark.parametrize('second_domain', [None, 'naydex.net', 'yastatic.net'])
def test_bk_rtb_meta_repack_external_base_path(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, second_domain):
    response = resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path.json")
    expected_content = resource.find("resources/test_rtb_auction_response_processing/bk_rtb_auction_response_external_base_path_expected_crypted.json")

    # Имитация стандартного ответа от БК
    def handler(**_):
        return {'text': response, 'code': 200, 'headers': {'content-type': 'application/json'}}

    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False
    if second_domain is not None:
        new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
        new_test_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    original_url = 'http://an.yandex.ru/meta/html_repack.js?woah=bing'
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')
    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if second_domain is None:
        assert_that(proxied_data.content, equal_to(expected_content))
    else:
        if second_domain != 'yastatic.net':
            # test-local.naydex.net/test/...
            replaced = "test-local.{}{}".format(second_domain, COOKIELESS_PATH_PREFIX)
        else:
            # yastatic.net/naydex/test-local/test/...
            replaced = "{}/naydex/test-local{}".format(second_domain, COOKIELESS_PATH_PREFIX)

        proxied = json.loads(proxied_data.content)
        expected = json.loads(expected_content)
        # replace `test.local` to cookieless in expected content
        assert_that(proxied["rtb"]["basePath"], equal_to(expected["rtb"]["basePath"].replace("test.local/", replaced)))
        proxied_html = urlsafe_b64decode(proxied["rtb"]["html"].encode("utf-8"))
        expected_html = urlsafe_b64decode(expected["rtb"]["html"].encode("utf-8")).replace("test.local/", replaced)
        assert_that(proxied_html, equal_to(expected_html))


rtb_json = """{"data": [], "rtb": {"targetUrl": "otkryt'_demo_schet", "html": "", "abuseLink": ""}}"""
rtb_json_repacked = """{"data": [], "rtb": {"html": "", "abuseLink": "", "targetUrl": "otkryt'_demo_schet"}}"""
rtb_answer = "Ya [1524124928839]('{}')".format(rtb_json.replace("'", "\\'"))
rtb_links = {
    'BK': 'http://an.yandex.ru/meta/rtb_auction_sample?test=woah&callback=Ya%5B1524124928839%5D',
    'ADFOX': 'http://ads.adfox.ru/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=&callback=Ya%5B1524124928839%5D',
    'AN_ADFOX': 'http://an.yandex.ru/adfox/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=&callback=Ya%5B1524124928839%5D',
    'BK_SERVERSIDE': 'http://an.yandex.ru/meta/92550?imp-id=712&page-ref=https%3A%2F%2Fmail.yandex.ru%2F&target-ref=https%3A%2F%2Fmail.yandex.ru%2F%3Fuid%3D572843837&layout-config=%7B%22win_width'
                     '%22%3A1745%2C%22win_height%22%3A889%2C%22width%22%3A1410%2C%22height%22%3A0%2C%22left%22%3A297%2C%22top%22%3A68%2C%22visible%22%3A1%2C%22ad_no%22%3A0%2C%22req_no%22%3A0%7D&'
                     'charset=utf-8&server-side=1',
    'BK_WITHOUT_CALLBACK': 'http://an.yandex.ru/meta/rtb_auction_sample?test=woah',
    'ADFOX_WITHOUT_CALLBACK': 'http://ads.adfox.ru/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=',
    'AN_ADFOX_WITHOUT_CALLBACK': 'http://an.yandex.ru/adfox/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=',
}


@pytest.mark.parametrize('rtb_response_is_json, request_via_script, rtb_request, expected_type, expected_answer', [
    (True, True, rtb_links['BK'], 'application/javascript', "Ya[1524124928839]('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (True, True, rtb_links['ADFOX'], 'application/javascript', "Ya[1524124928839]('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (True, True, rtb_links['AN_ADFOX'], 'application/javascript', "Ya[1524124928839]('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (False, True, rtb_links['BK'], 'application/javascript', "Ya[1524124928839]('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (False, True, rtb_links['ADFOX'], 'application/javascript', rtb_answer),  # failed to repack
    (False, True, rtb_links['AN_ADFOX'], 'application/javascript', rtb_answer),  # failed to repack
    (True, False, rtb_links['BK'], 'application/json', rtb_json_repacked),
    (True, False, rtb_links['ADFOX'], 'application/json', rtb_json_repacked),
    (True, False, rtb_links['AN_ADFOX'], 'application/json', rtb_json_repacked),
    # если запрос без квери-арга callback, то он должен обработаться как XHR вне зависимости от настройки request_via_script в конфиге
    (True, True, rtb_links['BK_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    (True, True, rtb_links['ADFOX_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    (True, True, rtb_links['AN_ADFOX_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    (True, False, rtb_links['BK_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    (True, False, rtb_links['ADFOX_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    (True, False, rtb_links['AN_ADFOX_WITHOUT_CALLBACK'], 'application/json', rtb_json_repacked),
    # https://st.yandex-team.ru/ANTIADB-1888 yandex_mail server side requests
    (True, False, rtb_links['BK_SERVERSIDE'], 'application/javascript', "('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (False, False, rtb_links['BK_SERVERSIDE'], 'application/javascript', "('{}')".format(rtb_json_repacked.replace("'", "\\'"))),
    (True, False, rtb_links['BK_SERVERSIDE'] + '&callback=json', 'application/json', rtb_json_repacked),
    (False, True, rtb_links['BK_SERVERSIDE'] + '&callback=json', 'application/json', rtb_json_repacked),
])
@pytest.mark.parametrize('updated_content_type', ['', 'text/x-json'])
def test_rtb_auction_via_script_processing(stub_server,
                                           set_handler_with_config,
                                           get_key_and_binurlprefix_from_config,
                                           get_config, rtb_response_is_json,
                                           request_via_script,
                                           rtb_request,
                                           expected_type,
                                           expected_answer,
                                           updated_content_type):

    def handler(**_):
        if rtb_response_is_json:
            return {'text': rtb_json, 'code': 200, 'headers': {'Content-Type': 'application/json'}}
        else:
            return {'text': rtb_answer, 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script
    if updated_content_type:
        expected_type = updated_content_type
        update_dict = {'Content-Type': updated_content_type}
        new_test_config['UPDATE_RESPONSE_HEADERS_VALUES'] = {
            'an\.yandex\.ru/.*?': update_dict,
            'ads\.adfox\.ru/.*?': update_dict,
        }
    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    crypted_url = crypt_url(binurlprefix, rtb_request, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         SEED_HEADER_NAME: seed,
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == expected_type
    assert_that(proxied_data.text, equal_to(expected_answer))


expected_answer_jsonp = "Ya[1524124928839]('{}')".format(rtb_json_repacked.replace("'", "\\'"))
expected_answer_jsonp_quote = "Ya[\"1524124928839\"]('{}')".format(rtb_json_repacked.replace("'", "\\'"))
rtb_answer_quote = "Ya [\"1524124928839\"]('{}')".format(rtb_json.replace("'", "\\'"))
callback_value = "Ya%5B1524124928839%5D"
callback_value_quote = "Ya%5B%221524124928839%22%5D"


@pytest.mark.parametrize('request_via_script, expected_type, rtb_content, expected_content, callback', [
    (False, 'application/javascript', rtb_answer, expected_answer_jsonp, callback_value),
    (True, 'application/javascript', rtb_answer, expected_answer_jsonp, callback_value),
    (False, 'application/javascript', rtb_answer_quote, expected_answer_jsonp_quote, callback_value_quote),
    (True, 'application/javascript', rtb_answer_quote, expected_answer_jsonp_quote, callback_value_quote),
])
def test_rtb_page_with_jsonp_arg(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config,
                                 request_via_script, expected_type, rtb_content, expected_content, callback):

    def handler(**_):
        return {'text': rtb_content, 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script
    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    url = 'http://bs.yandex.ru/page/rtb_auction_sample?test=woah&callback={}&jsonp=1'.format(callback)
    crypted_url = crypt_url(binurlprefix, url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         SEED_HEADER_NAME: seed,
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == expected_type
    assert_that(proxied_data.text, equal_to(expected_content))


@pytest.mark.parametrize('request_via_script, expected_type', [
    (False, 'application/json'),
    (True, 'application/javascript'),
])
def test_native_design_rtb_meta_repack(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config,
                                       request_via_script, expected_type):
    native_design_rtb_json = {
        'seatbid': [],
        'settings': {
            '2021': {
                'ssr': {},
                'name': 'nativeDesign',
                'template': '<div class="wrapper-media">\n<ya-units-grid cols="5" rows="1">\n</ya-units-grid>\n</div>',
                'css': '.wrapper-media {\n\tfont-family: YS Text,Helvetica Neue,Arial,sans-serif;\n\t-webkit-font-feature-settings: "liga","kern";\n}'
            }
        }
    }

    def handler(**_):
        return {'text': json.dumps(native_design_rtb_json), 'code': 200, 'headers': {'Content-Type': 'application/json'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script
    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, rtb_links['BK'], key, True, origin='test.local')

    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         SEED_HEADER_NAME: seed,
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == expected_type
    if not request_via_script:
        assert_that(json.loads(proxied_data.text), equal_to(native_design_rtb_json))
    else:
        expected_answer = r'{"seatbid": [], "settings": {"2021": {"ssr": {}, "name": "nativeDesign", ' \
                          r'"css": ".wrapper-media {\\n\\tfont-family: YS Text,Helvetica Neue,Arial,sans-serif;\\n\\t-webkit-font-feature-settings: \\\"liga\\\",\\\"kern\\\";\\n}", ' \
                          r'"template": "<div class=\\\"wrapper-media\\\">\\n<ya-units-grid cols=\\\"5\\\" rows=\\\"1\\\">\\n</ya-units-grid>\\n</div>"}}}'
        assert_that(proxied_data.text, equal_to("Ya[1524124928839]('{}')".format(expected_answer)))


@pytest.mark.parametrize('adfox_base_url,adfox_handler,adfox_type,repacked',
                         [('http://ads.adfox.ru', 'getCode', None,  False),
                          ('http://ads.adfox.ru', 'getBulk', 'direct', True),
                          ('http://ads.adfox.ru', 'getBulk', 'direct_not_base64', True),
                          ('http://ads.adfox.ru', 'getBulk', 'direct_ssr', True),
                          ('http://ads.adfox.ru', 'getBulk', 'image', False),
                          ('http://an.yandex.ru/adfox', 'getBulk', 'direct', True),
                          ('http://an.yandex.ru/adfox', 'getBulk', 'direct_not_base64', True),
                          ('http://an.yandex.ru/adfox', 'getBulk', 'direct_ssr', True),
                          ('http://an.yandex.ru/adfox', 'getBulk', 'image', False)])
def test_adfox_rtb_meta_repack(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config,
                               adfox_base_url, adfox_handler, adfox_type, repacked):
    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)

    url_path = '/12345/{}/html_repack{}.js?woah=bing'.format(adfox_handler, '_{}'.format(adfox_type) if adfox_type is not None else '')
    original_url = adfox_base_url + url_path
    stub_url = urljoin(stub_server.url, url_path)
    crypted_url = crypt_url(binurlprefix, original_url, key, True, origin='test.local')

    original_data = requests.get(stub_url)
    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    if repacked:
        binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
        base_path = test_config.html5_base_path_re.search(html_decoded)
        expected_html_data = body_crypt(body=html_decoded,
                                        binurlprefix=binurlprefix_for_bodycrypt,
                                        key=key,
                                        crypt_url_re=test_config.crypt_html5_relative_url_re,
                                        enable_trailing_slash=False,
                                        file_url=base_path.group(1),
                                        min_length=test_config.RAW_URL_MIN_LENGTH,
                                        crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE)
        expected_html_data = body_crypt(expected_html_data,
                                        binurlprefix=binurlprefix_for_bodycrypt,
                                        key=key,
                                        crypt_url_re=test_config.crypt_url_re_with_counts,
                                        enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                                        min_length=test_config.RAW_URL_MIN_LENGTH,
                                        crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                                        partner_url_re_match=test_config.partner_url_re)
        proxied_data = proxied_data.json()
        if adfox_type == "direct_ssr":
            decoded_rtb_json = proxied_data['data'][0]['attributes']['data']
            proxied_banner = urlsafe_b64decode(decoded_rtb_json['direct']['ssr']['html'].encode('utf-8'))
        else:
            if adfox_type == "direct_not_base64":
                decoded_rtb_json = proxied_data['data'][0]['attributes']['data']
            else:
                decoded_rtb_json = json.loads(urlsafe_b64decode(proxied_data['data'][0]['attributes']['dataBase64'].encode('utf-8')))
            proxied_banner = urlsafe_b64decode(decoded_rtb_json['rtb']['html'].encode('utf-8'))
            assert 'url' not in decoded_rtb_json['rtb'].keys()
        assert_that(proxied_banner, equal_to(expected_html_data))
    else:
        # Вообще, в реальности ответы совпадать не будут, так как в них почти всегда есть ссылки, которые мы пошифруем
        # Но в тестах все упрощено и ссылок в ответах заглушки нет.
        assert_that(proxied_data.content, equal_to(original_data.content))


@pytest.mark.parametrize('adfox_base_url', ['http://ads.adfox.ru', 'http://an.yandex.ru/adfox'])
def test_adfox_rtb_meta_repack_without_html(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, adfox_base_url):
    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    crypted_url = crypt_url(binurlprefix, adfox_base_url +'/12345/getBulk/html_repack_without_html.js?woah=bing', key, True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    binurlprefix_for_bodycrypt = CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX)
    expected_js = body_crypt(bk_without_html_rtb_auction_response_js,
                             binurlprefix=binurlprefix_for_bodycrypt,
                             key=key,
                             crypt_url_re=test_config.crypt_url_re_with_counts,
                             enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                             min_length=test_config.RAW_URL_MIN_LENGTH,
                             crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                             partner_url_re_match=test_config.partner_url_re)
    proxied_data = proxied_data.json()
    assert_that(urlsafe_b64decode(proxied_data['data'][0]['attributes']['dataBase64'].encode('utf-8')), equal_to(expected_js))


@pytest.mark.parametrize('adfox_base_url', ['http://ads.adfox.ru', 'http://an.yandex.ru/adfox'])
def test_adfox_rtb_meta_repack_without_rtb_field(stub_server, get_key_and_binurlprefix_from_config, get_config,
                                                 set_handler_with_config, adfox_base_url):
    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    crypted_url = crypt_url(binurlprefix,
                            adfox_base_url + '/12345/getBulk/html_repack_without_rtb.js?woah=bing', key,
                            True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    expected_data = body_crypt(json.dumps(bk_rtb_auction_response_without_rtb_json),
                               binurlprefix=CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX),
                               key=key,
                               crypt_url_re=test_config.crypt_url_re_with_counts,
                               enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                               min_length=test_config.RAW_URL_MIN_LENGTH,
                               crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                               partner_url_re_match=test_config.partner_url_re)
    assert_that(urlsafe_b64decode(proxied_data.json()['data'][0]['attributes']['dataBase64'].encode('utf-8')),
                equal_to(expected_data))


@pytest.mark.parametrize('adfox_base_url', ['http://ads.adfox.ru', 'http://an.yandex.ru/adfox'])
@pytest.mark.parametrize('type_response', ['base64', 'encodeURI'])
def test_adfox_rtb_meta_repack_html(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, adfox_base_url, type_response):
    stub_server.set_handler(handler)

    partner_name = "test_local"
    test_config = get_config(partner_name)
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = False

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(partner_name)
    if type_response == 'base64':
        path = '/12345/getBulk/html_repack_html_base64.js?woah=bing'
        data_key = 'htmlBase64'
        decode_func = urlsafe_b64decode
    else:
        path = '/12345/getBulk/html_repack_html_encoded.js?woah=bing'
        data_key = 'htmlEncoded'
        decode_func = unquote

    crypted_url = crypt_url(binurlprefix,
                            adfox_base_url + path, key,
                            True, origin='test.local')

    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    expected_data = body_replace(body=adfox_banner_html,
                                 key=key,
                                 replace_dict=test_config.replace_body_with_tags_re,
                                 crypt_in_lowercase=test_config.CRYPT_IN_LOWERCASE)

    expected_data = body_crypt(body=expected_data,
                               binurlprefix=CryptUrlPrefix('http', 'test.local', seed, test_config.CRYPT_URL_PREFFIX),
                               key=key,
                               crypt_url_re=test_config.crypt_url_re_with_counts,
                               enable_trailing_slash=test_config.CRYPT_ENABLE_TRAILING_SLASH,
                               min_length=test_config.RAW_URL_MIN_LENGTH,
                               crypted_url_mixing_template=test_config.CRYPTED_URL_MIXING_TEMPLATE,
                               partner_url_re_match=test_config.partner_url_re)
    assert_that(decode_func(proxied_data.json()['data'][0]['attributes'][data_key].encode('utf-8')),
                equal_to(expected_data))


@pytest.mark.parametrize('rtb_response_code', [200, 403, 404, 500])
def test_rtb_auction_process_with_various_response_codes(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, rtb_response_code):
        """
        Проверяем, что обработка ответа от РТБ хоста происходит только при коде ответа 200
        204 в тесте не проверяем, так как тестовая заглушка в случае 204 отдает ответ без тела и заголовка Content-Type :(
        """
        rtb_json = '{"data": [], "rtb": {"html": "", "abuseLink": ""}}'

        def handler(**_):
            return {'text': rtb_json, 'code': rtb_response_code, 'headers': {'Content-Type': 'application/json'}}

        stub_server.set_handler(handler)

        test_config = get_config('test_local')
        new_test_config = test_config.to_dict()
        new_test_config['RTB_AUCTION_VIA_SCRIPT'] = True
        set_handler_with_config(test_config.name, new_test_config)

        seed = 'my2007'
        key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

        rtb_links = ['http://an.yandex.ru/meta/rtb_auction_sample?test=woah&callback=Ya%5B1524124928839%5D',
                     'http://ads.adfox.ru/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=&callback=Ya%5B1524124928839%5D',
                     'http://an.yandex.ru/adfox/242790/getBulk/v2?p1=bxymx&p2=fged&puid1=&puid2=&puid3=&puid4=&callback=Ya%5B1524124928839%5D']

        for rtb_link in rtb_links:
            crypted_url = crypt_url(binurlprefix, rtb_link, key, True, origin='test.local')

            proxied_data = requests.get(crypted_url,
                                        headers={'host': 'test.local',
                                                 SEED_HEADER_NAME: seed,
                                                 PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

            assert proxied_data.status_code == rtb_response_code
            if rtb_response_code != 200:
                assert proxied_data.headers['Content-Type'] == 'application/json'
                assert_that(proxied_data.text, equal_to(rtb_json))
            else:
                assert proxied_data.headers['Content-Type'] == 'application/javascript'
                assert_that(proxied_data.text, equal_to('Ya[1524124928839](\'{}\')'.format(rtb_json)))


@pytest.mark.parametrize('request_via_script', [True, False])
@pytest.mark.parametrize('adfox_base_url', ['http://ads.adfox.ru', 'http://an.yandex.ru/adfox'])
def test_adfox_errors_response(request_via_script, stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, adfox_base_url):
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script
    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    crypted_url = crypt_url(binurlprefix, adfox_base_url + '/12345/getBulk/errors?callback=Ya%5B1524124928839%5D', key, True, origin='test.local')
    proxied_response = requests.get(
        crypted_url,
        headers={'host': 'test.local', SEED_HEADER_NAME: seed, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    )
    if request_via_script:
        assert_that(proxied_response.text.startswith('//'))
        assert_that(json.loads(proxied_response.text[2:]), equal_to(adfox_errors_response))
        assert proxied_response.headers['Content-Type'] == 'application/javascript'
    else:
        assert proxied_response.headers['Content-Type'] == 'application/json; charset=utf-8'
        assert_that(proxied_response.json(), equal_to(adfox_errors_response))


def test_bk_rtb_meta_repack_with_invalid_json(stub_server, get_key_and_binurlprefix_from_config, get_config):
    rtb_json = """{"rtb": {"html": "", "abuseLink": ""}, "common": {"isYandex": "1", "canRetry": 0, "isYandexPage": "0", "reloadTimeout": "10"}}"""
    rtb_answer = "json('{}')".format(rtb_json)

    def handler(**_):
            return {'text': rtb_answer, 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    url = 'http://an.yandex.ru/meta/rtb_auction_sample?test=woah&callback=Ya%5B1524124928839%5D'

    crypted_url = crypt_url(binurlprefix, url, key, True, origin='test.local')

    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == 'application/json'
    assert_that(json.loads(proxied_data.text), equal_to(json.loads(rtb_json)))


@pytest.mark.parametrize('response_content, expected_source, server_side', (
        (rambler_response_server_side, crypted_rambler_banner_html, True),
        (rambler_response_client_side, "//test.local/SNXt12154/my20071_/mAYu7idBAmI/p4eUHqJ/h6K--5/UCHaP/p2cVlrR/ii6uNgD34t/eTKK8/qH3lAzzT/F_VXN9W/TcX9v6fX/H6s/eeBZwHaeZu0/bOmwt/KMCKaHk50h4SihNY/", False),
))
@pytest.mark.parametrize('content_type', ['application/x-javascript', 'application/x-shared-scripts'])
def test_rambler_rtb_repack(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, response_content, expected_source, server_side, content_type):

    def handler(**_):
            return {'text': response_content, 'code': 200, 'headers': {'Content-Type': '{}; charset=windows-1251'.format(content_type)}}

    stub_server.set_handler(handler)

    callback = 'Begun_Autocontext_saveFeed0'

    url = 'http://ssp.rambler.ru/context.jsp?test=woah&callback={}'.format(callback)
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['AD_SYSTEMS'] = [1, 2, 3, 4]
    new_test_config['CRYPT_BODY_RE'] = [r'\bYa\b', r'\bAdvManager\b']
    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, url, key, True, origin='test.local')
    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert proxied_data.status_code == 200

    json_data = json.loads(proxied_data.text[len(callback) + 1:-1])
    if server_side:
        _, source = json_data['banners']['graph'][0]['source'].split(',', 1)
        source = b64decode(source.encode('utf-8'))
    else:
        source = json_data['banners']['graph'][0]['source']

    assert_that(source, equal_to(expected_source))


@pytest.mark.parametrize('method', ('get', 'post'))
@pytest.mark.parametrize('host', ('mobile.yandexadexchange.net', 'adsdk.yandex.ru'))
def test_nanpu_repack(stub_server, cryprox_worker_url, get_config, method, host):
    request_method = getattr(requests, method)
    content = {
        'clickUrl': '//adsdk.yandex.ru:443/proto/report/an.yandex.ru/count/WP0ejI',
        'clickUrl2': '//an.yandex.ru/count/WP0ejI',
        'showNotices': ['//adsdk.yandex.ru:443/proto/report/an.yandex.ru/rtbcount/WP0ejI'],
        'abuse': '//adsdk.yandex.ru:443/proto/report/an.yandex.ru/abuse/WP0ejI',
        'abuse2': '//an.yandex.ru/abuse/WP0ejI',
        'ad': '//mobile.yandexadexchange.net:80/v4/ad',
        'ad2': '//adsdk.yandex.ru:80/v4/ad',
        'img': '//avatars.mds.yandex.net/get-direct-picture/1674598/orig',
        'script': '//yastatic.net/pcode/media/loader.js',
        'loader': '//yandex.ru/ads/system/context.js',
        'text': 'Ya.Context.AdvManager.render()',
    }
    expected = {
        'clickUrl': '//test.local/SNXt12263/my20071_/mIa_q2LhAsM/IBUR2PK/l7vhvY/9DR_z/9391vrA/C06rNnCT1x/dGW8_/5zEtECKe/R-fdfgV/cuaVjISE/II4/IeAhwHrqJpl/3AlQx/wITSdR/Ax3i4avt9bk/',
        'clickUrl2': '//test.local/XWtN7S467/my20071_/mIYaerJFAxN/JYeUG7L/hqGu58/9fP9y/_1cNJ3C/2Q255KMkBZ/XQKL6/pfTpRbIZAnResg_/',
        'showNotices': ['//test.local/SNXt12699/my20071_/mIa_q2LhAsM/IBUR2PK/l7vhvY/9DR_z/9391vrA/C06rNnCT1x/dGW8_/5zEtECKe/R-fZOMC/f_3PtaCb/ErR/xQj14AKeXqE/3WnRd/3EhigR/EdokMuitOraHDfT/'],
        'abuse': '//test.local/SNXt12263/my20071_/mIa_q2LhAsM/IBUR2PK/l7vhvY/9DR_z/9391vrA/C06rNnCT1x/dGW8_/5zEtECKe/R-fd_UV/b_eVjISE/II4/IeAhwHrqJpl/3AlQx/wITSdR/Ax3i4avt9bk/',
        'abuse2': '//test.local/XWtN7S467/my20071_/mIYaerJFAxN/JYeUG7L/hKyu-t/5fP9y/_1cNJ3C/2Q255KMkBZ/XQKL6/pfTpRbIZAnResg_/',
        'ad': '//mobile.yandexadexchange.net:80/v4/ad',
        'ad2': '//adsdk.yandex.ru:80/v4/ad',
        'img': '//test.local/SNXt11718/my20071_/mIeeimJEwmf/4NUUTWd/hKC_7M/NeBun/7n85l91/-1865wHmY9/aiKm6/ofStBeVP/V2EI65Y/M_3IsrPr/GqU/AZQh-DbGRoE/H9tzZ/Kez2BU0N3u7o/',
        'script': '//test.local/XWtN8S775/my20071_/mQbvqmJEo8M/sBeR2_L/la207d/5fBen/r2cgv7x/2w_rlnU3hj/RRSE3/7D_nmrtT/CP-YvIT/aLzWtLfVKbse/',
        'loader': '//test.local/XWtN8S993/my20071_/mQbue2IEZ7I/5sfQ3-X/yr2i-s/8VBaP/s38d05g/qlxb1xHzx6/aRSa3/7Pijnf2Q/i35WOMF/b-aUt7vXJIgeeA/',
        'text': 'Ya.ou7mHeayqMQ.AdvManager.render()',
    }
    content = b64encode(json.dumps(content))

    def handler(**_):
        return {'text': content, 'code': 200}

    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    seed = 'my2007'
    headers = {
        'host': 'test.local',
        PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
        SEED_HEADER_NAME: seed,
        FETCH_URL_HEADER_NAME: 'http://{host}:80/v4/ad'.format(host=host),
    }
    proxied_data = request_method(urljoin(cryprox_worker_url, '/appcry'), headers=headers)
    assert proxied_data.status_code == 200
    proxied = json.loads(b64decode(proxied_data.text.encode('utf-8')))
    assert_that(proxied, equal_to(expected))
