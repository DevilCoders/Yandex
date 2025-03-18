# -*- coding: utf8 -*-

from urlparse import urljoin

import pytest
import requests

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.cryprox.common.tools.misc import ConfigName


@pytest.mark.parametrize('cookie', ['', 'olololo', None])  # None = default value 'bltsr'
@pytest.mark.parametrize('config_name', ('test_local', 'test_local_2::active::None::None'))
def test_cookie_of_the_day_handler(cryprox_worker_address, get_config, cookie, set_handler_with_config, config_name):
    test_config = get_config(config_name)
    if cookie is not None:
        new_test_config = test_config.to_dict()
        new_test_config['CURRENT_COOKIE'] = cookie
        set_handler_with_config(config_name, new_test_config)
    response = requests.get(urljoin('http://' + cryprox_worker_address, '/cookie_of_the_day'),
                            headers={system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0], "host": system_config.DETECT_LIB_HOST})
    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == ConfigName(config_name).service_id
    assert response.text == (cookie if cookie is not None else 'bltsr')
