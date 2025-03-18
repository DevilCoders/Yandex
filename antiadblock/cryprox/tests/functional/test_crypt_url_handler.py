# -*- coding: utf8 -*-

from urlparse import urljoin

import pytest
import requests

from antiadblock.cryprox.cryprox.config import system as system_config


@pytest.mark.parametrize('url,expected_url,expected_code', [
    ("http://aabturbo.gq/js/wp-embed.min.js",
     "http://aabturbo-gq.naydex.net/SNXt16950/my2007kK/Kdf7P9al80M/5pFUHmL/y6mqpt/8VHOn/sxJZk7B/-w87IoHHNy/bj63_/J2OtkmCe/wvEfqpP/duGVrKSZ/IIk/jQjMfMpG4x2/X69DV/MOiWBD/UpvkJWRhM/j6MjfD3/fnuSc0T/sPio/YA9wfzcs/tLCcwahcw/XNE3nPjP0M",
     200),
    ("http://aabturbo.gq/js/wp-embed.min.js?v=5.1",
     "http://aabturbo-gq.naydex.net/SNXt17822/my2007kK/Kdf7P9al80M/5pFUHmL/y6mqpt/8VHOn/sxJZk7B/-w87IoHHNy/bj63_/J2OtkmCe/wvEfqpP/duGVrKSZ/IIk/jQjMfMpG4x2/X67TM/DYH_fF/lJpi5Gh5u/HPBBjT0/PHoQtw9/g9Ob/XDNzcXg_/sOyQz_xf1/SUE0W_z/BWS81L6g6xo",
     200),
    ("http://aabturbo.gq/js/wp-embed.min.js?v=5.1&param=1",
     "http://aabturbo-gq.naydex.net/SNXt19239/my2007kK/Kdf7P9al80M/5pFUHmL/y6mqpt/8VHOn/sxJZk7B/-w87IoHHNy/bj63_/J2OtkmCe/wvEfqpP/duGVrKSZ/IIk/jQjMfMpG4x2/X67TM/DYH_fF/RAtlIS8uu/SGQU78_/d_db74a/pe6s/SiJTUVgU/isy755h2x/WoIxGPl/AnO_3ar6/2iSdJU/IvHzGdorZQ",
     200),
    ("//aabturbo.gq/js/wp-embed.min.js",
     "https://aabturbo-gq.naydex.net/SNXt17277/my2007kK/Kdf_roahE0M/IxEV2mG/iuC8-J/QUDfj/q090_5x/28-7V7QHNx/eD-w7/JDP_1_VL/RrRYv9d/M_jJ9KPE/aIE/sRTJVcZW_hy/HjoWN/OJz6aX/x9zkJG-qN/bkMSnO0/P_7ScQ7/n_u9/dwlnYngk/6PmDjr9Z3/W8PyDj5BWjN5Q",
     200),
    # Bad requests
    ("http://aabturbo.gq", "", 400),
    ("/js/wp-embed.min.js", "", 400),
])
def test_crypt_url_handler(cryprox_worker_address, get_config, url, expected_url, expected_code):
    test_config = get_config(system_config.AUTOREDIRECT_SERVICE_ID)

    host = "aabturbo.gq"
    response = requests.get(urljoin('http://' + cryprox_worker_address, '/get_crypted_url?url={}&host={}'.format(url.replace('&', '%26'), host)),
                            headers={system_config.PARTNER_TOKEN_HEADER_NAME: test_config.PARTNER_TOKENS[0],
                                     "host": system_config.DETECT_LIB_HOST,
                                     system_config.SEED_HEADER_NAME: "my2007"})

    assert response.headers.get(system_config.NGINX_SERVICE_ID_HEADER) == system_config.AUTOREDIRECT_SERVICE_ID
    assert expected_code == response.status_code
    if expected_code == 200:
        assert expected_url == response.text
