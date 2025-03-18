import os

import pytest
import yatest

from antiadblock.cryprox.cryprox.common.tools.ip_utils import get_yandex_nets, CACHE_FILENAME, HARDCODED_YANDEX_NETS

EXPECTED_NETS = map(str, range(10))
RESPONSE_TEXT = '\n'.join(EXPECTED_NETS)
PERSISTENT_VOLUME_PATH = yatest.common.output_path("perm")
CACHE_RESOURCE = os.path.join(PERSISTENT_VOLUME_PATH, CACHE_FILENAME)


@pytest.mark.parametrize('content, code, cache_exists, expected',
                         [(RESPONSE_TEXT, 200, True, EXPECTED_NETS),  # got list of nets from server
                          ('fail', 400, True, EXPECTED_NETS),  # got list of nets from cache
                          ('fail', 400, False, HARDCODED_YANDEX_NETS)])  # got default list
def test_get_nets(stub_server, content, code, cache_exists, expected):

    def handler(**_):
        return {'text': content, 'code': code}

    stub_server.set_handler(handler)
    if not cache_exists:
        os.remove(CACHE_RESOURCE)
    yandex_nets = get_yandex_nets(url=stub_server.url, cache=CACHE_RESOURCE)
    assert yandex_nets == expected
