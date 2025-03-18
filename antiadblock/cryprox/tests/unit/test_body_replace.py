# -*- coding: utf8 -*-

import pytest
import cchardet as chardet

from antiadblock.libs.decrypt_url.lib import get_key
import antiadblock.cryprox.cryprox.common.cry as cry
from antiadblock.cryprox.cryprox.common.cryptobody import body_replace
from antiadblock.cryprox.cryprox.common.tools.regexp import compile_replace_dict
from conftest import TEST_CRYPT_SECRET_KEY


key = get_key(TEST_CRYPT_SECRET_KEY, cry.generate_seed())
replace_prefix = chr(ord(key[0]) % 26 + ord('a'))


def test_replace():
    replace_dict = {
        r'//ads\.adfox\.ru/\d+/(prepareCode|getCode)\?': b'getCodeTest',
        r'system/(context)\.js': b'context_adb',
        r'partner-code-bundles/\d+/loaders/(context)\.js': b'context_adb',
        r'pcode/adfox/(loader)\.js': b'loader_adb',
        r'\"((https?:)?//url\-(?:to|me)\.crypt/.*?)\"': None,  # use crypt for this urls, not replace
    }
    raw_text = '''
        href="//ads.adfox.ru/123/prepareCode?code=trololo"
        href2="//ads.adfox.ru/123/getCode?code=trololo"
        test3="//yandex.ru/system/context.js"
        test4="https://url-me.crypt/with/path/file.ext?with=param-pam-pam&and=fra#gment"
        test5="https://yastatic.net/pcode/adfox/loader.js"
        test6="//yastatic.net/partner-code-bundles/12345/loaders/context.js"
        '''
    expected_crypted_text = '''
        href="//ads.adfox.ru/123/getCodeTest?code=trololo"
        href2="//ads.adfox.ru/123/getCodeTest?code=trololo"
        test3="//yandex.ru/system/context_adb.js"
        test4="{}"
        test5="https://yastatic.net/pcode/adfox/loader_adb.js"
        test6="//yastatic.net/partner-code-bundles/12345/loaders/context_adb.js"
        '''.format(replace_prefix + cry.encrypt_xor('https://url-me.crypt/with/path/file.ext?with=param-pam-pam&and=fra#gment', key).replace('-', 'a'))
    crypted_text = body_replace(raw_text, key, compile_replace_dict(replace_dict))
    assert expected_crypted_text == crypted_text
    assert raw_text != crypted_text


def test_replace_dict_variations():
    replace_dict = {
        r'adv_pos_top': None,
        r'adv_pos_bottom': None,
        r'\badv\b': None,
        r'[\"\'.](misc-bdv)\b': None,
        r'not_found': None,
        r'second_(group)': None,
    }
    raw_text = '''
        <div class="adv adv_pos_top"></div>
        .adv_pos_bottom
        ./../desktop.blocks/misc-bdv/adv_pos/adv_pos_top.css:end
        class="misc-bdv"
        .misc-bdv
        second_group
        '''
    expected_crypted_text = '''
        <div class="{2} {0}"></div>
        .{1}
        ./../desktop.blocks/misc-bdv/adv_pos/{0}.css:end
        class="{3}"
        .{3}
        second_{4}
        '''.format(replace_prefix + cry.encrypt_xor('adv_pos_top', key).replace('-', 'a'),
                   replace_prefix + cry.encrypt_xor('adv_pos_bottom', key).replace('-', 'a'),
                   replace_prefix + cry.encrypt_xor('adv', key).replace('-', 'a'),
                   replace_prefix + cry.encrypt_xor('misc-bdv', key).replace('-', 'a'),
                   replace_prefix + cry.encrypt_xor('group', key).replace('-', 'a'))
    crypted_text = body_replace(raw_text, key, compile_replace_dict(replace_dict))
    assert expected_crypted_text == crypted_text


def test_replace_processor_without_dict():
    crypted_text = body_replace('string', key, {})
    assert 'string' == crypted_text


