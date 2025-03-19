import os
import requests


def test_oauth():
    port = os.environ['MDB_PASSPORT_RECIPE_PORT']
    resp = requests.get(
        f'http://localhost:{port}/blackbox',
        params={
            'method': 'oauth',
            'oauth_token': '11111111-1111-1111-1111-111111111111',
            'userip': '127.0.0.1',
        },
    )
    assert resp.status_code == 200
