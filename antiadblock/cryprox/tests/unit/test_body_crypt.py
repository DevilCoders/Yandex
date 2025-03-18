
import re2
from urlparse import urljoin

from antiadblock.cryprox.cryprox.common.tools.regexp import re_expand, re_expand_relative_urls, re_completeurl
from antiadblock.cryprox.cryprox.config import bk as bk_config
from antiadblock.cryprox.cryprox.common.cryptobody import body_crypt, crypt_url, CryptUrlPrefix


def test_body_crypt(get_test_key_and_binurlprefix):
    crypt_list = [
        r'(?:an|yabs)\.yandex\.ru/(?:system|resource)/.*?',
        r'awaps(?:\-v6)?\.yandex\.(?:ru|net)/.*?',
        r'(?:st\.)?yandexadexchange\.net/.*?',
        r'yastatic\.net/.*?']
    test_urls = (
        '//an.yandex.ru/resource/banner.gif',
        'http://awaps-v6.yandex.net/banner.gif',
        'https://st.yandexadexchange.net/banner.gif',
        '//yastatic.net/context.js',
        '//an.yandex.ru/system/context.js',
        '//yabs.yandex.ru/system/context.js')
    string_template = '"{}"\n\'{}\'\n![CDATA[{}]]\n&quot;{}&quot;({})\n&quot;{}\\&quot;'

    key, binurlprefix = get_test_key_and_binurlprefix()

    raw_text = string_template.format(*test_urls)
    expected_crypted_text = string_template.format(*(crypt_url(binurlprefix, url, key, False) for url in test_urls))

    crypted_text = body_crypt(raw_text, binurlprefix, key, re2.compile(re_expand(crypt_list)), False)
    assert expected_crypted_text == crypted_text
    assert raw_text != crypted_text


def test_body_crypt_without_dict(get_test_key_and_binurlprefix):
    key, binurlprefix = get_test_key_and_binurlprefix()
    crypted_text = body_crypt('string', binurlprefix, key, re2.compile(re_expand([])), False)
    assert 'string' == crypted_text


def test_doublecrypt_protect(get_test_key_and_binurlprefix):
    """
    Test protection from double crypting (protection locates here - cryprox.common.cryptobody.body_crypt)
    """

    key, binurlprefix = get_test_key_and_binurlprefix(host='localhost')

    img_src = 'http://an.yandex.ru/resource/0/c1/tVK-Oiz0m0j0DyMciZzbM9pytDspCC7Rn1SA+BgxLFDAK5ONds0UnxB--BFzX_tulKWw0C9iOH9Y6sWgesHWgd' \
              'HBNuT+yjuZVMHhttNjVcKsKU7C8FyHEi4Yb9m_thIRIEP9Fi9qY45mhtOVFL+xPjS6ywkGv9xogoB+hegeZJoJv26YqFogHozr2_txpZTnnM9t1uE0MU5RGF' \
              'iMyaZKZ4yikeYLrfyGmGtb4kSh7YQpDGDDj9z7xOw_tJwEVdQiu7pFxj-j364jeNgkX+ZYwMdfBUBOpCm9xs5ZSDEn1Yos7KNbMkJVu_t5FtB1TVox0OLKN5' \
              '3d5ZYcjekWCsJ9uaT+89JnJ5EunUPE-Tbd0vv2Z6P8m3A_tc0Ysw9dtH4XfS1gAWBAGE55hERuW+3ZlIweCXBbaTgMZb3Lia9QChxi9my0Y_FQk2SfzQA_A_' \
              '.htm/data/img/d873c021f740ddcccc0c91aea101fba1.jpg'

    test_body = """
    <iframe>
        <div class="something">
            <img src="http://yandex.ru/some-pic.jpg" />
            <img src="{img_src}" />
            <script src="https://yastatic.net/context.js" />
        </div>
    </iframe>
    """.format(img_src=img_src)
    crypt_url_re = re2.compile(re_expand(bk_config.CRYPT_URL_RE))

    crypted_body = body_crypt(test_body, binurlprefix, key, crypt_url_re, False)
    double_crypted_body = body_crypt(crypted_body, binurlprefix, key, crypt_url_re, False)

    assert crypted_body.find(img_src) == -1
    assert crypted_body != test_body and crypted_body == double_crypted_body


def test_double_encryption_with_different_binurlprefixes_protection(get_test_key_and_binurlprefix):
    original_url = 'http://yastatic.net/morda/_/ololo1234/abrashvabra544'
    original_text = 'href="{}"'.format(original_url)

    same_binurlprefix = CryptUrlPrefix(scheme='http', host='yastatic.net', seed='abrash', prefix='/morda/_/')
    other_binurlprefix = CryptUrlPrefix(scheme='http', host='yandex.com', seed='my2007', prefix='/get/')

    key, binurlprefix = get_test_key_and_binurlprefix()

    crypted_same_binurlprefix = body_crypt(original_text, same_binurlprefix, key, re2.compile(re_expand([r'yastatic\.net/.*?'])), False)
    crypted_other_binurlprefix = body_crypt(original_text, other_binurlprefix, key, re2.compile(re_expand([r'yastatic\.net/.*?'])), False)

    assert crypted_same_binurlprefix == original_text
    assert crypted_other_binurlprefix == 'href="{}"'.format(crypt_url(other_binurlprefix, original_url, key, False))


def test_body_crypt_relative_urls(get_test_key_and_binurlprefix):
    crypt_list = [
        r'(?:\.{0,2}/)*relative_1/.*?',
        r'/relative_2/.*?']
    test_urls = (
        '/relative_1/path/to/file.ext',
        './relative_1/path/to/file.ext',
        '/relative_2/path/to/file.ext',
        '../relative_1/path/to/file.ext',
        '../../relative_1/path/to/file.ext')
    string_template = '"{}"\n\'{}\'\n({})\n&quot;{}&quot;\n&quot;{}\\&quot;'

    key, binurlprefix = get_test_key_and_binurlprefix()

    base_url = 'http://base.url/path/'
    raw_text = string_template.format(*test_urls)
    expected_crypted_text = string_template.format(*map(lambda url: crypt_url(binurlprefix, urljoin(base_url, url), key, False), test_urls))
    crypted_text = body_crypt(raw_text, binurlprefix, key, re2.compile(re_expand_relative_urls(crypt_list)), False, base_url)
    assert expected_crypted_text == crypted_text
    assert raw_text != crypted_text


def test_randomly_body_crypt_image_urls(get_test_key_and_binurlprefix):
    crypt_list = [r'ya\.ru/(?:pictures|funny_gifs)/.*?']
    test_urls = (
        '//ya.ru/pictures/banner.gif',
        'http://ya.ru/funny_gifs/funny_gif.gif',
        'https://ya.ru/pictures/picture.png',
    )
    string_template = '"{}"\n\'{}\'\n![CDATA[{}]]'

    key, binurlprefix = get_test_key_and_binurlprefix()

    raw_text = string_template.format(*test_urls)
    fully_crypted_text = string_template.format(*(crypt_url(binurlprefix, url, key, False) for url in test_urls))

    crypted_text_default_ratio = body_crypt(raw_text, binurlprefix, key, re2.compile(re_expand(crypt_list)), False)
    crypted_text_zero_ratio = body_crypt(
        raw_text, binurlprefix, key, re2.compile(re_expand(crypt_list)), False,
        image_urls_crypting_ratio=0, partner_url_re_match=re2.compile('^' + re_completeurl(crypt_list) + '$', re2.IGNORECASE),
    )
    assert fully_crypted_text == crypted_text_default_ratio
    assert raw_text == crypted_text_zero_ratio
