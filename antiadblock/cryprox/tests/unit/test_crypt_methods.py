import pytest

from antiadblock.libs.decrypt_url.lib import decrypt_xor, decrypt_base64
import antiadblock.cryprox.cryprox.common.cry as cry
from antiadblock.cryprox.cryprox.common.tools.crypt_cookie_marker import get_crypted_cookie_value, decrypt, hash_string

from .conftest import TEST_CRYPT_SECRET_KEY


links_to_crypt = ('//avatars.mds.yandex.net/get-auto/70747/catalog.20681959.775214863236386480/cattouchret',
                  '//yastatic.net/iconostasis/_/UPXtFygA_Hhj_j-wXrS6RNLXyvM.png',
                  'https://awaps.yandex.net/YkZbKgdWgD.png')


@pytest.mark.parametrize('link', links_to_crypt)
def test_base64_crypt_methods(link):
    """
    Simple test to crypt/decrypt urls using base64 crypting
    """

    base64_encoded_link = cry.encrypt_base64(link)
    base64_decoded_link = decrypt_base64(base64_encoded_link)

    assert link != base64_encoded_link and link == base64_decoded_link
    assert '.net' not in base64_encoded_link


@pytest.mark.parametrize('link', links_to_crypt)
def test_xor_crypt_methods(link):
    """
    Simple test to crypt/decrypt urls using xor crypting
    """

    crypt_key = 'SomeCryptKey4Testing'
    xor_encoded_link = cry.encrypt_xor(link, crypt_key)
    xor_decoded_link = decrypt_xor(xor_encoded_link, crypt_key)

    assert link != xor_encoded_link and link == xor_decoded_link
    assert '.net' not in xor_encoded_link


def test_get_crypted_cookie():
    """
    Simple test to crypt/decrypt cookie value
    """
    ip = "192.168.0.13"
    user_agent = "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0; chromeframe/12.0.742.112)"
    accept_language = "ru-ru,ru;q=0.8,en-us;q=0.6,en;q=0.4"
    fake_time = 1548676612
    crypt_value = get_crypted_cookie_value(TEST_CRYPT_SECRET_KEY, ip, user_agent, accept_language, fake_time)

    encrypted = decrypt(crypt_value, TEST_CRYPT_SECRET_KEY).split()

    assert fake_time == int(encrypted[0])
    assert hash_string(ip) == encrypted[1]
    assert hash_string(user_agent) == encrypted[2]
    assert hash_string(accept_language) == encrypted[3]
