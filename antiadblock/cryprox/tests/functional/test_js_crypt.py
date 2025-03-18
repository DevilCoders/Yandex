# -*- coding: utf8 -*-
import json

import re2
import pytest
import requests
from urllib import unquote
from bs4 import BeautifulSoup
from hamcrest import assert_that, equal_to, is_not

from library.python import resource

from antiadblock.libs.decrypt_url.lib import get_key, decrypt_xor
from antiadblock.cryprox.cryprox.config.system import SEED_HEADER_NAME, PARTNER_TOKEN_HEADER_NAME, EncryptionSteps, DETECT_LIB_PATHS, DETECT_LIB_HOST
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, CryptUrlPrefix, crypt_js_body, SCRIPT_REMOVER
from antiadblock.cryprox.cryprox.common.cry import generate_seed


js_code_original = """
function aab(result) {
    if (result.blocked === true) {
         console.log('AdBlock Enabled')
         console.log(result)
     } else {
         console.log('Адблок включен')
         console.log(result)
     }
};
"""

html_with_js_original = resource.find("resources/test_js_crypt/html_with_js_original.html").encode('utf-8')

expected_crypted_js = resource.find("resources/test_js_crypt/expected_crypted.js")

# содержит 2 поля html в которых по разному заэнкоженные <script>
json_code_original = resource.find("resources/test_js_crypt/json_code_original.json")

html_with_resource_src = r'''
<html>
    <head>
        <link rel="stylesheet" href="//yastatic.net/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1">
        <link rel="stylesheet" href="//yastatic.net/vBWQGTk6rxkZqP5laLcgPsydoqg.css">
        <link rel="stylesheet" href="//yastatic.net/v1bD9ivqzar6ZtsTtObKnHLT3Po.css?check-xhr-working=1" />
    </head>
    <body>
        <style src="//yastatic.net/static.css"></style>
        <script type="text/javascript" src="//yastatic.net/static.js"></script>
        <script src="//yastatic.net/static.js"></script>
        <script src="//yastatic.net/static.js" type="application/javascript"></script>
        <script src="//yastatic.net/static.js"
            type="application/javascript">
        </script>
        <script src="//yastatic.net/static.js" type=""></script>
        <script type="text/javascript" src="//yastatic.ru/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
crypted_html_with_resource_src = r'''
<html>
    <head>
        <link rel="stylesheet" href="//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/">
        <link rel="stylesheet" href="//test.local/SNXt10301/my20071_/mQbvqmJEo8M/sBeR2_L/k4yM2P/wkA7r/9yMJa8i/Lk9r1ZHnVA/aTKh8/YPH_1vXe/DXvV9Yi/Q93okpP9/C5A/kVCMfM5e1iGPWjQ/">
        <link rel="stylesheet" href="//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/" />
    </head>
    <body>
        <style src="//test.local/XWtN7S249/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRu_/8w_Zfwj/OTxZNHNFVZ/VD-g7/YaOvVfHagbvSQ/"></style>
        <script type="text/javascript" src="//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/"></script>
        <script src="//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/"></script>
        <script src="//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/" type="application/javascript"></script>
        <script src="//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/"
            type="application/javascript">
        </script>
        <script src="//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/" type=""></script>
        <script type="text/javascript" src="//yastatic.ru/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
