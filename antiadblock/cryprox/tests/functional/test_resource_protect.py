from urlparse import urlparse

import re2
import pytest
import requests

import antiadblock.cryprox.cryprox.common.visibility_protection as v_protect
from antiadblock.cryprox.cryprox.common.tools.misc import parse_nonce_from_headers
from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME, EncryptionSteps
from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url, body_replace, crypt_inline_js, crypt_js_body

HTML_TEMPL = r'''
<html>
<head>
    <style>#ads{{min-height:1200px;}}</style>
{injected_inline_script}{hiding_style}</head>
<body>
    <div class="main{prot_placeholder}">
        <div class="content"></div>
    </div>
    <style>.content {{width:1200px;}}</style>
    <style type="text/css">
        #Wrapper {{width:1200px;}}
    </style>
{fixing_inline_script}</body>
</html>
'''
HTML_RAW = HTML_TEMPL.format(prot_placeholder='', hiding_style='', fixing_inline_script='', injected_inline_script='')
INJECT_INLINE_JS = r'''
if(a===1){
    console.log("this script is injected");
}'''


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_request_xhr_protect(get_config, get_key_and_binurlprefix_from_config, config_name):
    """
    Test to check CSS resource fixing styles are always available
    """
    test_config = get_config(config_name)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    expected = body_replace(v_protect.FIX_CSS, key, {re2.compile(v_protect.PLACEHOLDER): None})

    parsed = urlparse(v_protect.XHR_FIX_STYLE_SRC)
    host = parsed.hostname
    crypted_url = crypt_url(binurlprefix, parsed._replace(scheme='https').geturl(), key, False)
    proxied = requests.get(crypted_url, headers={'host': host, PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    assert proxied == expected


@pytest.mark.parametrize('js_is_crypted', [False, True])
@pytest.mark.parametrize('xhr_protect', [False, True])
@pytest.mark.parametrize('nonce', ['', '1234567qwerTY'])
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_request_script_protect(js_is_crypted, xhr_protect, nonce, config_name, get_config, get_key_and_binurlprefix_from_config, set_handler_with_config):
    """
    Test to check JS resource fixing styles
    """
    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()
    new_test_config['XHR_PROTECT'] = xhr_protect
    new_test_config['VISIBILITY_PROTECTION_CLASS_RE'] = 'not None'
    if js_is_crypted:
        new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.loaded_js_crypt.value)
    set_handler_with_config(config_name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    nonce_css = "a.setAttribute(\"nonce\", \"{}\");".format(nonce) if nonce else ""
    if xhr_protect:
        body = v_protect.FIX_JS_WITH_XHR.format(
            SRC=urlparse(crypt_url(binurlprefix, v_protect.XHR_FIX_STYLE_SRC, key, True, origin='test.local'))._replace(netloc='test.local').geturl(),
            NONCE_2=nonce_css
        )
    else:
        body = v_protect.FIX_JS.format(NONCE_2=nonce_css)

    expected = body_replace(body, key, {re2.compile(v_protect.PLACEHOLDER): None})
    if js_is_crypted:
        expected = crypt_js_body(expected, 'my2007', charset='ascii')

    parsed = urlparse(v_protect.SCRIPT_FIX_STYLE_SRC.format(ARGS="?{}={}".format(v_protect.NONCE_QUERY_ARG_NAME, nonce) if nonce else ""))
    crypted_url = crypt_url(binurlprefix, parsed._replace(scheme='https').geturl(), key, False, origin='test.local')
    proxied = requests.get(crypted_url, headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})

    assert proxied.headers['Content-Type'] == 'application/javascript'
    assert proxied.text == expected


@pytest.mark.parametrize('js_is_crypted', [False, True])
@pytest.mark.parametrize('xhr_protect, loaded_js_protect', [(False, False), (False, True), (True, False), (True, True)])
@pytest.mark.parametrize('headers_csp', ['', "default-src 'none'; script-src 'nonce-12345678qwerty'; style-src 'nonce-qwerty9876543221'"])
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_visibility_protection_insert(headers_csp, xhr_protect, loaded_js_protect, js_is_crypted, config_name,
                                      stub_server, get_config, get_key_and_binurlprefix_from_config, set_handler_with_config):

    def handler(**_):
        return dict(text=HTML_RAW, code=200, headers={'Content-Type': 'text/html', 'Content-Security-Policy': headers_csp})

    stub_server.set_handler(handler)

    test_config = get_config(config_name)
    new_test_config = test_config.to_dict()
    new_test_config['VISIBILITY_PROTECTION_CLASS_RE'] = 'main'
    new_test_config['XHR_PROTECT'] = xhr_protect
    new_test_config['LOADED_JS_PROTECT'] = loaded_js_protect
    new_test_config['REMOVE_SCRIPTS_AFTER_RUN'] = True
    new_test_config['PROXY_URL_RE'] = [r'yastatic\.net/safeframe\-bundles/.*?', r'test\.local/.*?']
    if js_is_crypted:
        new_test_config['ENCRYPTION_STEPS'].append(EncryptionSteps.inline_js_crypt.value)
    set_handler_with_config(config_name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied_partner = requests.get(crypt_url(binurlprefix, 'http://test.local/page', key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    proxied_ads = requests.get(crypt_url(binurlprefix, 'http://yastatic.net/ads', key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    proxied_render_adb_html = requests.get(crypt_url(binurlprefix, 'http://yastatic.net/safeframe-bundles/0.69/1-1-0/render_adb.html', key, False, origin='test.local'),
                                           headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text

    crypted_placeholder = body_replace(v_protect.PLACEHOLDER, key, {re2.compile(v_protect.PLACEHOLDER): None})
    nonce_js, nonce_css = parse_nonce_from_headers({'content-security-policy': headers_csp})
    if loaded_js_protect:
        template = v_protect.LOADED_SCRIPT_TEMPLATE
        fix_url = v_protect.SCRIPT_FIX_STYLE_SRC.format(ARGS="?{}={}".format(v_protect.NONCE_QUERY_ARG_NAME, nonce_css) if nonce_css else "")
    else:
        template = v_protect.XHR_TEMPLATE
        fix_url = v_protect.XHR_FIX_STYLE_SRC
    fixing_script = template.format(
        SRC=urlparse(crypt_url(binurlprefix, fix_url, key, True, origin='test.local'))._replace(netloc='test.local').geturl(),
        NONCE="nonce=\"{}\" ".format(nonce_js) if nonce_js else "",
        NONCE_2="a.setAttribute(\"nonce\", \"{}\");".format(nonce_css) if nonce_css else "",
        SCRIPT_BREAKER_PROTECTION='',
    )
    if js_is_crypted:
        fixing_script = crypt_inline_js(fixing_script, 'my2007', remove_after_run=True)

    if xhr_protect or loaded_js_protect:
        expected_partner_content = HTML_TEMPL.format(
            prot_placeholder=' ' + crypted_placeholder,
            hiding_style='<style {NONCE_2}type="text/css">.{PLACEHOLDER}{{visibility:hidden}}</style>'.format(
                PLACEHOLDER=crypted_placeholder,
                NONCE_2="nonce=\"{}\" ".format(nonce_css) if nonce_css else ""),
            fixing_inline_script=fixing_script, injected_inline_script='',
        )
    else:
        expected_partner_content = HTML_RAW

    assert proxied_partner == expected_partner_content
    assert proxied_ads == HTML_RAW
    assert proxied_render_adb_html == HTML_RAW


@pytest.mark.parametrize('inject_inline_js', [None, INJECT_INLINE_JS])
@pytest.mark.parametrize('xhr_protect, loaded_js_protect', [(False, False), (False, True), (True, False)])
def test_object_current_script_breaker(inject_inline_js, xhr_protect, loaded_js_protect, stub_server, get_config, get_key_and_binurlprefix_from_config, set_handler_with_config):

    def handler(**_):
        return dict(text=HTML_RAW, code=200, headers={'Content-Type': 'text/html'})

    stub_server.set_handler(handler)

    test_config = get_config('test_local')
    new_test_config = test_config.to_dict()
    new_test_config['VISIBILITY_PROTECTION_CLASS_RE'] = 'main'
    new_test_config['XHR_PROTECT'] = xhr_protect
    new_test_config['LOADED_JS_PROTECT'] = loaded_js_protect
    new_test_config['BREAK_OBJECT_CURRENT_SCRIPT'] = True
    new_test_config['INJECT_INLINE_JS'] = inject_inline_js
    expected_inline_js = v_protect.INLINE_JS_TEMPLATE.format(INLINE_JS=(inject_inline_js or '') + v_protect.OBJECT_CURRENT_SCRIPT_BREAKER, NONCE='')

    set_handler_with_config(test_config.name, new_test_config)

    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    proxied_partner = requests.get(crypt_url(binurlprefix, 'http://test.local/page', key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text
    proxied_ads = requests.get(crypt_url(binurlprefix, 'http://yastatic.net/ads', key, False, origin='test.local'), headers={'host': 'test.local', PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]}).text

    if loaded_js_protect or xhr_protect:
        crypted_placeholder = body_replace(v_protect.PLACEHOLDER, key, {re2.compile(v_protect.PLACEHOLDER): None})
        crypted_property_to_check = body_replace(v_protect.PROPERTY_TO_CHECK_DEFINE_PROPERTY, key, {v_protect.crypt_property_to_check_re: None})
        if loaded_js_protect:
            template = v_protect.LOADED_SCRIPT_TEMPLATE
            fix_url = v_protect.SCRIPT_FIX_STYLE_SRC.format(ARGS='')
        else:
            template = v_protect.XHR_TEMPLATE
            fix_url = v_protect.XHR_FIX_STYLE_SRC
        fixing_script = template.format(
            SRC=urlparse(crypt_url(binurlprefix, fix_url, key, True, origin='test.local'))._replace(netloc='test.local').geturl(),
            SCRIPT_BREAKER_PROTECTION=v_protect.SCRIPT_BREAKER_PROTECTION.replace(v_protect.PROPERTY_TO_CHECK_DEFINE_PROPERTY, crypted_property_to_check),
            NONCE='', NONCE_2=''
        )
        expected_partner_content = HTML_TEMPL.format(
            injected_inline_script=expected_inline_js,
            prot_placeholder=' ' + crypted_placeholder,
            hiding_style='<style type="text/css">.{PLACEHOLDER}{{visibility:hidden}}</style>'.format(PLACEHOLDER=crypted_placeholder),
            fixing_inline_script=fixing_script)
    else:
        expected_partner_content = HTML_TEMPL.format(injected_inline_script=expected_inline_js, prot_placeholder='', hiding_style='', fixing_inline_script='')

    assert proxied_partner == expected_partner_content
    assert proxied_ads == HTML_RAW
