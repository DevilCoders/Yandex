
import pytest

from antiadblock.libs.decrypt_url.lib import get_key
from antiadblock.cryprox.cryprox.common.cry import generate_seed
from antiadblock.cryprox.cryprox.common.cryptobody import CryptUrlPrefix


TEST_CRYPT_SECRET_KEY = 'duoYujaikieng9airah4Aexai4yek4qu'
# Not real port here, in unit tests. For real port one should use functional fixture containers_context
FAKE_WORKER_PORT = 8081


@pytest.fixture(scope="function")
def get_test_key_and_binurlprefix():

    def impl(seed='my2007', host='test.local', crypt_url_prefix='/'):
        crypted_host = host or 'localhost:{}'.format(FAKE_WORKER_PORT)
        seed = seed or generate_seed()
        key = get_key(TEST_CRYPT_SECRET_KEY, seed)
        binurlprefix = CryptUrlPrefix('http', crypted_host, seed, crypt_url_prefix)
        return key, binurlprefix

    return impl