expected_html_with_resource_src = r'''
<html>
    <head>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/",!0);
            xhr.onload=function(){var a=document.createElement("style");
            a.setAttribute("nonce", "qwerty9876543221");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <link rel="stylesheet" href="//test.local/SNXt10301/my20071_/mQbvqmJEo8M/sBeR2_L/k4yM2P/wkA7r/9yMJa8i/Lk9r1ZHnVA/aTKh8/YPH_1vXe/DXvV9Yi/Q93okpP9/C5A/kVCMfM5e1iGPWjQ/">
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/",!0);
            xhr.onload=function(){var a=document.createElement("style");
            a.setAttribute("nonce", "qwerty9876543221");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
    </head>
    <body>
        <style src="//test.local/XWtN7S249/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRu_/8w_Zfwj/OTxZNHNFVZ/VD-g7/YaOvVfHagbvSQ/"></style>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.setAttribute("nonce", "12345678qwerty");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.setAttribute("nonce", "12345678qwerty");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="application/javascript";
            a.setAttribute("nonce", "12345678qwerty");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="application/javascript";
            a.setAttribute("nonce", "12345678qwerty");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!0);
            xhr.onload=function(){var a=document.createElement("script");a.type="text/javascript";
            a.setAttribute("nonce", "12345678qwerty");a.innerHTML=xhr.responseText;document.head.appendChild(a)};xhr.send();
        })();{SCRIPT_REMOVER}
        </script>
        <script type="text/javascript" src="//yastatic.ru/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')
expected_html_with_resource_src_sync = r'''
<html>
    <head>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/",!1);
            xhr.send();
            document.write('<style nonce="qwerty9876543221">'+xhr.responseText+'</'+'style>');
        })();{SCRIPT_REMOVER}
        </script>
        <link rel="stylesheet" href="//test.local/SNXt10301/my20071_/mQbvqmJEo8M/sBeR2_L/k4yM2P/wkA7r/9yMJa8i/Lk9r1ZHnVA/aTKh8/YPH_1vXe/DXvV9Yi/Q93okpP9/C5A/kVCMfM5e1iGPWjQ/">
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/SNXt13135/my20071_/mQbvqmJEo8M/sBeR2_L/k_-5zY/IZHv3/10ds22Q/aizqhaH1l-/UgeRr/aLP_1vXe/FXTfvID/d7_Cs6aZ/Mos/zTD5fOMXntl/DIkwd/hGgOnd/2tVkIC9r6/fXHwvt4-_2/",!1);
            xhr.send();
            document.write('<style nonce="qwerty9876543221">'+xhr.responseText+'</'+'style>');
        })();{SCRIPT_REMOVER}
        </script>
    </head>
    <body>
        <style src="//test.local/XWtN7S249/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRu_/8w_Zfwj/OTxZNHNFVZ/VD-g7/YaOvVfHagbvSQ/"></style>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!1);
            xhr.send();
            document.write('<script nonce="12345678qwerty">'+xhr.responseText+'</'+'script>');
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!1);
            xhr.send();
            document.write('<script nonce="12345678qwerty">'+xhr.responseText+'</'+'script>');
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!1);
            xhr.send();
            document.write('<script nonce="12345678qwerty">'+xhr.responseText+'</'+'script>');
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!1);
            xhr.send();
            document.write('<script nonce="12345678qwerty">'+xhr.responseText+'</'+'script>');
        })();{SCRIPT_REMOVER}
        </script>
        <script nonce="12345678qwerty" type="text/javascript">
        (function(){
            var XMLHttpRequest=window.XDomainRequest||window.XMLHttpRequest,xhr=new XMLHttpRequest;
            xhr.open("get","//test.local/XWtN7S031/my20071_/mQbvqmJEo8M/sBeR2_L/lrq6_d/ITRub/87_ZBwj/CO1Y5cOlte/bi626/tzMvlvFZzXv/",!1);
            xhr.send();
            document.write('<script nonce="12345678qwerty">'+xhr.responseText+'</'+'script>');
        })();{SCRIPT_REMOVER}
        </script>
        <script type="text/javascript" src="//yastatic.ru/static.js"></script>
        <script type="text/javascript">window.alert(1);</script>
    </body>
