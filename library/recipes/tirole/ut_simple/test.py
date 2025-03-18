import os
import os.path
import requests

import tvmauth


TIROLE_PORT_FILE = "tirole.port"
TVMAPI_PORT_FILE = "tvmapi.port"


def _get_port(filename):
    assert os.path.isfile(filename)

    with open(filename) as f:
        return int(f.read())


def _check_revision(tirole_port, tvmapi_port, self_tvm_id, self_secret, slug, expected_revision):
    client1 = tvmauth.TvmClient(
        tvmauth.TvmApiClientSettings(
            self_tvm_id=self_tvm_id,
            self_secret=self_secret,
            disk_cache_dir="./",
            fetch_roles_for_idm_system_slug=slug,
            enable_service_ticket_checking=True,
            localhost_port=tvmapi_port,
            tirole_host='http://localhost',
            tirole_port=tirole_port,
            tirole_tvmid=1000001,
        )
    )
    assert client1.status == tvmauth.TvmClientStatus.Ok

    assert expected_revision == client1.get_roles().meta['revision']

    client1.stop()


def test_tirole():
    tirole_port = _get_port(TIROLE_PORT_FILE)
    tvmapi_port = _get_port(TVMAPI_PORT_FILE)

    r = requests.get("http://localhost:%d/ping" % tirole_port)
    assert r.status_code == 200, r.text

    _check_revision(
        tirole_port=tirole_port,
        tvmapi_port=tvmapi_port,
        self_tvm_id=1000501,
        self_secret='bAicxJVa5uVY7MjDlapthw',
        slug='some_slug_1',
        expected_revision='some_revision_1',
    )

    _check_revision(
        tirole_port=tirole_port,
        tvmapi_port=tvmapi_port,
        self_tvm_id=1000502,
        self_secret='e5kL0vM3nP-nPf-388Hi6Q',
        slug='some_slug_2',
        expected_revision='some_revision_2',
    )
    _check_revision(
        tirole_port=tirole_port,
        tvmapi_port=tvmapi_port,
        self_tvm_id=1000503,
        self_secret='S3TyTYVqjlbsflVEwxj33w',
        slug='some_slug_2',
        expected_revision='some_revision_2',
    )
