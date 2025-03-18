from yatest.common import source_path
from antiadblock.encrypter import CookieEncrypter, decrypt_crookie_with_default_keys

COOKIE_VALUE = "6731871211515776902"
ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS = "euo9uyWw7d4iZjI08fRvZutgqjIcQMU3c4uHIgjAx6RlAUm+Y5No9Nts3stu5KxLxQIlvvVt0tfHF/27g8lK/ePOV5A="

ENCRYPTED_BY_DEFAULT_KEY = "kPAqJd7N7Q28bqvvnA8DTGP2tjTMLWOILs8LTJt1fFDjS+U9q0AyvN2uclNYsEt4IgszZF4zolu3D6EKXuPhUBG0CBE="


# can't test encrypt independently since randomization inside encrypt
def test_encrypt_decrypt():
    keys_path = source_path("antiadblock/encrypter/tests/test_keys.txt")
    encrypter = CookieEncrypter(keys_path)
    encrypted = encrypter.encrypt_cookie(COOKIE_VALUE)
    decrypted = encrypter.decrypt_cookie(encrypted)
    assert COOKIE_VALUE == decrypted


def test_decrypt():
    keys_path = source_path("antiadblock/encrypter/tests/test_keys.txt")
    encrypter = CookieEncrypter(keys_path)
    decrypted = encrypter.decrypt_cookie(ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS)
    assert decrypted == COOKIE_VALUE


def test_decrypt_bad():
    keys_path = source_path("antiadblock/encrypter/tests/test_keys.txt")
    encrypter = CookieEncrypter(keys_path)
    decrypted = encrypter.decrypt_cookie(ENCRYPTED_COOKIE_VALUE_BY_TEST_KEYS + "fwefsfd")
    assert decrypted is None


def test_default_decrypt():
    decrypted = decrypt_crookie_with_default_keys(ENCRYPTED_BY_DEFAULT_KEY)
    assert decrypted == COOKIE_VALUE


def test_default_decrypt_bad():
    decrypted = decrypt_crookie_with_default_keys(ENCRYPTED_BY_DEFAULT_KEY + "fwravwaf")
    assert decrypted is None