</html>
'''.replace(' ' * 4, '').replace('\n', '')


def handler(**request):
    path = request.get('path', '/')
    req_headers = {key.replace('HTTP_', '').replace('_', '-').lower(): val for key, val in request['headers'].items()}

    # Using `expected-content-type` header value to control `content-type` header in response
    headers = {'content-type': req_headers['expected-content-type'], 'Content-Security-Policy': req_headers.get('expected-csp', '')}

    if path == '/js_crypt.js':
        return {'text': js_code_original, 'code': 200, 'headers': headers}
    elif path == '/js_crypt.html':
        return {'text': html_with_js_original, 'code': 200, 'headers': headers}
    elif path == '/json_crypt.json':
        return {'text': json_code_original, 'code': 200, 'headers': headers}
    elif path == '/replace_resource_with_xhr.html':
        return {'text': html_with_resource_src, 'code': 200, 'headers': headers}

    return {'text': 'What are u looking for?', 'code': 404}


@pytest.mark.parametrize('encryption_step_enabled,script_remove_after_use,expected_content_type,crypted', [
    (True, True, 'text/javascript', True),
    (True, False, 'application/x-javascript', True),
    (True, True, 'application/javascript', True),
    (True, False, 'application/json', False),
    (True, True, 'text/css', False),
    # mime-type 'text/html' fixed to 'application/javascript'
    (True, False, 'text/html', True),
    (False, True, 'text/javascript', False),
    (False, False, 'application/x-javascript', False),
    (False, True, 'application/javascript', False),
    (False, False, 'application/json', False),
    (False, True, 'text/css', False),
    (False, False, 'text/html', False)])
def test_loaded_js_crypt(stub_server, cryprox_worker_address, set_handler_with_config, get_config, encryption_step_enabled, expected_content_type, crypted, script_remove_after_use):
    """
    Проверяем работу шифрования содержимого JS скриптов, загружаемых по ссылкам https://st.yandex-team.ru/ANTIADB-1032
    :param encryption_step_enabled: флаг - включен или нет EncryptionSteps.loaded_js_crypt
    :param script_remove_after_use: включено или нет удаление скриптов из DOM после их работы
    :param expected_content_type: ожидаемый тип контента в ответе заглушки
    :param crypted: флаг True/False - ожидается ли, что скрипт будет зашифрован в проксе
    """

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if encryption_step_enabled:
        new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.loaded_js_crypt.value)
        new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = script_remove_after_use

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'

    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, test_config.CRYPT_URL_PREFFIX)

    # Дополнительные тестовые кейсы с разными доменами - проверяем, что шифрование скриптов работает для ссылок с любых разрешенных доменов, а не только для партнерских
    for host in ['an.yandex.ru', 'test.local', 'ads.adfox.ru', 'yastatic.net', 'yastat.net']:

        crypted_url = crypt_url(binurlprefix, 'http://{}/js_crypt.js'.format(host), key, False, origin='test.local')

        proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                          SEED_HEADER_NAME: seed,
                                                          PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                          'expected-content-type': expected_content_type}).content

        if crypted:
            assert_that(proxied_data, equal_to(expected_crypted_js + (SCRIPT_REMOVER if script_remove_after_use else "")))
        else:
            assert proxied_data == js_code_original


IE11_USERAGENT = 'Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko'
EDGE_USERAGENT ='Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.74 Safari/537.36 Edg/79.0.309.43'
CHROME_USERAGENT = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/70.0.3538.77 Safari/537.36'


@pytest.mark.parametrize('encryption_step_enabled,script_remove_after_use,expected_content_type,useragent,crypted', [
    (True, True, 'text/javascript', None,  False),
    (True, False, 'application/x-javascript', None, False),
    (True, True, 'application/javascript', None, False),
    (True, False, 'application/json', None, True),
    (True, True, 'text/css', None, True),
    (True, False, 'text/html', None, True),
    (False, True, 'text/javascript', None, False),
    (False, False, 'application/x-javascript', None, False),
    (False, True, 'application/javascript', None, False),
    (False, False, 'application/json', None, False),
    (False, True, 'text/css', None, False),
    (False, False, 'text/html', None, False),
    (True, False, 'text/html', IE11_USERAGENT, False),
    (True, False, 'text/html', EDGE_USERAGENT, False),
    (True, False, 'text/html', CHROME_USERAGENT, True),
])
def test_inline_js_crypt(stub_server, cryprox_worker_address, set_handler_with_config, get_config, encryption_step_enabled, expected_content_type, useragent, crypted, script_remove_after_use):
    """
    Проверяем работу шифрования содержимого inline JS скриптов https://st.yandex-team.ru/ANTIADB-1032
    :param encryption_step_enabled: флаг - включен или нет EncryptionSteps.inline_js_crypt
    :param script_remove_after_use: включено или нет удаление скриптов из DOM после их работы
    :param expected_content_type: ожидаемый тип контента в ответе заглушки (для javascript шифровать не должны)
    :param useragent: User Agent string
    :param crypted: флаг True/False - ожидается ли, что скрипты будут зашифрованы в проксе
    """
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if encryption_step_enabled:
        new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.inline_js_crypt.value)
        new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = script_remove_after_use

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'

    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, test_config.CRYPT_URL_PREFFIX)

    HEADERS = {'host': 'test.local', SEED_HEADER_NAME: seed, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
               'expected-content-type': expected_content_type}
    if useragent is not None:
        HEADERS.update({'user-agent': useragent})

    # Дополнительные тестовые кейсы с разными доменами - проверяем, что шифрование скриптов работает для ссылок с любых разрешенных доменов, а не только для партнерских
    for host in ['an.yandex.ru', 'test.local', 'ads.adfox.ru', 'yastatic.net', "yastat.net"]:

        crypted_url = crypt_url(binurlprefix, 'http://{}/js_crypt.html'.format(host), key, False, origin='test.local')

        proxied_data = requests.get(crypted_url, headers=HEADERS).text

        if not crypted:
            assert_that(proxied_data, equal_to(html_with_js_original))
        else:
            assert_that(proxied_data, is_not(equal_to(html_with_js_original)))

            # Проверим, что все скрипты пошифровались нужным алгоритмом и присутствуют в верстке
            # для этого выпаршиваем скрипты из оригинальной верстки и шифрованной
            # затем шифруем оригинальные скрипты нашим алгоритмом и ищем среди шифрованной верстки
            original_soup = BeautifulSoup(html_with_js_original, "lxml")
            crypted_soup = BeautifulSoup(proxied_data, "lxml")

            original_scripts_bodies = [script.string for script in original_soup.findAll('script') if script.string is not None]
            crypted_scripts_bodies = [script.string for script in crypted_soup.findAll('script') if script.string is not None and
                                      script.string != '\n' and
                                      (script.attrs.get('type', '').find('text/javascript') >= 0 or script.attrs.get('type') is None)]
            original_scripts = [script for script in original_soup.findAll('script')]
            not_crypted_scripts = [script for script in crypted_soup.findAll('script') if script.string not in crypted_scripts_bodies]

            # Проверяем, что мы не сожрали часть скриптов регулярками
            assert len(original_soup.findAll('script')) == len(crypted_soup.findAll('script'))

            # Захардкодил кол-во скриптов, которые, как я ожидаю, должны быть зашифрованы в тестовом html. Так надежнее.
            assert len(crypted_scripts_bodies) == 4

            for crypted_script in crypted_scripts_bodies:
                assert crypted_script not in original_scripts_bodies
            # Проверяем, что скрипты, не попавшие в crypted_scripts не шифрованы (вдруг мы ошиблись в фильтре при формировании crypted_scripts)
            for not_crypted_script in not_crypted_scripts:
                assert not_crypted_script in original_scripts


expected_crypted_strings = json.loads(resource.find("resources/test_js_crypt/expected_crypted_strings.json"))


@pytest.mark.parametrize('script_remove_after_use', [True, False])
def test_inline_js_crypt_on_ajax_json(stub_server, cryprox_worker_url, set_handler_with_config, get_config, script_remove_after_use):
    """
    Тест на то, что если партнер подгружает по ajax урлам куски html страницы, то они верно находятся, парсятся, декодятся,
    оборачиваются шифрующей функцией и так же аккуратно помещатся обратно в виде куска html кода с экранированными двойными кавычками
    """

    stub_server.set_handler(handler)

    seed = 'my2007'
    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.inline_js_crypt.value)
    new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = script_remove_after_use

    set_handler_with_config(test_config.name, new_test_config)

    content_type = 'application/json'
    proxied_data = requests.get('{}/json_crypt.json'.format(cryprox_worker_url), headers={'host': 'test.local',
                                                                                          SEED_HEADER_NAME: seed,
                                                                                          PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                                                          'expected-content-type': content_type})
    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == content_type
    assert proxied_data.content.count('script') == 5  # остались только внешние оболочки скриптов + один тип (type="text/javascript"), все внутри зашифровано
    assert proxied_data.content.count("eval(decodeURIComponent(FRZA(atob(`") == 2  # скрипты заменились

    matches = re2.findall(r"\(atob\(`([^)']+)`\)", proxied_data.content)
    assert len(matches) == 2  # два зашифрованных скрипта
    for crypt_part in matches:
        assert unquote(decrypt_xor(crypt_part, seed)) in expected_crypted_strings


@pytest.mark.parametrize('request_via_script', [True, False])
@pytest.mark.parametrize('script_remove_after_use', [True, False])
@pytest.mark.parametrize('rtb_json',
                         ['{"rtb": {"html": "", "abuseLink": ""}}', """{"rtb": {"targetUrl": "otkryt\\'_demo_schet"}}"""])
def test_loaded_js_crypt_with_jsonify(stub_server, set_handler_with_config, get_key_and_binurlprefix_from_config, get_config, request_via_script, script_remove_after_use, rtb_json):

    rtb_answer = rtb_json.replace("\\'", "'")

    def handler(**_):
        return {'text': rtb_answer, 'code': 200, 'headers': {'Content-Type': 'application/javascript'}}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    seed = 'my2007'
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, 'http://an.yandex.ru/meta/rtb_auction_sample?test=woah&callback=Ya%5B1524124928839%5D', key, True, origin='test.local')

    new_test_config = test_config.to_dict()
    new_test_config['RTB_AUCTION_VIA_SCRIPT'] = request_via_script
    new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.loaded_js_crypt.value)
    new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = script_remove_after_use
    set_handler_with_config(test_config.name, new_test_config)
    proxied_data = requests.get(crypted_url,
                                headers={'host': 'test.local',
                                         SEED_HEADER_NAME: seed,
                                         PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied_data.status_code == 200
    assert proxied_data.headers['Content-Type'] == 'application/javascript'
    if request_via_script:
        assert_that(proxied_data.text, equal_to(crypt_js_body("Ya[1524124928839]('{}')".format(rtb_json), seed, remove_after_run=script_remove_after_use)))
    else:
        assert_that(proxied_data.text, equal_to(crypt_js_body('({})'.format(rtb_json.replace("\\'", "'")), seed, remove_after_run=script_remove_after_use)))


@pytest.mark.parametrize('partner_content_url_crypted', [True, False])
@pytest.mark.parametrize('script_remove_after_use', [True, False])
@pytest.mark.parametrize('sync', [True, False])
def test_replace_resource_with_xhr(stub_server, cryprox_worker_address, set_handler_with_config, get_config, partner_content_url_crypted, script_remove_after_use, sync):

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    if sync:
        new_test_config['REPLACE_RESOURCE_WITH_XHR_SYNC_RE'] = [r'yandex\.ru/static\.js', r'//yastatic\.net/static\.js', r'//yastatic\.net/\w*?\.css\?check-xhr-working=1']
    else:
        new_test_config['REPLACE_SCRIPT_WITH_XHR_RE'] = [r'yandex\.ru/static\.js', r'//yastatic\.net/static\.js', r'//yastatic\.net/\w*?\.css\?check-xhr-working=1']
    new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = script_remove_after_use

    if not partner_content_url_crypted:
        new_test_config['ENCRYPTION_STEPS'].remove(EncryptionSteps.partner_content_url_crypt.value)

    set_handler_with_config(test_config.name, new_test_config)

    seed = 'my2007'

    key = get_key(test_config.CRYPT_SECRET_KEY, seed)
    binurlprefix = CryptUrlPrefix('http', cryprox_worker_address, seed, test_config.CRYPT_URL_PREFFIX)

    crypted_url = crypt_url(binurlprefix, 'http://test.local/replace_resource_with_xhr.html', key, False, origin='test.local')
    proxied_data = requests.get(crypted_url, headers={'host': 'test.local',
                                                      SEED_HEADER_NAME: seed,
                                                      PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                      'expected-content-type': "text/html",
                                                      'expected-csp': "default-src 'none'; script-src 'nonce-12345678qwerty'; style-src 'nonce-qwerty9876543221'"}).text
    expected = expected_html_with_resource_src_sync if sync else expected_html_with_resource_src
    if partner_content_url_crypted:
        assert_that(proxied_data, equal_to(expected.replace('{SCRIPT_REMOVER}',
                                                            SCRIPT_REMOVER if script_remove_after_use else "")))
    else:
        assert_that(proxied_data, equal_to(crypted_html_with_resource_src))


@pytest.mark.parametrize('add_nonce', [True, False])
@pytest.mark.parametrize('amp', ['&', '&amp;'])
@pytest.mark.parametrize('quote', ['\'', '&#x27;'])
@pytest.mark.parametrize('last_symbol', ['', ';'])
def test_add_nonce(stub_server, get_key_and_binurlprefix_from_config, set_handler_with_config, get_config, add_nonce, amp, quote, last_symbol):
    csp_string_template = "report-to https://csp.yandex.net/csp?from=dest{amp}project=project;" \
                          "default-src {quote}none{quote}; connect-src {quote}self{quote} *.adfox.ru mc.admetrica.ru *.auto.ru auto.ru *.yandex.net *.yandex.ru yandex.ru yastatic.net;" \
                          "script-src {quote}unsafe-eval{quote} {quote}unsafe-inline{quote} *.adfox.ru *.yandex.ru yandex.ru *.yandex.net yastatic.net static.yastatic.net{nonce_src};" \
                          "report-uri https://csp.yandex.net/csp?from=autoru-frontend-desktop{amp}version=201904.16.121024{amp}yandexuid=4192397941554369671" + last_symbol

    response_text_template = """<!DOCTYPE HTML>
    <html>
     <head>
      <meta http-equiv="Content-Security-Policy" content="{csp_string}">
     </head>
     <body>
      <script type="text/javascript" src="//yastatic.ru/static.js"></script>
      <script type="text/javascript"{{nonce}}>window.alert(1);</script>
      <script{{nonce}}>window.alert(1);</script>
      <script>
      </script>
      <script type="text/javascript" nonce="123">window.alert(1);</script>
      <script nonce="123">window.alert(1);</script>
     </body>
    </html>""".format(csp_string=csp_string_template)

    response_headers = {"Cache-Control": "max-age=0, must-revalidate, proxy-revalidate, no-cache, no-store, private",
                        "Date": "Wed, 17 Apr 2019 09:24:24 GMT",
                        "Content-Security-Policy": csp_string_template.format(nonce_src="", amp='&', quote='\''),
                        "Content-Type": "text/html", "Pragma": "no-cache", "Server": "nginx", "X-Ua-Bot": "1",
                        "Expires": "Thu, 01 Jan 1970 00:00:01 GMT", "Strict-Transport-Security": "max-age=31536000",
                        "Set-Cookie": "from=direct; Domain=.auto.ru; Path=/; HttpOnly", "X-Frame-Options": "SAMEORIGIN",
                        "Vary": "Accept-Encoding", "X-Content-Type-Options": "nosniff"}

    def handler(**_):
        return {'text': response_text_template.format(nonce="", nonce_src="", amp=amp, quote=quote), 'code': 200, 'headers': response_headers}

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['ADD_NONCE'] = add_nonce
    set_handler_with_config(test_config.name, new_test_config)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    proxied = requests.get(crypt_url(binurlprefix, 'http://{}/test.html'.format('test.local'), key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    if add_nonce:
        nonce = generate_seed(change_period=test_config.SEED_CHANGE_PERIOD, salt=test_config.version)
        expected_csp_string = csp_string_template.format(nonce_src=" 'nonce-{}'".format(nonce), amp='&', quote='\'')
        expected_body = response_text_template.format(nonce=" nonce=\"{}\"".format(nonce), nonce_src=" 'nonce-{}'".format(nonce), amp='&', quote='\'')
    else:
        expected_csp_string = csp_string_template.format(nonce_src="", amp='&', quote='\'')
        expected_body = response_text_template.format(nonce="", nonce_src="", amp=amp, quote=quote)

    assert_that(proxied.text, equal_to(expected_body))
    assert_that(proxied.headers['Content-Security-Policy'], equal_to(expected_csp_string))
    assert proxied.status_code == 200
