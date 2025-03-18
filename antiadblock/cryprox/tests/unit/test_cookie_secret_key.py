from antiadblock.cryprox.cryprox.common.cryptobody import get_script_key, validate_script_key

from .conftest import TEST_CRYPT_SECRET_KEY


SEED = 'my2007'


def get_fake_now():
    return 1548676612


def test_success_check(get_test_key_and_binurlprefix):
    key, _ = get_test_key_and_binurlprefix(seed=SEED)
    data = get_script_key(key, SEED)
    assert validate_script_key("".join(data), TEST_CRYPT_SECRET_KEY)[0]


def test_fail_check_time(get_test_key_and_binurlprefix):
    key, _ = get_test_key_and_binurlprefix(seed=SEED)
    data = get_script_key(key, SEED, functime=get_fake_now)
    assert not validate_script_key("".join(data), TEST_CRYPT_SECRET_KEY)[0]


def test_fail_check_time_bad_data(get_test_key_and_binurlprefix):
    key, _ = get_test_key_and_binurlprefix(seed=SEED)
    data = get_script_key(key, SEED)
    assert not validate_script_key("".join(data)[:-10], TEST_CRYPT_SECRET_KEY)[0]
