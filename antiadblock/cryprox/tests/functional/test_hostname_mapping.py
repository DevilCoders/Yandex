
import pytest
import requests

from antiadblock.cryprox.cryprox.config.system import PARTNER_TOKEN_HEADER_NAME


@pytest.mark.usefixtures("stub_server")
def test_hostname_mapping_param(set_handler_with_config, cryprox_worker_url, stub_port, get_config):

    config = get_config('test_local')
    test_config = config.to_dict()
    host = "this-is-a-fake-address.com:{}".format(stub_port)
    response = requests.get(cryprox_worker_url, headers={'host': host,
                                                         PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})

    assert response.status_code == 599

    test_config['HOSTNAME_MAPPING'] = {"this-is-a-fake-address.com": "localhost"}

    set_handler_with_config(config.name, test_config)

    response = requests.get(cryprox_worker_url, headers={'host': host,
                                                         PARTNER_TOKEN_HEADER_NAME: config.PARTNER_TOKENS[0]})

    assert response.status_code == 200