def test_avoid_forbid_symbols_in_js():
    # With this key adv becomes y - 9XF which is interpreted in js as minus sign
    # So without replacing it'll be invalid key
    key = get_key('xeiqu3noo7rooTheik4ohk4soosi7aeC', 'd2bcbf')
    replace_dict = {
        r'[\"\'.](adv)\b': None,
        r'class ?= ?\"(ring)[\w\- ]*\"': None,
        r'[a-zA-Z0-9\-]*_?(proffit)_?[a-zA-Z0-9\-]*': None,
    }
    raw_js = '''
    <div class="ring hidden-xs">
    proffit_sticky-pause_no
    _isWide: function () {
            for (var t = this.props.adv.image, e = 0; e < t._images.length; ++e) {
                if (t._images[e].isWide === !0) {
                    return !0;
                }
            }
            return !1
        }
    '''
    expected_js = '''
    <div class="y6NjdUQ hidden-xs">
    y6sPcUJcb0w_sticky-pause_no
    _isWide: function () {
            for (var t = this.props.ya9XF.image, e = 0; e < t._images.length; ++e) {
                if (t._images[e].isWide === !0) {
                    return !0;
                }
            }
            return !1
        }
    '''
    crypted_js = body_replace(raw_js, key, compile_replace_dict(replace_dict))
    assert expected_js == crypted_js


@pytest.mark.parametrize('charset',
                         ['cp1251',
                          'utf-8',
                          'koi8-r',
                          'MacCyrillic'])
@pytest.mark.parametrize('replace_dict, comment',
                         [({r'\b(ascii)\[\d+\]': 'still ascii'}, 'both ascii'),
                          ({r'Демократия': 'Авторитаризм'}, 'both utf-8, so all should be good'),
                          ({u'Демократия': u'Авторитаризм'}, 'undefined result, but no crash'),
                          ({'Демократия'.decode('utf-8').encode('cp1251'): 'Авторитаризм'.decode('utf-8').encode(
                              'cp1251')}, 'both cp1251, so all good'),
                          ({'abc': u'Авторитаризм'}, 'undefined result, but still no crash')
                          ])
@pytest.mark.parametrize('body_original',
                         ['ascii[9000]',
                          'Демократия',
                          'abc Демократия'], )
def test_replace_various_charsets(charset, replace_dict, comment, body_original):
    replace_dict_re = compile_replace_dict(replace_dict)
    test_body = body_original.decode('utf-8').encode(charset)
    crypted_body = body_replace(test_body, 'lulz', replace_dict_re)
    body_encoding = chardet.detect(crypted_body)['encoding']
    try:
        crypted_body.decode(body_encoding)
    except ValueError:
        pytest.fail('chardet detect detected wrong encoding: {}, {}, {} '.format(charset, body_original, comment))


@pytest.mark.parametrize('body, replace_dict, seed, result', [
    # smoke
    ('<div class="test" ></div>',
     {"test": None}, "my2007",
     '<div class="oii9p" ></div>'),

    # prefix and value depend on seed
    ('<div class="test" ></div>',
     {"test": None}, "my2006",
     '<div class="i9a8r" ></div>'),

    # no uppercase
    ('#abcdefghijklmopqrstuvwxyz1234567890abcdefghijklmopqrstuvwxyz1234567890abcdefghijklmopqrstuvwxyz1234567890#',
     {r"[^#]+": None},
     "my2006",
     '#iwswympyvu01fn9p5c1kuondi65il3z0z8kovn6qb3vyupft6h2d4g5fw0lql98fpchunr8ph8huco0k5acy5pv40poy7d9y20e8qv1syn#'),

    # with group
    ('<div class="test" ></div>',
     {"class=\"(test)\"": None},
     "my2007",
     '<div class="oii9p" ></div>'),

    # replace
    ('<div class="test" ></div>',
     {"class=\"(test)\"": "OJLOAEW"},
     "my2007",
     '<div class="OJLOAEW" ></div>'),
])
def test_bodycrypt_with_lowercase(get_test_key_and_binurlprefix, body, seed, replace_dict, result):
    key, binurlprefix = get_test_key_and_binurlprefix(seed=seed)
    crypted_text = body_replace(body=body, key=key, replace_dict=compile_replace_dict(replace_dict), crypt_in_lowercase=True)

    assert result == crypted_text
