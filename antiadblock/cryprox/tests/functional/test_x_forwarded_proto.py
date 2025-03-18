# -*- coding: utf8 -*-

import pytest
import requests
from urlparse import urljoin

from antiadblock.cryprox.cryprox.config import system as system_config
from antiadblock.cryprox.tests.lib.containers_context import TEST_LOCAL_HOST


@pytest.mark.parametrize('x_forwarded_proto, expected_code',
                         [('http', 200),
                          # ('https', 200), -- этот кейс не проверяем, так как на текущий момент тестовая стаба не поддерживает https
                          ('http://somedomain.ru/', 400),
                          ('https://buglloc.ru/', 400),
                          ('www', 400),
                          (None, 200),
                          ])
def test_xfp_validation(stub_server, cryprox_worker_url, get_config, x_forwarded_proto, expected_code):
    """
    Проверяем валидацию заголовка X-Forwarded-Proto в декораторе fixup_scheme
    """

    proxied_data = requests.get(urljoin(cryprox_worker_url, 'test.bin'),
                                headers={"host": TEST_LOCAL_HOST,
                                         'X-Forwarded-Proto': x_forwarded_proto,
                                         system_config.REQUEST_ID_HEADER_NAME: 'my2007id',
                                         system_config.PARTNER_TOKEN_HEADER_NAME: get_config('test_local').PARTNER_TOKENS[0]})

    assert proxied_data.status_code == expected_code
    if expected_code == 400:
        assert proxied_data.content == 'Bad X-Forwarded-Proto header value\n'
