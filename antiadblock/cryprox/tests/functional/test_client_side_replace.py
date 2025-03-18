import json
from urlparse import urljoin
from base64 import urlsafe_b64encode
from time import gmtime, strptime, mktime

import re2
import pytest
import requests
from hamcrest import assert_that, equal_to
from urllib3.util import SKIP_HEADER

from antiadblock.cryprox.cryprox.common.tools.regexp import re_merge
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.config.service import COOKIELESS_PATH_PREFIX
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, validate_script_key
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST, AUTO_RU_HOST, DEFAULT_COOKIELESS_HOST


@pytest.mark.parametrize('service_id, partner_host', [('autoru', AUTO_RU_HOST),
                                                      ("test_local", TEST_LOCAL_HOST),
                                                      ("test_local_2::active::None::None", TEST_LOCAL_HOST)])
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG.js', 'http://yastat.net/partner-code/PCODE_CONFIG.js'))
@pytest.mark.parametrize('argus_proxy', (True, False))
@pytest.mark.usefixtures('stub_server')
def test_macros_ADB_CONFIG(service_id, partner_host, url, get_key_and_binurlprefix_from_config, get_config, argus_proxy):

    p_config = get_config(service_id)
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config)

    expected_config = {'encode': {'key': urlsafe_b64encode(p_key)},
                       'detect': {'links': p_config.PARTNER_DETECT_LINKS + system_config.DETECT_LINKS,
                                  'trusted': p_config.DETECT_TRUSTED_LINKS,
                                  'custom': system_config.DETECT_HTML + p_config.PARTNER_DETECT_HTML,
                                  'iframes': p_config.PARTNER_DETECT_IFRAME},
                       'pid': service_id.split('::', 1)[0],
                       'extuidCookies': p_config.EXTUID_COOKIE_NAMES,
                       'cookieMatching': {'publisherTag': p_config.EXTUID_TAG,
                                          'types': p_config.CM_TYPES,
                                          'redirectUrl': p_config.CM_REDIRECT_URL,
                                          'imageUrl': p_config.CM_IMAGE_URL,
                                          'publisherKey': p_config.PUBLISHER_SECRET_KEY,
                                          'cryptedUidUrl': system_config.CRYPTED_YAUID_URL,
                                          'cryptedUidCookie': system_config.CRYPTED_YAUID_COOKIE_NAME,
                                          'cryptedUidTTL': system_config.CRYPTED_YAUID_TTL,
                                          },
                       'cookieDomain': {'type': p_config.DETECT_COOKIE_TYPE,
                                        'list': [],
                                        },
                       'cookieTTL': system_config.DETECT_COOKIE_TTL,
                       'log': {'percent': 0,
                               },
                       'rtbRequestViaScript': p_config.RTB_AUCTION_VIA_SCRIPT,
                       'countToXhr': p_config.COUNT_TO_XHR,
                       'treeProtection': {'enabled': p_config.DIV_SHIELD_ENABLE},
                       'hideMetaArgsUrlMaxSize': p_config.CRYPTED_URL_MIN_LENGTH,
                       'disableShadow': p_config.DISABLE_SHADOW_DOM,
                       'cookieName': p_config.CURRENT_COOKIE,
                       'deprecatedCookies': p_config.DEPRECATED_COOKIES,
                       'hideLinks': p_config.HIDE_LINKS,
                       'invertedCookieEnabled': p_config.INVERTED_COOKIE_ENABLED,
                       'removeAttributeId': p_config.REMOVE_ATTRIBUTE_ID,
                       'blockToIframeSelectors': p_config.BLOCK_TO_IFRAME_SELECTORS,
                       'additionalParams': p_config.ADDITIONAL_PCODE_PARAMS,
                       }

    headers = {"host": partner_host,
               system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}

    if argus_proxy:
        expected_config['pcodeDebug'] = True
        headers[system_config.ARGUS_REPLAY_HEADER_NAME] = 'headerForTest'

    proxied_data = requests.get(crypt_url(p_binurlprefix, url, p_key, False, origin=partner_host),
                                headers=headers).text

    test_url = urljoin('http://' + system_config.DETECT_LIB_HOST, system_config.DETECT_LIB_PATHS[0]) + '?pid={}'.format(service_id.split('::', 1)[0])
    proxied_with_pid_data = requests.get(crypt_url(p_binurlprefix, test_url, p_key, False, origin=partner_host),
                                         headers=headers).text

    # ADB_CONFIG = |{some valid json ...|,"fn": {js function which is not valid json}}
    adb_replaced_config = json.loads(proxied_data.split(' = ')[1].split(',"fn"')[0] + '}')

    assert_that(adb_replaced_config, equal_to(expected_config))
    assert_that(proxied_data, equal_to(proxied_with_pid_data))


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('second_domain', ('naydex.net', 'yastatic.net', None))
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG.js', 'http://yastat.net/partner-code/PCODE_CONFIG.js'))
@pytest.mark.parametrize('replace_body_re', (None, {'charCodeAt': 'someReplace'}))
@pytest.mark.parametrize('crypt_body_re', (False, True))
def test_macros_ADB_functions(get_config, get_key_and_binurlprefix_from_config, set_handler_with_config,
                              second_domain, url, replace_body_re, crypt_body_re):
    p_config = get_config('test_local')
    new_p_config = p_config.to_dict()
    if second_domain is not None:
        new_p_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = second_domain
        new_p_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain
    if replace_body_re:
        new_p_config['REPLACE_BODY_RE'] = replace_body_re
    if crypt_body_re:
        new_p_config['CRYPT_BODY_RE'] = ['fromCharCode']

    set_handler_with_config(p_config.name, new_p_config)
    seed = 'my2007'
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config, seed)

    proxied_data = requests.get(crypt_url(p_binurlprefix, url, p_key, False, origin='test.local'),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    if second_domain is None:
        cookieless_url_prefix = 'http://test.local/'
        cookieless_path_prefix = ''
    elif second_domain == 'yastatic.net':
        cookieless_url_prefix = 'http://yastatic.net/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = '/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
    else:
        cookieless_url_prefix = 'http://test-local.{}{}'.format(second_domain, COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = COOKIELESS_PATH_PREFIX

    def replace_in_functions(s):
        client_cookieless_pattern = re_merge(p_config.YANDEX_STATIC_URL_RE).replace('/','\/').replace('\\\\', '\\')
        all_url_prefixes = p_config.CRYPT_PREFFIXES
        if second_domain is not None:
            all_url_prefixes = re_merge([all_url_prefixes, re2.escape(cookieless_path_prefix)])
        is_encoded_url_regex = r'/^(?:https?:)?\/\/[^/]+?(?:{all_url_prefixes})\w{{9}}\/{seed}./'.format(all_url_prefixes=all_url_prefixes, seed=seed)
        return (s.replace('{seed}', seed)
                .replace('{encode_key}', urlsafe_b64encode(p_key))
                .replace('{url_prefix}', 'http://test.local/')
                .replace('{is_encoded_url_regex}', is_encoded_url_regex)
                .replace('{seedForMyRandom}', seed.encode('hex'))
                .replace('{cookieless_url_prefix}', cookieless_url_prefix)
                .replace('{client_cookieless_regex}', client_cookieless_pattern)
                .replace('{aab_origin}', 'test.local'))
    # Split on ADB_CONFIG and ADB_FUNCTIONS, split by function marker and remove trailing symbols };
    crypted_functions_in_adb_config = proxied_data.split("\n")[0].split('"fn":')[1][:-2]
    crypted_functions_in_adb_functions = proxied_data.split("\n")[1].split('"fn":')[1][:-2]

    expected_functions = replace_in_functions(p_config.crypted_functions.lstrip(',"fn":'))
    if replace_body_re:
        for k, v in replace_body_re.items():
            expected_functions = expected_functions.replace(k, v)
    if crypt_body_re:
        expected_functions = expected_functions.replace('fromCharCode', 'onqSGYsq6JEwWPopV')
    assert_that(crypted_functions_in_adb_config, equal_to(expected_functions))
    assert_that(crypted_functions_in_adb_functions, equal_to(expected_functions))


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('second_domain', ('naydex.net', 'yastatic.net', None))
@pytest.mark.parametrize('add_partner_patterns', (True, False))
def test_macros_ADB_functions_with_two_domains(get_config, get_key_and_binurlprefix_from_config, set_handler_with_config, second_domain, add_partner_patterns):
    url = 'http://yastatic.net/partner-code/PCODE_CONFIG.js'
    p_config = get_config('test_local')
    new_p_config = p_config.to_dict()
    if second_domain is not None:
        new_p_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = second_domain
        new_p_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain
    new_p_config['PARTNER_TO_COOKIELESS_HOST_URLS_RE'] = [r'cryprox\.yandex\.net', r'yandex\.ru/kakayatossylka'] if add_partner_patterns else []

    set_handler_with_config(p_config.name, new_p_config)
    seed = 'my2007'
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config, seed)

    proxied_data = requests.get(crypt_url(p_binurlprefix, url, p_key, False, origin='test.local'),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    if second_domain is None:
        cookieless_url_prefix = 'http://test.local/'
        cookieless_path_prefix = ''
    elif second_domain == 'yastatic.net':
        cookieless_url_prefix = 'http://yastatic.net/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = '/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
    else:
        cookieless_url_prefix = 'http://test-local.{}{}'.format(second_domain, COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = COOKIELESS_PATH_PREFIX

    def replace_in_functions(s):
        client_cookieless_pattern = re_merge(new_p_config['PARTNER_TO_COOKIELESS_HOST_URLS_RE'] + p_config.YANDEX_STATIC_URL_RE).replace('/', '\/').replace('\\\\', '\\')
        all_url_prefixes = p_config.CRYPT_PREFFIXES
        if second_domain is not None:
            all_url_prefixes = re_merge([all_url_prefixes, re2.escape(cookieless_path_prefix)])
        is_encoded_url_regex = r'/^(?:https?:)?\/\/[^/]+?(?:{all_url_prefixes})\w{{9}}\/{seed}./'.format(all_url_prefixes=all_url_prefixes, seed=seed)
        return (s.replace('{seed}', seed)
                .replace('{encode_key}', urlsafe_b64encode(p_key))
                .replace('{url_prefix}', 'http://test.local/')
                .replace('{is_encoded_url_regex}', is_encoded_url_regex)
                .replace('{seedForMyRandom}', seed.encode('hex'))
                .replace('{cookieless_url_prefix}', cookieless_url_prefix)
                .replace('{client_cookieless_regex}', client_cookieless_pattern)
                .replace('{aab_origin}', 'test.local'))
    # Split on ADB_CONFIG and ADB_FUNCTIONS, split by function marker and remove trailing symbols };
    crypted_functions_in_adb_config = proxied_data.split("\n")[0].split('"fn":')[1][:-2]
    crypted_functions_in_adb_functions = proxied_data.split("\n")[1].split('"fn":')[1][:-2]

    expected_functions = replace_in_functions(p_config.crypted_functions.lstrip(',"fn":'))
    assert_that(crypted_functions_in_adb_config, equal_to(expected_functions))
    assert_that(crypted_functions_in_adb_functions, equal_to(expected_functions))


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('second_domain', ('naydex.net', 'yastatic.net', None))
@pytest.mark.parametrize('content_type', ('application/javascript', 'text/html'))
@pytest.mark.parametrize('replace_adb_functions', (True, False))
def test_macros_ADB_functions_in_partner_request(stub_server, get_config, get_key_and_binurlprefix_from_config, set_handler_with_config, second_domain, content_type, replace_adb_functions):

    content = "const adblockFunctions = '__ADB_FUNCTIONS__';"

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': content_type}}

    stub_server.set_handler(handler)

    p_config = get_config('test_local')
    new_p_config = p_config.to_dict()
    if second_domain is not None:
        new_p_config['ENCRYPT_TO_THE_TWO_DOMAINS'] = second_domain
        new_p_config['PARTNER_COOKIELESS_DOMAIN'] = second_domain
    new_p_config['REPLACE_ADB_FUNCTIONS'] = replace_adb_functions

    set_handler_with_config(p_config.name, new_p_config)
    seed = 'my2007'
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config, seed)

    proxied_data = requests.get(crypt_url(p_binurlprefix, 'http://{}/test.html'.format('test.local'), p_key, False, origin='test.local'),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    if second_domain is None:
        cookieless_url_prefix = 'http://test.local/'
        cookieless_path_prefix = ''
    elif second_domain == 'yastatic.net':
        cookieless_url_prefix = 'http://yastatic.net/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = '/naydex/test-local{}'.format(COOKIELESS_PATH_PREFIX)
    else:
        cookieless_url_prefix = 'http://test-local.{}{}'.format(second_domain, COOKIELESS_PATH_PREFIX)
        cookieless_path_prefix = COOKIELESS_PATH_PREFIX

    def replace_in_functions(s):
        all_url_prefixes = p_config.CRYPT_PREFFIXES
        if second_domain is not None:
            all_url_prefixes = re_merge([all_url_prefixes, re2.escape(cookieless_path_prefix)])
        is_encoded_url_regex = r'/^(?:https?:)?\/\/[^/]+?(?:{all_url_prefixes})\w{{9}}\/{seed}./'.format(all_url_prefixes=all_url_prefixes, seed=seed)
        client_cookieless_pattern = re_merge(p_config.YANDEX_STATIC_URL_RE).replace('/', '\/').replace('\\\\', '\\')
        return (s.replace('{seed}', seed)
                .replace('{encode_key}', urlsafe_b64encode(p_key))
                .replace('{url_prefix}', 'http://test.local/')
                .replace('{is_encoded_url_regex}', is_encoded_url_regex)
                .replace('{seedForMyRandom}', seed.encode('hex'))
                .replace('{cookieless_url_prefix}', cookieless_url_prefix)
                .replace('{client_cookieless_regex}', client_cookieless_pattern)
                .replace('{aab_origin}', 'test.local'))

    if replace_adb_functions:
        crypted_functions_in_adb_functions = proxied_data.split('"fn":')[1][:-2]

        expected_functions = replace_in_functions(p_config.crypted_functions.lstrip(',"fn":'))
        assert_that(crypted_functions_in_adb_functions, equal_to(expected_functions))
    else:
        assert_that(proxied_data, equal_to(content))


@pytest.mark.usefixtures('stub_server')
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG_DOUBLE.js', 'http://yastat.net/partner-code/PCODE_CONFIG_DOUBLE.js'))
def test_js_replace_replaces_once_only(get_key_and_binurlprefix_from_config, get_config, url):
    config = get_config('test_local')
    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config("test_local")
    proxied_data = requests.get(crypt_url(binurlprefix, url, key, False),
                                headers={'host': TEST_LOCAL_HOST,
                                         system_config.SEED_HEADER_NAME: seed,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]}).text
    assert proxied_data.count('__ADB_CONFIG__') == 1
    assert proxied_data.count('__ADB_FUNCTIONS__') == 1


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG.js', 'http://yastat.net/partner-code/PCODE_CONFIG.js'))
def test_mode_by_pass(get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, url):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['DISABLE_DETECT'] = True

    set_handler_with_config(test_config.name, new_test_config)

    p_config = get_config('test_local')
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config)

    proxied_data = requests.get(
        crypt_url(p_binurlprefix, url, p_key, False),
        headers={"host": TEST_LOCAL_HOST,
                 system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    # ADB_CONFIG = |{some valid json ...|,"fn": {js function which is not valid json}}
    adb_replaced_config = json.loads(proxied_data.split(' = ')[1].split(',"fn"')[0] + '}')

    curtime = mktime(gmtime())
    time = mktime(strptime(adb_replaced_config["dbltsr"], "%Y-%m-%dT%H:%M:%S+0000"))
    assert abs(time - curtime) < 30  # delta < 30s


@pytest.mark.parametrize('adb_script_name', ('PCODE_CONFIG_1.js', 'PCODE_CONFIG_2.js', 'PCODE_CONFIG_3.js', 'PCODE_CONFIG_4.js'))
@pytest.mark.parametrize('url_base', ('http://yastatic.net/partner-code/', 'http://yastat.net/partner-code/'))
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
@pytest.mark.usefixtures('stub_server')
def test_macros_ADB_CONFIG_with_script_key(stub_server, get_key_and_binurlprefix_from_config, get_config, adb_script_name, url_base, config_name):

    resource = {
        'PCODE_CONFIG_1.js': 'var a,b,c;a="__scriptKey0Value__",b="__scriptKey1Value__",c="__scriptKey2Value__"',
        'PCODE_CONFIG_2.js': 'var a,b;' + 'a="__scriptKey0Value__",b="__scriptKey1Value__",' * 5 + 'a="__scriptKey0Value__",' * 5 + 'b="__scriptKey1Value__"',
        'PCODE_CONFIG_3.js': 'var A,B;' + 'A="__scriptKey0Value__",B="__scriptKey1Value__",' * 5 + 'A="__scriptKey0Value__",' * 5 + 'B="__scriptKey1Value__"',
        'PCODE_CONFIG_4.js': 'var _A1,Bs3;' + '_A1="__scriptKey0Value__",Bs3="__scriptKey1Value__",' * 5 + '_A1="__scriptKey0Value__",' * 5 + 'Bs3="__scriptKey1Value__"',
    }

    def handler(**_):
        return {'text': resource.get(adb_script_name, ''), 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    p_config = get_config(config_name)
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config)

    proxied_data = requests.get(crypt_url(p_binurlprefix, url_base + adb_script_name, p_key, False),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    assert '__scriptKey0Value__' not in proxied_data
    assert '__scriptKey1Value__' not in proxied_data
    parts = re2.findall(r'\b(a|b|A|B|_A1|Bs3)=\"(\w+?)\"', proxied_data)
    assert len(parts) == 2
    parts = dict(parts)
    part_1 = parts.get("a", parts.get("A", parts.get("_A1", "")))
    part_2 = parts.get("b", parts.get("B", parts.get("Bs3", "")))
    assert validate_script_key(str(part_1) + str(part_2), p_config.CRYPT_SECRET_KEY)[0]


@pytest.mark.usefixtures("stub_server")
@pytest.mark.parametrize('url', ('http://yastatic.net/partner-code/PCODE_CONFIG.js', 'http://yastat.net/partner-code/PCODE_CONFIG.js'))
def test_detect_trusted_links(get_key_and_binurlprefix_from_config, get_config, set_handler_with_config, url):
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    detect_trusted_links = [
        {'type': 'get', 'src': 'ya.ru'},
        {'type': 'head', 'src': 'google.ru'},
    ]
    new_test_config['DETECT_TRUSTED_LINKS'] = detect_trusted_links

    set_handler_with_config(test_config.name, new_test_config)

    p_config = get_config('test_local')
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config)

    proxied_data = requests.get(
        crypt_url(p_binurlprefix, url, p_key, False),
        headers={"host": TEST_LOCAL_HOST,
                 system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0]}).text

    # ADB_CONFIG = |{some valid json ...|,"fn": {js function which is not valid json}}
    adb_replaced_config = json.loads(proxied_data.split(' = ')[1].split(',"fn"')[0] + '}')
    assert_that(adb_replaced_config['detect']['trusted'], equal_to(detect_trusted_links))


USER_AGENTS = {
    'without': None,
    'desktop': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.14; rv:69.0) Gecko/20100101 Firefox/69.0',
    'mobile': 'Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; Desire_A8181 Build/FRF91) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1',
}


@pytest.mark.parametrize('content_type', ('application/javascript', 'text/html'))
@pytest.mark.parametrize('user_agent', USER_AGENTS.values())
def test_macros_ADB_cookies_in_partner_request(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config, content_type, user_agent):

    content = "const adblockCookies = '__ADB_COOKIES__';"

    if user_agent is None:
        user_agent = SKIP_HEADER

    def handler(**_):
        return {'text': content, 'code': 200, 'headers': {'Content-Type': content_type}}

    stub_server.set_handler(handler)

    p_config = get_config('test_local')

    new_p_config = p_config.to_dict()
    new_p_config['CURRENT_COOKIE'] = 'somecookie'
    new_p_config['DEPRECATED_COOKIES'] = ['oldcookie1', 'oldcookie2']

    set_handler_with_config(p_config.name, new_p_config)

    seed = 'my2007'
    p_key, p_binurlprefix = get_key_and_binurlprefix_from_config(p_config, seed)
    proxied_data = requests.get(crypt_url(p_binurlprefix, 'http://{}/test.html'.format('test.local'), p_key, False, origin='test.local'),
                                headers={"host": TEST_LOCAL_HOST,
                                         system_config.PARTNER_TOKEN_HEADER_NAME: p_config.PARTNER_TOKENS[0],
                                         "user-agent": user_agent}).text

    if user_agent == USER_AGENTS['desktop']:
        assert_that(proxied_data, equal_to(content))  # without replace
    else:
        expected_cookie_params = {
            'cookieName': new_p_config['CURRENT_COOKIE'],
            'deprecatedCookies': new_p_config['DEPRECATED_COOKIES'],
        }
        adb_replaced_cookies = json.loads(proxied_data.split(' = ')[1][:-1])
        assert_that(adb_replaced_cookies, equal_to(expected_cookie_params))
