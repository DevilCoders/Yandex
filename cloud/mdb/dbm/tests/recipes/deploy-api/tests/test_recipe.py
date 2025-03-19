import os
import requests


def test_ping():
    port = os.environ['MDB_DEPLOY_API_RECIPE_PORT']
    resp = requests.get(
        f'http://localhost:{port}/ping',
    )
    assert resp.status_code == 200
