# coding=utf-8
from copy import deepcopy

import pytest


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('change_values, pattern, active, expected_count, code',
                         [({}, 'auto', True, 1, 200),
                          ({}, "'PROXY_URL_RE'", True, 1, 200),
                          ({}, '"PROXY_URL_RE"', True, 1, 200),
                          ({}, "cant find this text", True, 0, 200),
                          ({}, "'cant find this text'", True, 0, 200),
                          ({}, 'BAD_KEY: any value', True, 0, 200),
                          ({}, '\\w+\\.)*auto\\.ru', True, 1, 200),
                          # ({}, 'EXTUID_COOKIE_NAMES: dRuId', True, 1, 200),
                          ({}, 'EXTUID_COOKIE_NAMES: any value', True, 0, 200),
                          # ({}, 'RTB_AUCTION_VIA_SCRIPT: FaLse', True, 1, 200),
                          ({}, 'RTB_AUCTION_VIA_SCRIPT: true', True, 0, 200),
                          # ({}, 'IMAGE_URLS_CRYPTING_PROBABILITY: 100', True, 1, 200),
                          ({}, 'IMAGE_URLS_CRYPTING_PROBABILITY: 1000', True, 0, 200),
                          ({}, 'PROXY_URL_RE: \\w+\\.)*auto\\.ru', True, 1, 200),
                          ({}, '  ', True, 1, 400),
                          ({}, '%20%20', True, 0, 200),
                          ({}, 'ж', True, 0, 400),
                          ({}, '\xd0\xb6', True, 0, 400),
                          ({}, 'ж ', True, 0, 400),
                          ({}, ' ж', True, 0, 400),
                          ({}, 't', True, 0, 400),
                          ({'CRYPT_URL_RE': '/static/.*'}, 'CRYPT_URL_RE: static', False, 1, 200),
                          ({'CRYPT_URL_RE': '/static/.*'}, '\\w+\\.)*auto\\.ru', False, 2, 200),
                          ({'CRYPT_URL_RE': '/static/.*'}, 'PROXY_URL_RE: \\w+\\.)*auto\\.ru', False, 2, 200),
                          ({'CRYPT_URL_RE': '/static/.*'}, 'CRYPT_URL_RE: any value', False, 0, 200),
                          ({'CRYPT_URL_RE': '(?:\\w+\\.)*yastatic\\.net/(?:yandex-video-player-iframe-api-bundles|share2)/.*?'}, '(?:\\w+\\.)*yastatic\\.net/', False, 1, 200),
                          ({"PROXY_URL_RE": "(?:[\\w\\-]*\\.)*a_b\\wbt%o\\.ru'/.*?"}, "PROXY_URL_RE:  *a_b\\wbt%o\\.ru'/.", False, 1, 200),
                          ({"PROXY_URL_RE": "(?:[\\w\\-]*\\.)*a_b\\bb%to\\.ru'/.*?"}, "PROXY_URL_RE:  *a_b\\bb%to\\.ru'/.", False, 1, 200),
                          ({"PROXY_URL_RE": "(?:[\\w\\-]*\\.)*a_b\bb%to\\.ru'/.*?"}, "PROXY_URL_RE:  *a_b\bb%to\\.ru'/.", False, 1, 200),
                          ({"PROXY_URL_RE": "(?:[\\w\\-]*\\.)*acb\\wbwto\\.ru/.*?"}, "PROXY_URL_RE:  *a_b\\wbb%to\\.ru/.", False, 0, 200),
                          ({'PROXY_URL_RE': 'class="(direct_\nd*)'}, 'PROXY_URL_RE: class="(direct_\nd*)', False, 1, 200),
                          ({'CRYPT_URL_RE': '/аб_вgг%д/.*'}, 'CRYPT_URL_RE: б_вgг%', False, 1, 200),
                          ({'CRYPT_URL_RE': '/аб_вgг%д/.*'}, 'CRYPT_URL_RE: б_Вgг%', False, 1, 200),
                          ({'CRYPT_URL_RE': '/аБ_вgг%д/.*'}, 'CRYPT_URL_RE: б_вgг%', False, 1, 200)
                          ])
def test_search(api, session, service, change_values, pattern, active, expected_count, code):

    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    old_config = r.json()["items"][0]

    if len(change_values) > 0:
        config_data = deepcopy(old_config["data"])
        for key, val in change_values.iteritems():
            if key in config_data:
                config_data[key].append(val)
            else:
                config_data[key] = [val]

        r = session.post(api["label"][service_id]["config"],
                         json=dict(data=config_data, data_settings={}, comment="Config 2", parent_id=old_config['id']))
        assert r.status_code == 201

    r = session.get(api["search"], params=dict(pattern=pattern, active=active))

    assert r.status_code == code
    if code == 200:
        assert r.json()["total"] == expected_count


@pytest.mark.usefixtures("transactional")
def test_search_limit_offset(api, session, service):
    service_id = service["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    assert r.json()["total"] == 1
    config = r.json()["items"][0]

    config_data = deepcopy(config["data"])
    config_data["CRYPT_URL_RE"] = [r'/static/.*']

    parent_id = config["id"]
    config_ids = []
    for i in xrange(3):
        r = session.post(api["label"][service_id]["config"], json=dict(data=config_data, data_settings={}, comment="Config {}".format(i), parent_id=parent_id))
        assert r.status_code == 201
        parent_id = r.json()["id"]
        config_ids.append(parent_id)

    r = session.get(api["search"], params=dict(offset=1, limit=2, pattern='tatic', active=False))
    assert r.status_code == 200
    assert r.json()["total"] == 3
    assert len(r.json()["items"]) == 2
    assert r.json()["items"][0]["id"] == config_ids[::-1][1]
    assert r.json()["items"][1]["id"] == config_ids[::-1][2]
