# -*- coding: utf8 -*-
import json
from urlparse import urljoin, urlparse

import re2
import pytest
import requests
from bs4 import BeautifulSoup
from hamcrest import assert_that, equal_to, any_of

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix
from antiadblock.cryprox.cryprox.common.cry import encrypt_xor, generate_seed
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, COMMON_TAGS_REPLACE_LIST,\
    INSERT_STYLE_TEMPLATE, DETECT_LIB_HOST, EncryptionSteps, IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT, \
    AUTOREDIRECT_SERVICE_ID, AUTOREDIRECT_DETECT_LIB_PATH, NGINX_SERVICE_ID_HEADER
from antiadblock.cryprox.cryprox.config.ad_systems import AdSystem
from antiadblock.cryprox.cryprox.config.service import COOKIELESS_PATH_PREFIX
from antiadblock.cryprox.cryprox.common.tools.misc import ConfigName
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST, AUTO_RU_HOST, DEFAULT_COOKIELESS_HOST


def test_generic_rule_evade(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    """
    Проверяет шифрование с обходом общих правил для урлов

    схема алгоритма подбора параметров:

        def gen_case():
            return ['-AD2_']
        s1 = string.ascii_letters +  string.digits + '@$&*?/,.[]'
        BASE64_START_BYTE_MATCH = 21
        while True:
         seed1 = os.urandom(6).translate(TABLE_FOR_RANDOM_BASE64)
         key = hmac.new("duoYujaikieng9airah4Aexai4yek4qu", seed1, hashlib.sha512).digest()[BASE64_START_BYTE_MATCH:]
         for result in gen_case():
          for prefix in prefixes:
           for prefix_len in xrange(0,4):
            result_augmented = prefix * prefix_len + result + '1' # + '1' for extra few bits in 6-bit group
            try:
                result_debase64 = base64.urlsafe_b64decode(result_augmented + b"=" * ((4 - len(result_augmented)) % 4))
            except TypeError as te:
                continue
            all_ok = True
            cyphered_string = ''
            for cp, kp in zip(result_debase64, key):
                cyphered = chr(ord(cp) ^ ord(kp))
                if cyphered not in s1:
                    all_ok = False
                    break
                cyphered_string += cyphered
            if all_ok:
                print 'ok', cyphered_string, seed1
                break
           if all_ok: break
          if all_ok: break
         if all_ok: break



    GOk&V

    проверка:

        base64.urlsafe_b64encode(crypto_xor("http://yastatic.net//GOk&V.css/%20", hmac.new("duoYujaikieng9airah4Aexai4yek4qu", seed1, hashlib.sha512).digest()))

    JTMzvmgDqvkIQNEHCIdeYAYgjNIu-AD2_A1Ax1ApYTd55Q==


    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPTED_URL_MIXING_TEMPLATE'] = (('/', 1),)

    set_handler_with_config(test_config.name, new_test_config)

    def handler(**_):
        return {
            'text': '<!DOCTYPE html><html><body><h1><img '
            'href="http://yastatic.net//GOk&V.css/%20"></h1></body></html>',
            'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)
    seed = '8pzm0y'

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config, seed=seed)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/', key, False, origin='test.local')
    proxied_html = requests.get(crypted_link, headers={"host": 'test.local', SEED_HEADER_NAME: seed, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert '-$AD2_' in proxied_html


def soup_tags_attrs(soup, tag, attr):
    """
    Find all tags with current attrs and return attrs values for that tags
    :param soup: html layout returnd by BeautifulSoup
    :param tag: html tag such as img, script, div, etc
    :param attr: tag attr to search - src, type, class
    :return: list with attrs `attr` for current tag within `soup`
    """
    return [elem.get(attr) for elem in soup.findAll(tag) if elem.get(attr) is not None]


def get_soup_elems_by_attr_value(soup, tag, attr_name, attr_value):
    """
    Find all html elems with current tag and attribute value
    :param soup: html layout returnd by BeautifulSoup
    :param tag: html tag such as img, script, div, etc
    :param attr_name: tag attr to search - src, type, class
    :param attr_value: tag attr value
    :return: list with BeautifulSoup.Tag elems
    """
    return [elem for elem in soup.findAll(tag) if elem.get(attr_name) == attr_value]


@pytest.mark.parametrize('method', ['get', 'post'])
def test_layout_crypt(stub_server, cryprox_worker_url, get_config, method):
    """
    Basic layout crypting test
    Check crypting img, scripts and css links and some class replaces
    """

    request_method = getattr(requests, method)
    crypted_index = request_method(cryprox_worker_url,
                                   headers={'host': AUTO_RU_HOST,
                                            SEED_HEADER_NAME: 'my2007',
                                            PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]}).text
    original_index = request_method(urljoin(stub_server.url, '/index.html')).text

    crypted_soup = BeautifulSoup(crypted_index, "lxml")
    original_soup = BeautifulSoup(original_index, "lxml")

    # Check that count of current layout elements didnt change
    for item in ['p', 'div', 'img', 'script', 'link']:
        assert len(crypted_soup.findAll(item)) == len(original_soup.findAll(item))

    # Check js scripts links crypting
    original_scripts = soup_tags_attrs(original_soup, 'script', 'src')
    crypted_scripts = soup_tags_attrs(crypted_soup, 'script', 'src')
    for script in crypted_scripts:
        assert script.find(TEST_LOCAL_HOST) and script not in original_scripts

    # Check detect script crypting
    src_pattern = 'src: "(https?://[\w\-\/\?\.\=]+)"'
    original_detect_script = get_soup_elems_by_attr_value(original_soup, 'script', 'type', 'detect')[0]
    crypted_detect_script = get_soup_elems_by_attr_value(crypted_soup, 'script', 'type', 'detect')[0]
    original_detect_src = re2.search(src_pattern, original_detect_script.text).groups()[0]
    crypted_detect_src = re2.search(src_pattern, crypted_detect_script.text).groups()[0]
    assert original_detect_src != crypted_detect_src
    assert crypted_detect_src.startswith('http://auto.ru/')

    # Check img links crypting
    original_images = soup_tags_attrs(original_soup, 'img', 'src')
    crypted_images = soup_tags_attrs(crypted_soup, 'img', 'src')
    for image in original_images:
        if image.find('http://avatars.mds.yandex.net/get-auto/') >= 0:
            assert image not in crypted_images
        else:
            assert image in crypted_images
    # Check that all crypted imgs starts with partner domain
    for crypted_image in crypted_images:
        if crypted_image not in original_images:
            assert crypted_image.startswith('http://auto.ru/')

    # Check css stylesheets links crypting
    original_css = soup_tags_attrs(original_soup, 'link', 'href')
    crypted_css = soup_tags_attrs(crypted_soup, 'link', 'href')
    for css in crypted_css:
        assert css not in original_css

    # Check div class crypting
    crypted_div_classes = crypted_soup.find('div', 'info')['class']
    assert len(crypted_div_classes) == 2
    assert 'info' in crypted_div_classes and 'gwd-ad' not in crypted_div_classes


@pytest.mark.parametrize('method', ['get', 'post'])
def test_json_crypt(stub_server, cryprox_worker_url, get_config, method):
    request_method = getattr(requests, method)
    pattern_index = request_method(urljoin(stub_server.url, '/crypted/json.html')).text
    crypted_index = request_method(urljoin(cryprox_worker_url, '/json.html'),
                                   headers={"host": AUTO_RU_HOST,
                                            SEED_HEADER_NAME: 'my2007',
                                            PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]}).text
    assert pattern_index == crypted_index


@pytest.mark.parametrize('test_file', ['/relative_urls/test.{}'.format(ext) for ext in ['css', 'html', 'js']])
def test_crypt_relative_urls(stub_server, test_file, cryprox_worker_url, get_config):
    response = requests.get(urljoin(cryprox_worker_url, test_file),
                            headers={"host": TEST_LOCAL_HOST,
                                     SEED_HEADER_NAME: 'my2007',
                                     PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]}).text
    expected_response = requests.get(urljoin(stub_server.url, '/crypted' + test_file)).text
    assert expected_response == response


def test_crypt_rambler_relative_urls(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['AD_SYSTEMS'] = [AdSystem.RAMBLER_SSP]
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    test_file = '/relative_urls/file.js'
    response = requests.get(crypt_url(binurlprefix, urljoin('http://img01.ssp.rambler.ru/', test_file), key, True, origin='test.local'),
                            headers={"host": TEST_LOCAL_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    expected_response = requests.get(urljoin(stub_server.url, '/crypted' + test_file)).text
    assert expected_response == response


def test_crypt_storage_mds_relative_urls(stub_server, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    text = '''manifest: [
    {src:"300x300_canvas_atlas_P_.png"},
    {src:"300x300_canvas_atlas_NP_.jpg"}]'''

    crypted_text = '''manifest: [
    {src:"http://test.local/SNXt13789/my2007kK/Kdf7P9ak0hP/pxRRX7K/iKqop8/IRBuj/qyIdu5g/b-_blhUHFx/dD2k7/d_IpVXIP/kWDJqcY/L6KKhLfV/K5I/gVAhQK5S3ml/DZjRp/hFBCsb/21JraKHlf/3eAxyi4/9_KYe8tjg.png"},
    {src:"http://test.local/SNXt13898/my2007kK/Kdf7P9ak0hP/pxRRX7K/iKqop8/IRBuj/qyIdu5g/b-_blhUHFx/dD2k7/d_IpVXIP/kWDJqcY/L6KKhLfV/K5I/gVAhQK5S3ml/DHghp/hChCvc/n1UtqyJks/fPFRv4o/dzGY-IejsU.jpg"}]'''

    def handler(**_):
        return {'text': text, 'code': 200}

    stub_server.set_handler(handler)

    response_text = requests.get(crypt_url(binurlprefix, 'http://storage.mds.yandex.net/get-canvas-html5/canvas.js', key, True, origin='test.local'),
                                 headers={"host": TEST_LOCAL_HOST,
                                          PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert crypted_text == response_text


@pytest.mark.parametrize('file', ['test.{}'.format(ext) for ext in ['css', 'html', 'js']])
def test_crypt_relative_urls_global_regex(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config, file):
    """
    Тест что по EncryptionStep=crypt_relative_urls_automatically шифруются относительные урлы в файлах партнера автоматически.
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.crypt_relative_urls_automatically.value)

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    response = requests.get(crypt_url(binurlprefix, urljoin('http://test.local', '/relative_urls/' + file), key, False, origin='test.local'),
                            headers={"host": TEST_LOCAL_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    expected_response = requests.get(urljoin(stub_server.url, '/crypted/relative_urls/partners/' + file)).text
    assert expected_response == response


def test_crypt_context(stub_server, cryprox_worker_url, get_config):
    """
    Test crypting `\bContext\b` cases
    """
    expected_content = requests.get(urljoin(stub_server.url, '/crypted/ya.context.js')).text
    crypted_content = requests.get(urljoin(cryprox_worker_url, '/ya.context.js'),
                                   headers={"host": AUTO_RU_HOST,
                                            SEED_HEADER_NAME: 'my2007',
                                            PARTNER_TOKEN_HEADER_NAME: get_config('autoru').PARTNER_TOKENS[0]}).text
    assert expected_content == crypted_content


def test_crypt_schemeless_urls(stub_server, cryprox_worker_url, get_config):
    expected_content = requests.get(urljoin(stub_server.url, '/crypted/schemeless_urls.html')).text
    crypted_content = requests.get(urljoin(cryprox_worker_url, '/schemeless_urls.html'),
                                   headers={"host": TEST_LOCAL_HOST,
                                            SEED_HEADER_NAME: 'my2007',
                                            PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]}).text
    assert expected_content == crypted_content


def test_crypt_html5_relative_urls(stub_server, get_key_and_binurlprefix_from_config, get_config):
    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    test_file = '/html5/banner.html'
    response = requests.get(crypt_url(binurlprefix, urljoin('http://yandexadexchange.net', test_file), key, True, origin='test.local'),
                            headers={"host": TEST_LOCAL_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    expected_response = requests.get(urljoin(stub_server.url, '/crypted' + test_file)).text
    assert expected_response == response


@pytest.mark.parametrize('test_file_extension', ['html', 'css', 'js'])
def test_crypt_html_tags(cryprox_worker_url, set_handler_with_config, test_file_extension, stub_server, get_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPTION_STEPS'] = [EncryptionSteps.tags_crypt.value, EncryptionSteps.advertising_crypt.value]

    set_handler_with_config(test_config.name, new_test_config)

    crypted_response = requests.get(urljoin(cryprox_worker_url, '/tags/' + 'tags.{}'.format(test_file_extension)),
                                    headers={"host": TEST_LOCAL_HOST,
                                             PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                                    ).text
    key = get_key(test_config.CRYPT_SECRET_KEY, generate_seed())

    response = requests.get(urljoin(stub_server.url, '/tags/' + 'tags_template.{}'.format(test_file_extension))).text
    random_letter = chr(ord(key[0]) % 26 + ord('a'))
    crypted_tag = encrypt_xor(COMMON_TAGS_REPLACE_LIST[0], key)
    crypted_tag = random_letter + '-' + crypted_tag.replace('_', random_letter)
    crypted_tag = crypted_tag.upper()
    expected_response = response.format(tag=crypted_tag, style=INSERT_STYLE_TEMPLATE.format(crypted_tag)) \
        if test_file_extension == 'html' else response.replace('{tag}', crypted_tag)
    assert not crypted_tag[0].isdigit()
    assert crypted_response == expected_response


def test_static_mon_crypt_tags(get_config, set_handler_with_config, get_key_and_binurlprefix_from_config, stub_server):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    new_test_config['ENCRYPTION_STEPS'] = [EncryptionSteps.tags_crypt.value, EncryptionSteps.advertising_crypt.value]

    set_handler_with_config(test_config.name, new_test_config)
    response = requests.get(crypt_url(binurlprefix, urljoin('http://{}/'.format(DETECT_LIB_HOST), 'tags/static_mon_tags.html'), key, True, origin='test.local'),
                            headers={"host": TEST_LOCAL_HOST,
                                     PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    expected_response = requests.get(urljoin(stub_server.url, 'tags/static_mon_tags.html')).text.replace('.div', '["div"]')
    assert expected_response == response


@pytest.mark.parametrize('test_file', ['self-closed_tags.html', 'crypt_tags_case_sensitive.html'])
def test_close_and_case_insensitive_html_tags(cryprox_worker_url, set_handler_with_config, stub_server, get_config, test_file):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPTION_STEPS'] = [EncryptionSteps.tags_crypt.value, EncryptionSteps.advertising_crypt.value]

    set_handler_with_config(test_config.name, new_test_config)

    crypted_response = requests.get(urljoin(cryprox_worker_url, '/tags/' + test_file),
                                    headers={"host": TEST_LOCAL_HOST,
                                             PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]},
                                    ).text

    key = get_key(test_config.CRYPT_SECRET_KEY, generate_seed())

    response = requests.get(urljoin(stub_server.url, '/crypted/tags/' + test_file)).text
    random_letter = chr(ord(key[0]) % 26 + ord('a'))
    crypted_tag = encrypt_xor(COMMON_TAGS_REPLACE_LIST[0], key)
    crypted_tag = random_letter + '-' + crypted_tag.replace('_', random_letter)
    crypted_tag = crypted_tag.upper()
    expected_response = response.format(tag=crypted_tag,
                                        style=INSERT_STYLE_TEMPLATE.format(crypted_tag))

    assert crypted_response == expected_response


json_to_test_crypt_probability = {
    "relative": [
        "/scripts/function/get_comment_ext.png",
        "/scripts/picture.jpeg",
    ] * 200,
    "partner_images": [
        "http://test.local/images/image.png",
        "http://avatars.mds.yandex.net/get-autoru/880749/4b107a3b91144a54bc25037ca76728fd/thumb_m",
        "//another.domain/gifs/some_gif.gif",
        "https://another.domain/gifs/pic.jpeg?size=410x680",
        "https://im0-tub-ru.yandex.net/i?id=e16fb03b11df4787623c0df080acefbe&n=13",
    ] * 200,
    "other_images": [
        "https://im-tub-ru.yandex.net/i?id=e16fb03b11df4787623c0df080acefbe&n=13",
        "https://im0-tub-ru.yandex.ru/i?id=e16fb03b11df4787623c0df080acefbe&n=13",
        "http://avatars.mds.yandex.net/test",
    ],
    "partner_other": [
        "http://test.local/images/cssss.css",
        "http://test.local/images/res.js",
    ],
    "bk": [
        "http://avatars.mds.yandex.net/get-direct/135341/DRPqm0sH4Jdr6AtpnjJ8MQ/y180",
        "http://avatars.mds.yandex.net/get-canvas/35835321.jpeg",
        "http://avatars.mds.yandex.net/get-direct/35835321.jpeg",
        "//yastatic.net/static/jquery-1.11.0.min.js",
    ],
}


@pytest.mark.parametrize('image_crypting_ratio, expexted_ratio', [
    (0., 0.),
    (0.2, 0.2),
    (0.5, 0.496),
    (1., 1.),
])
def test_partner_images_random_crypting(stub_server, image_crypting_ratio, expexted_ratio, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    """
    Проверяет что доля зашифрованных партнерских ссылок на спроксированной странице примерно совпадает с указанной
    в конфиге вероятностью шифрования IMAGE_URLS_CRYPTING_PROBABILITY.
    :param image_crypting_ratio: желаемая доля зашифрованных партнерских ссылок на картинки
    :param expexted_ratio: точное значение в спроксированной странице зависит от сида
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['IMAGE_URLS_CRYPTING_PROBABILITY'] = image_crypting_ratio * 100
    new_test_config['CRYPT_URL_RE'] = [
        'test\.local/images/.*?',
        'another\.domain/gifs/.*?',
        'avatars\.mds\.yandex\.net/get-autoru/.*?',
        'im\d\-tub\-ru\.yandex\.net/i.*?',
    ]

    set_handler_with_config(test_config.name, new_test_config)

    def handler(**_):
        return {'text': json.dumps(json_to_test_crypt_probability, sort_keys=True), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/image_urls_json', key, False, origin='test.local')
    original_json = json.loads(requests.get(urljoin(stub_server.url, 'image_urls_json')).text)
    proxied_json = json.loads(requests.get(crypted_link, headers={"host": 'test.local', SEED_HEADER_NAME: 'Sasha', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text)

    for url_class in original_json.keys():
        if url_class in ("partner_images", "other_images"):
            continue
        for raw_url, proxied_url in zip(original_json[url_class], proxied_json[url_class]):
            assert raw_url != proxied_url

    for raw_url, proxied_url in zip(original_json["other_images"], proxied_json["other_images"]):
        assert raw_url == proxied_url

    crypted_urls = 0
    for raw_url, proxied_url in zip(original_json["partner_images"], proxied_json["partner_images"]):
        if raw_url != proxied_url:
            crypted_urls += 1
    assert float(crypted_urls) / len(original_json["partner_images"]) == expexted_ratio


@pytest.mark.parametrize('image_to_iframe_changing_ratio, expexted_ratio, images_regexps', [
    (0., 0., []),
    (0., 0., ['(?:https?:)?//avatars.mds.yandex.net/get-autoru/.*?']),
    (0.2, 0.19, ['\.?/scripts/function/.*?']),
    (0.2, 0.19, ['(?:https?:)?//avatars.mds.yandex.net/get-autoru/.*?']),
    (0.5, 0.5233333333333333, ['(?:https?:)?//another\.domain/gifs/.*?', '(?:https?:)?//im\d\-tub\-ru\.yandex\.net/i.*?']),
    (1., 1., [
        '(?:https?:)?//test\.local/images/.*?',
        '(?:https?:)?//another\.domain/gifs/.*?',
        '(?:https?:)?//avatars\.mds\.yandex\.net/get-autoru/.*?',
        '(?:https?:)?//im\d\-tub\-ru\.yandex\.net/i.*?']),
])
def test_image_to_iframe_random_query_param_adding(stub_server, image_to_iframe_changing_ratio, expexted_ratio,
                                                   images_regexps, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    """
    Проверяет что определенная доля зашифрованных изображений, сматченных на регулярку IMAGE_TO_IFRAME_URL_RE, содержит квери параметр DSD=1 и
    и эта доля совпадает с параметром конфига IMAGE_TO_IFRAME_CHANGING_PROBABILITY.
    :param image_to_iframe_changing_ratio: желаемая доля зашифрованных изображений с квери параметром DSD=1
    :param expexted_ratio: точное значение в спроксированной странице зависит от сида
    :param images_regexps: регулярное выражение чтоб отделить картинки в которые надо добавлять параметр
    """
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['IMAGE_TO_IFRAME_CHANGING_PROBABILITY'] = image_to_iframe_changing_ratio * 100
    new_test_config['IMAGE_TO_IFRAME_URL_RE'] = images_regexps
    new_test_config['CRYPT_URL_RE'] = [
        'test\.local/images/.*?',
        'another\.domain/gifs/.*?',
        'avatars\.mds\.yandex\.net/get-autoru/.*?',
        'im\d\-tub\-ru\.yandex\.net/i.*?',
    ]

    set_handler_with_config(test_config.name, new_test_config)

    def handler(**_):
        return {'text': json.dumps(json_to_test_crypt_probability, sort_keys=True), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/image_urls_json', key, False, origin='test.local')
    original_html = requests.get(urljoin(stub_server.url, 'image_urls_json')).text
    proxied_json = json.loads(requests.get(crypted_link, headers={"host": 'test.local', SEED_HEADER_NAME: 'Sasha', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text)

    original_links_count = 0
    for regex in images_regexps:
        original_links_count += len(re2.findall(regex, original_html))

    crypted_links_count = 0
    for _, list_value in proxied_json.items():
        crypted_links_count += len([url for url in list_value if url.endswith(IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT)])

    assert float(crypted_links_count) / (original_links_count if image_to_iframe_changing_ratio != 0.0 else 1) == expexted_ratio


@pytest.mark.parametrize('img_to_iframe_length', [500, 3000, None])
@pytest.mark.parametrize('img_to_iframe_relative_url', [True, False, None])
def test_image_to_iframe_url_length(img_to_iframe_length, img_to_iframe_relative_url, stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config):
    """
    Проверяет что временно добавленное поле IMAGE_TO_IFRAME_URL_LENGTH
    (удалить после того как все правила https://st.yandex-team.ru/ANTIADBSUP-1346 исчезнут)
    работает корректно. Т.е. ссылки на картинки, которые должны быть преобразованы в iframe
    шуфруются с длиной IMAGE_TO_IFRAME_URL_LENGTH, а не CRYPTED_URL_MIN_LENGTH
    """
    default_crypted_url_length = 1000
    length_precision = 4 + len('http://test.local/')  # укладываемся в погрешность 4 символа на ссылку

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPTED_URL_MIXING_TEMPLATE'] = [('/', 1)]
    new_test_config['IMAGE_TO_IFRAME_CHANGING_PROBABILITY'] = 100
    new_test_config['CRYPTED_URL_MIN_LENGTH'] = default_crypted_url_length
    new_test_config['IMAGE_TO_IFRAME_URL_LENGTH'] = img_to_iframe_length
    if img_to_iframe_length is None:
        del new_test_config['IMAGE_TO_IFRAME_URL_LENGTH']
    new_test_config['IMAGE_TO_IFRAME_URL_IS_RELATIVE'] = img_to_iframe_relative_url
    if img_to_iframe_relative_url is None:
        del new_test_config['IMAGE_TO_IFRAME_URL_IS_RELATIVE']
    new_test_config['IMAGE_TO_IFRAME_URL_RE'] = ['[^\s\"\']*?/image']
    new_test_config['CRYPT_URL_RE'] = new_test_config['IMAGE_TO_IFRAME_URL_RE'] + ['[^\s\"\']*?/else']

    set_handler_with_config(test_config.name, new_test_config)

    def handler(**_):
        content = ['http://ya.ru/else', 'http://ya.ru/image']
        return {'text': json.dumps(content), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/content', key, False, origin='test.local')
    proxied_json = requests.get(crypted_link, headers={"host": 'test.local', SEED_HEADER_NAME: 'damn!!', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()

    expected_img_to_iframe_length = img_to_iframe_length if img_to_iframe_length is not None else default_crypted_url_length
    assert default_crypted_url_length - length_precision < len(proxied_json[0]) < default_crypted_url_length + length_precision
    assert proxied_json[0].startswith('http://test.local')

    image_url_length = len(proxied_json[1]) - len(IMAGE_TO_IFRAME_CRYPTING_URL_FRAGMENT)
    if img_to_iframe_relative_url:
        assert proxied_json[1].startswith('/')
        assert expected_img_to_iframe_length - length_precision < image_url_length + len('http://test.local') < expected_img_to_iframe_length + length_precision
    else:
        assert proxied_json[1].startswith('http://test.local')
        assert expected_img_to_iframe_length - length_precision < image_url_length < expected_img_to_iframe_length + length_precision


@pytest.mark.parametrize('body, crypt_list, result', [
    ('<div class="test" ></div>', ["test"], '<div class="oii9p" ></div>'),
    ('<div class="test" ></div><div class="test" ></div>', ["test"], '<div class="oii9p" ></div><div class="oii9p" ></div>'),
    ('<div class="test" ></div>', ["class=\"(test)\""], '<div class="oii9p" ></div>'),
    ('<div class="test" ></div><div class="test" ></div>', ["class=\"(test)\""], '<div class="oii9p" ></div><div class="oii9p" ></div>'),
    # ('<div class="test" ></div>', {"class=\"(test)\"": "OJLOAEW"}, '<div class="OJLOAEW" ></div>'),
])
def test_bodycrypt_with_lowercase(get_key_and_binurlprefix_from_config, stub_server, set_handler_with_config, get_config, body, crypt_list, result):
    def stub_handler(**_):
        return {'text': body, 'code': 200,
                'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(stub_handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_BODY_RE'] = crypt_list
    new_test_config['CRYPT_IN_LOWERCASE'] = True
    new_test_config['CRYPT_URL_RE'] = ['test\.local/html.*?']

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://test.local/html', key, False, origin='test.local')

    response = requests.get(crypted_link, headers={"HOST": 'test.local',
                                                   SEED_HEADER_NAME: 'my2007',
                                                   PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.status_code == 200
    assert response.text == result


@pytest.mark.parametrize('config_version', [1, 29, 'bro'])
def test_seed_deps_from_config_ver(stub_server, cryprox_worker_url, set_handler_with_config, get_config, config_version):
    """
    Проверяем, что значение seed-а зависит от версии конфига партнера
    :param config_version: версия конфига партнера
    """

    def handler(**_):
        return {'text': '<a href="https://yastatic.net/just-test-url">what</a>', 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()

    set_handler_with_config(test_config.name, new_test_config, version=config_version)

    seed_pure = generate_seed()
    seed_salt = generate_seed(salt=config_version)

    response = requests.get(cryprox_worker_url, headers={"HOST": 'test.local',
                                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text

    assert seed_pure != seed_salt
    assert response.find('/{}'.format(seed_salt)) > 0


@pytest.mark.parametrize('crypt_url_prefix', [None, '/prefix/'])
@pytest.mark.parametrize('second_domain', ['naydex.net', 'nenaydex.net', 'yastatic.net'])
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_crypt_to_the_two_domains(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config, crypt_url_prefix, config_name, second_domain):
    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
    new_test_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain
    new_test_config['CRYPT_URL_RE'].append(r'test\.local/.*?\.(?:j|cs)s')
    new_test_config['CRYPT_URL_RE'].append(r'an\.yandex\.ru/count/.*?')
    new_test_config['CRYPT_URL_RE'].append(r'yandex\.ru/an/count/.*?')
    new_test_config['CRYPT_URL_RE'].append(r'aab\-pub\.s3\.yandex\.net\/iframe\.html')
    new_test_config['CRYPT_RELATIVE_URL_RE'].append(r'\.{0,2}/\S*?\.(?:j|cs)s')
    new_test_config['PARTNER_TO_COOKIELESS_HOST_URLS_RE'] = [r'test\.local/.*?\.js']

    if crypt_url_prefix is not None:
        new_test_config['CRYPT_URL_PREFFIX'] = crypt_url_prefix

    set_handler_with_config(config_name, new_test_config)

    content_urls = {
        "cookieless": [
            "http://test.local/main.js",
            "//direct.yandex.ru/?partner",
            "/very/important/script.js",
            "http://yastatic.net/pcode/media/loader.js",
            "http://aab-pub.s3.yandex.net/iframe.html",
        ],
        "with_cookies": [
            "//test.local/main.css",
            "//ads.adfox.ru/123/goLink?code=trololo",
            "//an.yandex.ru/resource/context_static_r_4061.js",
            "//an.yandex.ru/count/trolololololololololololololololololollololololoolol11111",
            "//yandex.ru/ads/resource/context_static_r_4061.js",
            "//yandex.ru/an/count/trolololololololololololololololololollololololoolol11111",
        ]
    }

    def handler(**_):
        return {'text': json.dumps(content_urls), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)
    pid = ConfigName(config_name).service_id
    if second_domain != 'yastatic.net':
        # test-local.naydex.net/test/...
        expected_fmt = (pid.replace('_', '-').replace('.', '-') + '.', second_domain, COOKIELESS_PATH_PREFIX)
    else:
        # yastatic.net/naydex/test-local/test/...
        expected_fmt = (second_domain, '/naydex/' + pid.replace('_', '-').replace('.', '-'), COOKIELESS_PATH_PREFIX)
    expected_urls = {
        "cookieless": [
            "http://{}{}{}XWtN7S249/my2007kK/Kdf7P9akowI/poeTnSH/hKL05N/oZBqL/lw_Zfwj/OTxZNHNFVZ/VD-g7/YaOvVfHagbvSQ/".format(*expected_fmt),
            "//{}{}{}XWtN7S467/my20071_/mNZvu3Jkp7K/I9eRn6c/y7yupo/QACf7/73sxy3C/2Q255KMkBZ/XQKL6/pfTpRbIZAnResg_/".format(*expected_fmt),
            "http://{}{}{}XWtN9S647/my2007kK/Kdf7P9akowI/poeTnSH/hKL0_9/4CEaP/m3dlv8Q/aw9Kg6DnFi/czuxs/JjTjmflS/ijvWcUp/W9v0r7HH/McotSDRQM6eJ/".format(*expected_fmt),
            "http://{}{}{}XWtN9S538/my2007kK/Kdf7P9akc0I/ppRVnKH/y6C-_Z/QAC-P/r1YZt5h/a4-_N5EnN0/fznr9/IH_jnnlS/TX_RN4n/VdzOvqfA/a4guRDZdAKc/".format(*expected_fmt),
            "http://{}{}{}XWtN9S429/my2007kK/Kdf7P9al80M/8NAV3nK/lv318N/oeDOn/3nsdl91/24_K50EHc-/cj-o8/q3_kHnmV/CXiX9Ap/UubfqKCa/KYsiRjtuAA/".format(*expected_fmt),
        ],
        "with_cookies": [
            "//test.local{}XWtN6S595/my20071_/mdavqma1I6M/o9cDXaF/jKD16s/gDN9P/O8etfzC/CY3ZVbCXdj/bmWp8ZHBvWf7/".format(crypt_url_prefix or '/'),
            "//test.local{}XWtN9S647/my20071_/mIa_r8JFozP/pYeUG7L/1Pzopt/wfJOX/h2_1l8A/bu-bNxGC9k/aCSp8/Z7PjmflS/ijvWcUp/W9v0r7HH/McotSDRQM6eJ/".format(crypt_url_prefix or '/'),
            "//test.local{}SNXt10519/my20071_/mIYaerJFAxN/JYeUG7L/l6uo5s/4CC-m/g08Zu9x/ep7oNmCXNk/cyia7/K2U4Q6VJ/QDDScgh/XdDllIb9/Aq0/PUzJCK9a6hmzovhph/".format(crypt_url_prefix or '/'),
            "//test.local{}SNXt14879/my20071_/mIYaerJFAxN/JYeUG7L/hqGu58/9fHP7/g3MZs7B/6-9rN5En5_/diSp8/Z7PvVfIZ/AbfevgM/c_7Vt7vY/Kog/tSDteM5e6hm/PmvSl/ROWDfA/RMqu7qPms/vkPzrFy/PnndOYB/pbSweh5zfEUU/".format(crypt_url_prefix or '/'),
            "//test.local{}SNXt10737/my20071_/mQbue2IEZ7I/5sfQ3-X/yry--t/QFGu_/qn8pv7Q/a04qhKDmZx/biKmw/YD_5QiSO/kTaZcg_/XdP4hJvm/DKM/IaSNULIz4hW/DqsylhCg/".format(crypt_url_prefix or '/'),
            "//test.local{}SNXt14879/my20071_/mQbue2IEZ7I/5sfQ3XL/hqGu58/9fHP7/g3MZs7B/6-9rN5En5_/diSp8/Z7PvVfIZ/AbfevgM/c_7Vt7vY/Kog/tSDteM5e6hm/PmvSl/ROWDfA/RMqu7qPms/vkPzrFy/PnndOYB/pbSweh5zfEUU/".format(crypt_url_prefix or '/'),
        ]
    }

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, '//test.local/1.html', key, False, origin='test.local')
    response = requests.get('http:' + crypted_url, headers={"host": 'test.local', SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == pid
    assert_that(response.json(), equal_to(expected_urls))


@pytest.mark.parametrize('avatars_not_cookieless', [False, True])
def test_crypt_to_the_two_domains_avatars_not_cookieless(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config, avatars_not_cookieless):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
    new_test_config['AVATARS_NOT_TO_COOKIELESS'] = avatars_not_cookieless
    new_test_config['CRYPT_URL_RE'].append(r'avatars\.mds\.yandex\.net/get\-test/.*?')

    set_handler_with_config('test_local', new_test_config)

    content_urls = [
            "https://avatars.mds.yandex.net/get-test/x400",
            "https://avatars.mds.yandex.net/get-direct/x400",
    ]

    expected_fmt = 'test.local/' if avatars_not_cookieless else 'test-local.naydex.net' + COOKIELESS_PATH_PREFIX
    expected_urls = [
        "http://{}XWtN9S974/my2007kK/Kdf_roahE0J/49EQ2mX/y6O_-p/UJCeL/r1dEu7R/eltbtwCT9k/fzixs/YqU4Qj7V/CvxVMgv/Ttv9kprA/IJc/1CTtePJm6tlA/".format(expected_fmt),
        "http://{}SNXt10301/my2007kK/Kdf_roahE0J/49EQ2mX/y6O_-p/UJCeL/r1dEu7R/eltbtwCT90/czmg_/YaPqQyUO/zXvV9Yi/Q93okpP9/C5A/kVCMfM5e1iGPWjQ/".format(expected_fmt),
    ]

    def handler(**_):
        return {'text': json.dumps(content_urls), 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, '//test.local/1.html', key, False, origin='test.local')
    response = requests.get('http:' + crypted_url, headers={"host": 'test.local', SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert_that(response.json(), equal_to(expected_urls))


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_naydex_domain_handler_work(stub_server, get_key_and_binurlprefix_from_config, cryprox_worker_address, config_name):
    def handler(**request):
        if request.get('path', '/') == '/1.html':
            return {'text': "Victory!", 'code': 200, 'headers': {'Content-Type': 'text/html'}}
        return {'text': 'Unexpected request', 'code': 404}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(config_name)
    pid = ConfigName(config_name).service_id
    crypted_url = crypt_url(binurlprefix, 'http://test.local/1.html', key, False, origin='test.local')
    crypted_url = urlparse(crypted_url)._replace(netloc=cryprox_worker_address).geturl()
    response = requests.get(crypted_url, headers={"host": "{}{}".format(pid.replace('_', '-'), DEFAULT_COOKIELESS_HOST)})
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == pid
    assert response.content == "Victory!"


def test_invalid_url(stub_server, get_config, get_key_and_binurlprefix_from_config, cryprox_worker_address, set_handler_with_config):
    def handler(**request):
        if request.get('path', '/').endswith('file.js'):
            return {'text': '{"/very/important/script.js"}', 'code': 404, 'headers': {'Content-Type': 'text/html'}}
        return {'text': 'Unexpected request', 'code': 500}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RE'].append(r'test\.local/.*?\.(?:j|cs)s')
    new_test_config['CRYPT_RELATIVE_URL_RE'].append(r'\.{0,2}/.*?\.(?:j|cs)s')

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    # сломаем урл, он будет иметь все части чтоб считаться шифрованным, но не будет таким на самом деле
    crypted_url = urljoin(crypt_url(binurlprefix, 'http://test.local/1.html', key, False, 3000, [('/', 1)], origin='test.local'), 'file.js',)
    crypted_url = urlparse(crypted_url)._replace(netloc=cryprox_worker_address).geturl()
    response = requests.get(crypted_url, headers={"HOST": 'test.local',
                                                  SEED_HEADER_NAME: 'my2007',
                                                  PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert response.status_code == 404
    assert 'test.local' in response.text


@pytest.mark.parametrize("meta_attr_value", ["Content-Security-Policy", "\"Content-Security-Policy\""])
@pytest.mark.parametrize('second_domain', ['naydex.net', 'yastatic.net'])
def test_crypt_to_the_two_domains_check_csp(stub_server, set_handler_with_config, get_config, get_key_and_binurlprefix_from_config, meta_attr_value, second_domain):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = True
    new_test_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain
    new_test_config['CRYPT_URL_RE'].append(r'test\.local/.*?\.(?:j|cs)s')
    new_test_config['PARTNER_TO_COOKIELESS_HOST_URLS_RE'] = [r'test\.local/.*?\.js']

    set_handler_with_config(test_config.name, new_test_config)

    test_config_edited = get_config('test_local')

    script_path = "//test.local/omg/ololo.js"

    csp_string_template = "default-src 'none'; connect-src 'self' *.adfox.ru mc.admetrica.ru *.auto.ru auto.ru *.google-analytics.com *.yandex.net *.yandex.ru yandex.ru yastatic.net "\
        "wss://push-sandbox.yandex.ru wss://push.yandex.ru{second_domain_placeholder};font-src 'self' data: fonts.googleapis.com *.gstatic.com an.yandex.ru yastatic.net journalcdn.storage.yandexcloud.net "\
        "autoru-mag.s3.yandex.net{second_domain_placeholder};form-action 'self' money.yandex.ru auth.auto.ru;frame-src *.adfox.ru *.auto.ru auto.ru i.autoi.ru *.avto.ru *.yandex.net *.yandex.ru yandex.ru "\
        "*.yandexadexchange.net yandexadexchange.net yastatic.net *.youtube.com *.vertis.yandex-team.ru *.tinkoff.ru *.spincar.com{second_domain_placeholder};img-src 'self' data: blob: ad.adriver.ru *.adfox.ru "\
        "*.auto.ru auto.ru *.autoi.ru *.tns-counter.ru mc.webvisor.org *.yandex.net yandex.net *.yandex.ru yandex.ru yastatic.net *.ytimg.com mc.admetrica.ru journalcdn.storage.yandexcloud.net "\
        "autoru-mag.s3.yandex.net t.co analytics.twitter.com *.google.com *.google.ru *.google-analytics.com *.googleadservices.com *.googlesyndication.com *.gstatic.com{second_domain_placeholder};media-src "\
        "data: *.adfox.ru *.yandex.net *.yandex.ru yandex.st yastatic.net{second_domain_placeholder}; object-src data: *.adfox.ru *.googlesyndication.com{second_domain_placeholder};script-src 'unsafe-eval' 'unsafe-inline' "\
        "*.auto.ru auto.ru *.adfox.ru *.yandex.ru yandex.ru *.yandex.net yastatic.net static.yastatic.net *.youtube.com *.ytimg.com journalcdn.storage.yandexcloud.net autoru-mag.s3.yandex.net "\
        "static.ads-twitter.com *.twitter.com *.google-analytics.com *.googleadservices.com *.googlesyndication.com *.gstatic.com{second_domain_placeholder};style-src 'self' 'unsafe-inline' *.adfox.ru "\
        "*.auto.ru auto.ru yastatic.net static.yastatic.net journalcdn.storage.yandexcloud.net autoru-mag.s3.yandex.net{second_domain_placeholder};report-uri "\
        "https://csp.yandex.net/csp?from=autoru-frontend-desktop&version=201904.16.121024&yandexuid=4192397941554369671;child-src"
    csp_string = csp_string_template.format(second_domain_placeholder='')

    response_text = '''<!DOCTYPE HTML>
<html>
 <head>
 <meta http-equiv={meta_attr_value} content="{{csp_string}}">
 <meta http-equiv={meta_attr_value} content="{{csp_string}}">
 </head>
 <body>
  <script src="{{script_path}}"></script>
  <p>Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diem
  nonummy nibh euismod tincidunt ut lacreet dolore magna aliguam erat volutpat.</p>
  <p>Ut wisis enim ad minim veniam, quis nostrud exerci tution ullamcorper
  suscipit lobortis nisl ut aliquip ex ea commodo consequat.</p>
 </body>
</html>'''.format(meta_attr_value=meta_attr_value)

    response_headers = {"Cache-Control": "max-age=0, must-revalidate, proxy-revalidate, no-cache, no-store, private",
                        "Date": "Wed, 17 Apr 2019 09:24:24 GMT",
                        "Content-Security-Policy": csp_string,
                        "Content-Type": "text/html", "Pragma": "no-cache", "Server": "nginx", "X-Ua-Bot": "1",
                        "Expires": "Thu, 01 Jan 1970 00:00:01 GMT", "Strict-Transport-Security": "max-age=31536000",
                        "Set-Cookie": "from=direct; Domain=.auto.ru; Path=/; HttpOnly", "X-Frame-Options": "SAMEORIGIN",
                        "Vary": "Accept-Encoding", "X-Content-Type-Options": "nosniff"}

    def handler(**_):
        return {'text': response_text.format(csp_string=csp_string, script_path=script_path), 'code': 200, 'headers': response_headers}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, '//test.local/1.html', key, False, origin='test.local')
    proxied_html = requests.get('http:' + crypted_url, headers={"host": 'test.local', SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    script_path_crypted = urlparse(crypt_url(binurlprefix, script_path, key, True, origin='test.local'))
    crypted_csp_string = csp_string_template.format(second_domain_placeholder=' ' + test_config_edited.second_domain)
    if second_domain != 'yastatic.net':
        script_path_crypted = script_path_crypted._replace(netloc=test_config_edited.second_domain,
                                                           path=COOKIELESS_PATH_PREFIX + script_path_crypted.path.lstrip('/'))
        expected_csp_string = crypted_csp_string
    else:
        script_path_crypted = script_path_crypted._replace(netloc=test_config_edited.second_domain,
                                                           path='/naydex/test-local' + COOKIELESS_PATH_PREFIX + script_path_crypted.path.lstrip('/'))
        expected_csp_string = csp_string

    script_path_crypted_url = script_path_crypted.geturl()
    expected_proxified_html = response_text.format(csp_string=expected_csp_string, script_path=script_path_crypted_url)

    assert proxied_html.text == expected_proxified_html
    assert proxied_html.headers['Content-Security-Policy'] == expected_csp_string


@pytest.mark.parametrize('user_agent,is_mobile', [
    ('Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1', True),
    ('Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0', False),
])
def test_autoredirect_inject_script(stub_server, get_key_and_binurlprefix_from_config, cryprox_worker_address, get_config, restore_stub, user_agent, is_mobile):

    content = "var a,b,c;"

    def handler(**request):
        if 'bla-bla.js' in request.get('path', '/') and 'v=5.2.4&ver=5.2.4' in request.get('query', ''):
            return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/javascript'}}
        return {'text': 'Unexpected request', 'code': 404}

    stub_server.set_handler(handler)

    config = get_config(AUTOREDIRECT_SERVICE_ID)
    key, binurlprefix = get_key_and_binurlprefix_from_config(AUTOREDIRECT_SERVICE_ID)
    crypted_url = crypt_url(binurlprefix, 'http://turbo.local/detect?domain=aabturbo.gq&path=bla-bla.js?ver=5.2.4&proto=http', key, False, origin='test.local')
    crypted_url = urlparse(crypted_url)._replace(netloc=cryprox_worker_address).geturl()
    proxied = requests.get(crypted_url + "?v=5.2.4", headers={"host": "aabturbo-gq{}".format(DEFAULT_COOKIELESS_HOST), "user-agent": user_agent})

    restore_stub()
    test_url = urljoin('http://' + DETECT_LIB_HOST, AUTOREDIRECT_DETECT_LIB_PATH) + '?pid={}'.format(AUTOREDIRECT_SERVICE_ID)
    proxied_with_pid_data = requests.get(crypt_url(binurlprefix, test_url, key, False, origin='test.local'),
                                         headers={"host": "aabturbo-gq{}".format(DEFAULT_COOKIELESS_HOST),
                                                  PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]}).text

    assert 200 == proxied.status_code
    if is_mobile:
        assert content == proxied.content
    else:
        assert proxied_with_pid_data + content == proxied.content


def test_bypass_urls(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['BYPASS_URL_RE'] = [r'yastatic\.net/.*?\.js']
    set_handler_with_config(test_config.name, new_test_config)

    response_text = '''{"non_crypted": ["http://yastatic.net/main.js","http://yastatic.net/static/bundle-1.0.js"],
"crypted": ["http://yastatic.net/main.css","http://yastatic.net/static/bundle-1.0.css"]}'''

    def handler(**_):
        return {'text': response_text, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, '//test.local/1.html', key, False, origin='test.local')
    proxied = requests.get('http:' + crypted_url, headers={"host": TEST_LOCAL_HOST, SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()

    response = json.loads(response_text)
    for url in response["non_crypted"]:
        assert url in proxied["non_crypted"]
    for url in response["crypted"]:
        assert url not in proxied["crypted"]


def test_bypass_relative_urls(stub_server, cryprox_worker_url, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['BYPASS_URL_RE'] = [r'test.local/scripts/.*?\.js']
    new_test_config['CRYPT_RELATIVE_URL_RE'] = [r'/scripts/.*?']
    set_handler_with_config(test_config.name, new_test_config)

    response_text = '''{"non_crypted": ["/scripts/main.js","/scripts/static/bundle-1.0.js"],
    "crypted": ["/scripts/main.css","/scripts/static/bundle-1.0.css"]}'''

    def handler(**_):
        return {'text': response_text, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, '//test.local/1.html', key, False, origin='test.local')
    proxied = requests.get('http:' + crypted_url, headers={"host": TEST_LOCAL_HOST, SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()

    response = json.loads(response_text)
    for url in response["non_crypted"]:
        assert "http://test.local{}".format(url) in proxied["non_crypted"]
    for url in response["crypted"]:
        assert "http://test.local{}".format(url) not in proxied["crypted"]


@pytest.mark.parametrize('crypt_bs_count_url', (None, False, True))
def test_crypt_bs_count_url(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, crypt_bs_count_url):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_BS_COUNT_URL'] = crypt_bs_count_url
    new_test_config['CRYPT_URL_RE'] = []
    new_test_config['CRYPT_ENABLE_TRAILING_SLASH'] = False
    new_test_config['INTERNAL'] = False
    set_handler_with_config(test_config.name, new_test_config)

    stub_json = [
        'https://an.yandex.ru/count/12312',
        'https://yandex.ru/an/count/12312',
        'https://bs.yandex.ru/count/123123',
        'https://yabs.yandex.ru/count/RRMq6vg6dTO50EO0CLpDMbq00000EEJlCOW2wGFO0WBm0lwLzwuB-0BGhuhDbjMlymJm1G6W1ge3mGOrjKYjVWQ17za60000O740002f1t-Bs7YbR8b2yGTRwu2qlO7Tq80A0OWA0QeB45SRh0j_Ym007DqBo7gG1G3m2mQO3jlAhCpDdO_B7U0F0P0GzgY6_8FdzQU70VWG580H6OWH0P0H0QWHm8Gz-X4P3G00000L000001q000009G00000j00000F0I5OWJ0P0JCi0J____________0TeJ2WW0400O0200A000=kBr7a541G0P80c2y26W4SDxqpD47W07U-0I80V-Ziz8za06KXBkvCvW1YAMvyJQu0VJzW9eSs06wdCKOu07AjEq4w05uc0AoXDuLe0B2XKI00-xwZea2Y0EjsRdH29W3oiSGe0C4i0C2w0JNMOW5XxK8a0MxW0om1UgH0hW5q8O3m0MxW0p81T260-051PW6_e2aHgW6gWEm1u20c3JG1mBW1uOAyGTRwu2qlO7TqFW70O080T08a8A0WS2GW8Q00U08uO8YW0e1mGe00000003mFzWA0k0AW8bw-0g0jHZP2t-Bs7YbR8b2w0k7jGY83FQGthu1w0mZYGu6d-RaHIQmFv0Em8Gze0x0X3sX3yFrA5oFmhK_sGy00000003mFu0Gbwsc59eG2H400000003mFyWG2g4H00000000y3-e4S24FR0H0OWI0P0I0PWJ0G00',
        'https://yabs.yandex.by/count/RRMq6vg6dTO50EO0CLpDMbq00000EEJlCOW2wGFO0WBm0lwLzwuB-0BGhuhDbjMlymJm1G6W1ge3mGOrjKYjVWQ17za60000O740002f1t-Bs7YbR8b2yGTRwu2qlO7Tq80A0OWA0QeB45SRh0j_Ym007DqBo7gG1G3m2mQO3jlAhCpDdO_B7U0F0P0GzgY6_8FdzQU70VWG580H6OWH0P0H0QWHm8Gz-X4P3G00000L000001q000009G00000j00000F0I5OWJ0P0JCi0J____________0TeJ2WW0400O0200A000=kBr7a541G0P80c2y26W4SDxqpD47W07U-0I80V-Ziz8za06KXBkvCvW1YAMvyJQu0VJzW9eSs06wdCKOu07AjEq4w05uc0AoXDuLe0B2XKI00-xwZea2Y0EjsRdH29W3oiSGe0C4i0C2w0JNMOW5XxK8a0MxW0om1UgH0hW5q8O3m0MxW0p81T260-051PW6_e2aHgW6gWEm1u20c3JG1mBW1uOAyGTRwu2qlO7TqFW70O080T08a8A0WS2GW8Q00U08uO8YW0e1mGe00000003mFzWA0k0AW8bw-0g0jHZP2t-Bs7YbR8b2w0k7jGY83FQGthu1w0mZYGu6d-RaHIQmFv0Em8Gze0x0X3sX3yFrA5oFmhK_sGy00000003mFu0Gbwsc59eG2H400000003mFyWG2g4H00000000y3-e4S24FR0H0OWI0P0I0PWJ0G00',
    ]

    def handler(**_):
        return {'text': json.dumps(stub_json), 'code': 200, 'headers': {'Content-Type': 'application/json'}}

    stub_server.set_handler(handler)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, 'http://test.local/1.html', key, False, origin='test.local')
    proxied = requests.get(crypted_url, headers={"host": TEST_LOCAL_HOST, SEED_HEADER_NAME: 'my2007', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).json()
    if crypt_bs_count_url:
        crypted_stub_json = map(lambda url: crypt_url(binurlprefix, url, key, False, origin='test.local'), stub_json)
        assert map(lambda url: urlparse(url).path, proxied) == map(lambda url: urlparse(url).path, crypted_stub_json)
    else:
        assert proxied == stub_json


def test_crypt_url_tracking(stub_server, get_key_and_binurlprefix_from_config, get_config):
    content = '"mrcImpression":"https://an.yandex.ru/tracking/00000Eu5xGm50B?action-id=11"'
    expected_content = '"mrcImpression":"http://test.local/SNXt11827/my2007kK/Kdf_roahE0P/8BJQ3WA/gLb1-8/5fHP7/u08Jp7R/X-quwlTSJV/b3692/Z-V4Xqba/gnEf_gO/Mfve5uWF/Grs/AZhVuEKqfrk/bHpiB/NIX-CX0F6iLqR/"'

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    url = "http://an.yandex.ru/meta/rtb_auction_sample?callback=Ya%5B8698788506613%5D"

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, url, key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied.status_code == 200
    assert_that(proxied.text, equal_to(expected_content))


def test_crypt_with_origin(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = '"<script src="http://test.local/kakoytoscript.js"/>'
    # Несмотря на то, что в заголовке host будет test.local, ссылка должна пошифроваться с указанным origin
    expected_content = \
        '"<script src="http://test.com/XWtN8S230/my2007kK/Kdf7P9akowI/poeTnSH/hKL04t/obB_X/739pj8R/uh7vJ_Dk1P/WwqHw/b3ymH_tR/R7VZeNOf_3XhIs/"/>'

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RE'].append(r'test\.local/.*?\.(?:j|cs)s')

    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, 'http://test.local/simple.html', key, False, origin='test.com'),
                           headers={'Host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert_that(proxied.text, equal_to(expected_content))


def test_crypt_with_random_url_preffixes(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = '"<script src="http://test.local/kakoytoscript.js"/>'
    # Несмотря на то, что в заголовке host будет test.local, ссылка должна пошифроваться с указанным origin
    expected_content = [
        '"<script src="http://test.com/preffix1/XWtN8S230/my2007kK/Kdf7P9akowI/poeTnSH/hKL04t/obB_X/739pj8R/uh7vJ_Dk1P/WwqHw/b3ymH_tR/R7VZeNOf_3XhIs/"/>',
        '"<script src="http://test.com/preffix2/XWtN8S230/my2007kK/Kdf7P9akowI/poeTnSH/hKL04t/obB_X/739pj8R/uh7vJ_Dk1P/WwqHw/b3ymH_tR/R7VZeNOf_3XhIs/"/>',
        '"<script src="http://test.com/preffix3/XWtN8S230/my2007kK/Kdf7P9akowI/poeTnSH/hKL04t/obB_X/739pj8R/uh7vJ_Dk1P/WwqHw/b3ymH_tR/R7VZeNOf_3XhIs/"/>',
    ]

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': 'text/html'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RANDOM_PREFFIXES'] = ['/preffix1/', '/preffix2/', '/preffix3/']
    new_test_config['CRYPT_URL_RE'].append(r'test\.local/.*?\.(?:j|cs)s')

    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, 'http://test.local/simple.html', key, False, origin='test.com'),
                           headers={'Host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert_that(proxied.text, any_of(equal_to(expected_content[0]),
                                     equal_to(expected_content[1]),
                                     equal_to(expected_content[2])))


def test_encrypt_links_in_srcset(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    small_image = "//avatars.mds.yandex.net/get-partner/small_image"
    large_image = "//avatars.mds.yandex.net/get-partner/large_image"
    content_template = '<div class="Gallery__activeImg-wrapper">' \
                       '<img class="Gallery__activeImg" alt="" ' \
                       'src="{small_image}" ' \
                       'srcSet="{small_image} 1x, ' \
                       '{large_image} 2x"/></div>'

    def handler(**_):
        return {'text': content_template.format(small_image=small_image, large_image=large_image), 'code': 200,
                'headers': {'Content-Type': 'text/html'}}
    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RE'] = ['avatars\.mds\.yandex\.net\/get\-partner\/(?:small|large)_image']
    new_test_config['CRYPT_ENABLE_TRAILING_SLASH'] = False
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, 'http://test.local/simple.html', key, False),
                           headers={'Host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert_that(proxied.text, equal_to(content_template.format(
        small_image=crypt_url(binurlprefix, small_image, key, False),
        large_image=crypt_url(binurlprefix, large_image, key, False)
    )))


def test_encrypt_links_in_json(stub_server, get_key_and_binurlprefix_from_config, get_config, set_handler_with_config):
    content = "%5C%22iconUrls%5C%22:%5B%5C%22https://avatars.mds.yandex.net/get-ott/374297%5C%22%5D%7D"
    expected = "%5C%22iconUrls%5C%22:%5B%5C%22http://test.local/SNXt10083/my2007kK/Kdf_roahE0J/49EQ2mX/y6O_-p/UJCeL/r1dEu7R/eltbtwCT9_/bj_qr/cWU4wGTV/DXxV9U_/U8DznJ36/MYE/yU3ldMJu3hVDW%5C%22%5D%7D"

    def handler(**_):
        return {'text': content, 'code': 200,
                'headers': {'Content-Type': 'text/html'}}
    stub_server.set_handler(handler)
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['CRYPT_URL_RE'] = ['avatars\.mds\.yandex\.net\/get\-ott\/\S*?']
    new_test_config['CRYPT_ENABLE_TRAILING_SLASH'] = False
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied = requests.get(crypt_url(binurlprefix, 'http://test.local/simple.html', key, False, origin='test.local'),
                           headers={'Host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert_that(proxied.text, equal_to(expected))


patch_csp = {"connect-src": ["strm.yandex.net", "*.strm.yandex.net"], "media-src": ["blob:"]}


@pytest.mark.parametrize('csp_patch', [{}, patch_csp])
@pytest.mark.parametrize('default_src', ["'none'", "'self'"])
@pytest.mark.parametrize('last_symbol', ['', ';'])
def test_patch_csp(stub_server, get_key_and_binurlprefix_from_config, set_handler_with_config, get_config, csp_patch, default_src, last_symbol):
    csp_string_begin = "default-src {};connect-src 'self' yastatic.net".format(default_src)
    csp_string_end = "report-uri https://csp.yandex.net/csp?from=autoru-frontend-desktop&version=201904.16.121024" + last_symbol

    response_text_template = """<!DOCTYPE HTML>
    <html>
     <head>
      <meta http-equiv="Content-Security-Policy" content="{csp_string}">
     </head>
     <body>
      <script type="text/javascript" src="//yastatic.ru/static.js"></script>
     </body>
    </html>"""

    original_csp = csp_string_begin + ";" + csp_string_end

    response_headers = {
        "Content-Security-Policy": original_csp,
        "Content-Type": "text/html",
    }

    def handler(**_):
        return {'text': response_text_template.format(csp_string=original_csp), 'code': 200, 'headers': response_headers}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if csp_patch:
        new_test_config['CSP_PATCH'] = csp_patch
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    headers = {'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}
    proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.html'.format('test.local'), key, False, origin='test.local'), headers=headers)
    if csp_patch:
        if default_src != "'none'":
            patch_csp["media-src"].insert(0, default_src)
        _csp = csp_string_begin + " {};media-src {};".format(" ".join(patch_csp["connect-src"]), " ".join(patch_csp["media-src"])) + csp_string_end
        expected_csp_string = _csp
        expected_body = response_text_template.format(csp_string=_csp)
    else:
        expected_csp_string = original_csp
        expected_body = response_text_template.format(csp_string=original_csp)

    assert proxied.status_code == 200
    assert_that(proxied.text, equal_to(expected_body))
    assert_that(proxied.headers['Content-Security-Policy'], equal_to(expected_csp_string))
