import pytest

from antiadblock.cryprox.cryprox.common.cryptobody import CryptUrlPrefix


@pytest.mark.parametrize('scheme,host,seed,prefix,old_prefix', [
    ('http', 'test.local', 'my2007', '/', '/oldprefix/'),
    ('https', 'yandex.ru', '123456', '/morda/_/', '/'),
    ('http', 'mail.yandex.ru', 'qwerty', '', '')
])
def test_CryptUrlPrefix(scheme, host, seed, prefix, old_prefix):
    """
    Test CryptUrlPrefix class methods
    """

    binurlprefix = CryptUrlPrefix(scheme, host, seed, prefix)

    assert binurlprefix.crypt_prefix() == b'{}://{}{}'.format(scheme, host, prefix)
    assert binurlprefix.crypt_prefix(prefix=old_prefix) == b'{}://{}{}'.format(scheme, host, old_prefix)
    assert binurlprefix.crypt_prefix(scheme='') == b'//{}{}'.format(host, prefix)
    assert binurlprefix.crypt_prefix(scheme='', prefix=old_prefix) == b'//{}{}'.format(host, old_prefix)
