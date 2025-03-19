import os
import requests


def test_ping():
    port = os.environ['MDB_DBM_PORT']
    resp = requests.get(
        f'http://localhost:{port}/ping',
    )
    assert resp.status_code == 200


def test_get_metrics():
    port = os.environ['MDB_DBM_METRICS_PORT']
    resp = requests.get(
        f'http://localhost:{port}/metrics',
    )
    assert resp.status_code == 200
