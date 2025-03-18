import re2
import pytest

from antiadblock.cryprox.cryprox.config import bk as bk_config
from antiadblock.cryprox.cryprox.common.cryptobody import body_crypt, crypt_url
from antiadblock.cryprox.cryprox.common.tools.regexp import re_expand


@pytest.mark.parametrize('trailed', (True, False))
def test_trailing_param(trailed, get_test_key_and_binurlprefix):
    link_to_crypt = '<img src="//an.yandex.ru/resource/banner.gif"/>'
    key, binurlprefix = get_test_key_and_binurlprefix()
    crypt_url_re = re2.compile(re_expand(bk_config.CRYPT_URL_RE))

    result = body_crypt(link_to_crypt, binurlprefix, key, crypt_url_re, trailed)
    assert result != link_to_crypt
    assert result.endswith('/"/>') is trailed


@pytest.mark.parametrize('url, enable_trailing_slash, expect_slash', [
    ('//yandex.ru/path', False, False),
    ('//yandex.ru/path/', False, True),
    ('//yandex.ru/path', True, True),
    ('//yandex.ru/path/', True, True),
])
def test_crypt_url_trailing_slash_logic(url, enable_trailing_slash, expect_slash, get_test_key_and_binurlprefix):
    key, binurlprefix = get_test_key_and_binurlprefix()

    result = crypt_url(binurlprefix, url, key, enable_trailing_slash)
    assert result.endswith('/') == expect_slash
