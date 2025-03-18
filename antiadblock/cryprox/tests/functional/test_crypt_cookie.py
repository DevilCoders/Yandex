import pytest
import requests

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.encrypter import CookieEncrypter
from yatest.common import source_path

COOKIE_VALUE = "6731871211515776902"


@pytest.mark.parametrize('host',
                         ['partner-webmon.yandex.ru', 'real-ping-mon.yandex.ru', 'cluster-webstatus.yandex.ru',
                          'http-check-headers.yandex.ru'])
def test_crypt_cookie(cryprox_worker_url, host):
    response_text = requests.get(cryprox_worker_url,
                                 headers={'host': host},
                                 cookies={system_config.YAUID_COOKIE_NAME: COOKIE_VALUE}).text

    keys_path = source_path("antiadblock/encrypter/tests/test_keys.txt")
    encrypter = CookieEncrypter(keys_path)
    decrypted = encrypter.decrypt_cookie(str(response_text))

    assert COOKIE_VALUE == decrypted


def test_crypt_empty_cookie(cryprox_worker_url):
    code = requests.get(cryprox_worker_url,
                        headers={'host': 'http-check-headers.yandex.ru'}
                        ).status_code

    # old cookie matcher error code
    assert 422 == code
