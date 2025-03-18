import re2
import pytest
import requests

from antiadblock.cryprox.cryprox.config.system import EncryptionSteps, InjectInlineJSPosition
from antiadblock.cryprox.cryprox.common.tools.regexp import re_merge
from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, crypt_js_body, body_replace
import antiadblock.cryprox.cryprox.common.visibility_protection as v_protect

HTML_JS_TEMPL = '''
<html>
<head{HEAD_ATTRS}>{HEAD_BEGINNING}
<div> This is script! </div>
{HEAD_END}</head>
<div class="head"></div>
<text>
    !function(e){{var n='<!DOCTYPE html><html><head><meta http-equiv="X-UA-Compatible" content="IE=edge,chrome=1"/></head><body>';}}
</text>
</html>'''

INLINE_SCRIPT_JS = '''
if(`a`.charCodeAt(0)!=97){
Object.defineProperty(cat
    String.prototype,
    `charCodeAt`,
    {value: function(index){
        window._charmap_ = window._charmap_||(window._charmap_=(function(){var a={}; for(var i=0;i<256;i++){a[String.fromCharCode(i)]=i};return a;})());return window._charmap_[this[index]];
    }}
);
} else{
    Object.defineProperty(String.prototype,`charCodeAt`,{configurable:false,writable:false});
}
'''


@pytest.mark.parametrize('head_attrs', ['', ' attr="kek"'])
@pytest.mark.parametrize('inject_position', [InjectInlineJSPosition.HEAD_END, InjectInlineJSPosition.HEAD_BEGINNING, None])
@pytest.mark.parametrize('inline_js', [INLINE_SCRIPT_JS, None])
@pytest.mark.parametrize('nonce_js', ['olololo', ''])
def test_inject_inline_js(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config,
                          nonce_js, inline_js, inject_position, head_attrs):
    def handler(**_):
        return {'text': HTML_JS_TEMPL.format(HEAD_BEGINNING='', HEAD_END='', HEAD_ATTRS=head_attrs), 'code': 200,
                'headers': {'Content-type': 'text/html',
                            'Content-Security-Policy': "script-src 'nonce-{NONCE}'".format(NONCE=nonce_js) if nonce_js else ''}}
    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()

    if inline_js is not None:
        new_test_config['INJECT_INLINE_JS'] = inline_js
        if inject_position is not None:
            new_test_config['INJECT_INLINE_JS_POSITION'] = inject_position
        set_handler_with_config(test_config.name, new_test_config)

        inject_to_head_end = inject_position is None or inject_position == InjectInlineJSPosition.HEAD_END
        injected_script = v_protect.INLINE_JS_TEMPLATE.format(
            INLINE_JS=inline_js,
            NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else ''
        )
        expected = HTML_JS_TEMPL.format(
            HEAD_END=injected_script if inject_to_head_end else '',
            HEAD_BEGINNING='' if inject_to_head_end else injected_script,
            HEAD_ATTRS=head_attrs
        )
    else:
        expected = HTML_JS_TEMPL.format(HEAD_BEGINNING='', HEAD_END='', HEAD_ATTRS=head_attrs)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_url = crypt_url(binurlprefix, 'http://test.local/page', key, False)
    proxied = requests.get(crypted_url, headers={'host': 'test.local',
                                                 system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                                 }).text

    assert expected == proxied


@pytest.mark.parametrize('head_attrs', ['', ' attr="kek"'])
@pytest.mark.parametrize('inject_position', [InjectInlineJSPosition.HEAD_END, InjectInlineJSPosition.HEAD_BEGINNING, None])
@pytest.mark.parametrize('nonce_js', ['olololo', ''])
def test_inject_inline_js_crypt(stub_server, get_config, set_handler_with_config, get_key_and_binurlprefix_from_config,
                                     nonce_js, inject_position, head_attrs):
    def handler(**_):
        return {'text': HTML_JS_TEMPL.format(HEAD_BEGINNING='', HEAD_END='', HEAD_ATTRS=head_attrs), 'code': 200,
                'headers': {'Content-type': 'text/html',
                            'Content-Security-Policy': "script-src 'nonce-{NONCE}'".format(NONCE=nonce_js) if nonce_js else ''}}
    stub_server.set_handler(handler)
    inline_js = INLINE_SCRIPT_JS
    crypt_list = ['_charmap_']

    test_config = get_config('test_local')
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)

    new_test_config = test_config.to_dict()
    new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.inline_js_crypt.value)
    new_test_config['INJECT_INLINE_JS'] = inline_js
    if inject_position is not None:
        new_test_config['INJECT_INLINE_JS_POSITION'] = inject_position
    new_test_config['CRYPT_BODY_RE'] = crypt_list
    inline_js = body_replace(inline_js, key, {re2.compile(re_merge(crypt_list)): None})

    set_handler_with_config(test_config.name, new_test_config)

    seed = "my2007"
    inline_js = crypt_js_body(
        body=inline_js,
        seed=seed
    )

    injected_script = v_protect.INLINE_JS_TEMPLATE.format(
        INLINE_JS=inline_js,
        NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else ''
    )
    inject_to_head_end = inject_position is None or inject_position == InjectInlineJSPosition.HEAD_END
    expected = HTML_JS_TEMPL.format(
        HEAD_END=injected_script if inject_to_head_end else '',
        HEAD_BEGINNING='' if inject_to_head_end else injected_script,
        HEAD_ATTRS=head_attrs
    )

    crypted_url = crypt_url(binurlprefix, 'http://test.local/page', key, False)
    proxied = requests.get(crypted_url, headers={'host': 'test.local',
                                                 system_config.SEED_HEADER_NAME: seed,
                                                 system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]
                                                 }).text

    assert expected == proxied
