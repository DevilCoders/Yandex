"""
A test to run against a live instance
"""

import requests


def test_ping(live_app_url):
    # TODO: get rid of the verify=False
    resp = requests.get(live_app_url + '/ping', verify=False)

    assert resp.status_code == 200
    assert resp.content == 'OK'
