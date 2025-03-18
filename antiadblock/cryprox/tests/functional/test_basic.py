import pytest
import requests
from urlparse import urljoin

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_fetch(stub_server, cryprox_worker_url, get_config, config_name):
    """
    Test only fetch content, without 'bodycrypt' and 'bodyreplace'
    """

    original_data = requests.get(urljoin(stub_server.url, 'test.bin')).text
    response = requests.get(urljoin(cryprox_worker_url, 'test.bin'),
                            headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: get_config(config_name).PARTNER_TOKENS[0]})
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == config_name.split('::', 1)[0]
    assert response.text == original_data


@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_fetch_cry_link(stub_server, get_key_and_binurlprefix_from_config, get_config, config_name):
    """
    Test only link decryption and fetch content, without 'bodycrypt' and 'bodyreplace'
    """
    from antiadblock.cryprox.cryprox.common.cryptobody import crypt_url

    test_config = get_config(config_name)
    key, binurlprefix = get_key_and_binurlprefix_from_config(test_config)
    crypted_link = crypt_url(binurlprefix, 'http://{}/test.bin'.format(TEST_LOCAL_HOST), key, enable_trailing_slash=True)

    original_data = requests.get(urljoin(stub_server.url, 'test.bin')).text
    response = requests.get(crypted_link,
                            headers={"host": TEST_LOCAL_HOST, system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0]})
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == config_name.split('::', 1)[0]
    assert response.text == original_data
