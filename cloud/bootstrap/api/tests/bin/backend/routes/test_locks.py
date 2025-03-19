"""Test routes in <locks> section"""

import datetime
import math
import requests
from typing import Dict, List, Optional

from common import bootstrap_api_req


def _datetimes_equal(dt1: datetime.datetime, dt2: datetime.datetime, allowed_delta: int = 10):
    """Check if specified datetimes differs only a little"""
    assert math.fabs((dt1 - dt2).total_seconds()) < allowed_delta, \
           "<{}> and <{}> differs more than <{}> seconds".format(dt1, dt2, allowed_delta)


def _add_lock(description: str, hosts: List[str], hb_timeout: Optional[int] = 1000, add_hosts: bool = False,
              expected_status: Optional[str] = "success", expected_json: Optional[Dict] = None) -> requests.Response:
    if add_hosts:
        resp = bootstrap_api_req(
            "post", "v1/hosts:batch-add",
            request_data=[{"fqdn": host, "dynamic_config": None, "stand": None} for host in hosts]
        )
        assert resp.json()["status"] == "success"

    request_data = {
        "description": description,
        "hosts": hosts,
        "hb_timeout": hb_timeout,
    }
    return bootstrap_api_req("post", "v1/locks", expected_status=expected_status, expected_json=expected_json,
                             request_data=request_data)


def _setup_simple():
    """Setup simple configuration"""
    resp = bootstrap_api_req("post", "v1/hosts:batch-add", request_data=[
        {"fqdn": "h1.yandex.net", "dynamic_config": None, "stand": None},
        {"fqdn": "h2.yandex.net", "dynamic_config": None, "stand": None},
        {"fqdn": "h3.yandex.net", "dynamic_config": None, "stand": None},
        {"fqdn": "h4.yandex.net", "dynamic_config": None, "stand": None},
        {"fqdn": "h5.yandex.net", "dynamic_config": None, "stand": None},
    ])
    assert resp.json()["status"] == "success"

    _add_lock("Lock1", ["h1.yandex.net", "h2.yandex.net"])
    _add_lock("Lock2", ["h3.yandex.net"], hb_timeout=10000)
    _add_lock("Lock3", ["h4.yandex.net", "h5.yandex.net"])


def test_add_lock_simple(bootstrap_db, bootstrap_api):
    # check try lock when no hosts
    _add_lock("Sample description", ["h1.yandex.net", "h2.yandex.net"], expected_status=None, expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Hosts <h1.yandex.net,h2.yandex.net> do not exist in database",
        "status": "fail"
    })

    # check add valid lock
    resp = _add_lock("Sample description", ["h1.yandex.net", "h2.yandex.net"], hb_timeout=1000, add_hosts=True)
    resp_json = resp.json()
    resp_json["data"].pop("expired_at")
    resp_json["data"]["hosts"].sort()
    expected_resp_json = {
        "code": 201,
        "data": {
            "description": "Sample description",
            "hb_timeout": 1000,
            "hosts": [
                "h1.yandex.net",
                "h2.yandex.net"
            ],
            "id": 1,
            "owner": "noauth_fake_user",
        },
        "error_message": None,
        "status": "success",
    }
    assert resp_json == expected_resp_json

    # try to get lock once again and get <CanNotLock>
    resp = _add_lock("Lock once again", ["h2.yandex.net"], expected_status="fail")
    assert resp.json() == {
        "code": 423,
        "data": None,
        "error_message": ("Host h2.yandex.net is already locked by <noauth_fake_user>: lock description <Sample "
                          "description>"),
        "status": "fail"
    }

    # check db content
    db_lock_info = bootstrap_db.select_one("locks", 1)[:4]
    expected_db_lock_info = (1, "noauth_fake_user", "Sample description", 1000)
    assert db_lock_info == expected_db_lock_info


def test_add_lock_fail_params(bootstrap_api):
    expected_json = {
        "code": 400,
        "data": None,
        "error_message": "BadRequest: Input payload validation failed:\n  hb_timeout: -1 is less than the minimum of 1",
        "status": "fail"
    }

    _add_lock(
        "Bad hb timeout", ["h1.yandex.net", "h2.yandex.net"], hb_timeout=-1, add_hosts=True, expected_status="fail",
        expected_json=expected_json
    )

    expected_json = {
        "code": 400,
        "data": None,
        "error_message": "BadRequest: Input payload validation failed:\n  hosts: [] is too short",
        "status": "fail"
    }

    _add_lock("No hosts", [], expected_status="fail", expected_json=expected_json)


def test_get_locks(bootstrap_api):  # FIXME: add check for db contains data after CLOUD-27660
    # get all locks (no locks in db)
    resp = bootstrap_api_req("get", "v1/locks")
    assert resp.json()["code"] == 200
    assert resp.json()["data"] == []

    _setup_simple()

    # get all locks
    resp = bootstrap_api_req("get", "v1/locks")
    assert {(lock["description"], tuple(sorted(lock["hosts"]))) for lock in resp.json()["data"]} == {
        ("Lock1", ("h1.yandex.net", "h2.yandex.net")),
        ("Lock2", ("h3.yandex.net",)),
        ("Lock3", ("h4.yandex.net", "h5.yandex.net")),
    }

    # get specific lock
    expected_lock = resp.json()["data"][0]
    resp = bootstrap_api_req("get", "v1/locks/{}".format(expected_lock["id"]))
    assert resp.json()["code"] == 200
    assert resp.json()["data"] == expected_lock
    assert resp.json()["data"] == expected_lock

    # get missing lock
    bootstrap_api_req("get", "v1/locks/123456", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Lock <123456> is not found in database",
        "status": "fail"
    })


def test_ping_lock_simple(bootstrap_db, bootstrap_api):
    _setup_simple()

    locks_before = bootstrap_api_req("get", "v1/locks").json()["data"]
    locks_before.sort(key=lambda elem: elem["id"])

    # ping and check for updated expiration time
    resp = bootstrap_api_req("post", "v1/locks/{}/ping".format(locks_before[0]["id"]))
    assert resp.json()["code"] == 200
    resp_expired_at = datetime.datetime.strptime(resp.json()["data"]["expired_at"], "%Y-%m-%d %H:%M:%S")  # FIXME: use constant after CLOUD-27660
    expected_expired_at = datetime.datetime.now() + datetime.timedelta(seconds=1000)
    _datetimes_equal(resp_expired_at, expected_expired_at)

    # check we did not touch other locks
    locks_after = bootstrap_api_req("get", "v1/locks").json()["data"]
    locks_after.sort(key=lambda elem: elem["id"])
    assert locks_before[1:] == locks_after[1:]


def test_ping_lock(bootstrap_db, bootstrap_api):  # FIXME: add check for db contains data after CLOUD-27660
    resp = _add_lock("Lock", ["h1.yandex.net", "h2.yandex.net"], hb_timeout=100, add_hosts=True)
    lock_id = resp.json()["data"]["id"]

    # check for correct expiration time
    resp_expired_at = datetime.datetime.strptime(resp.json()["data"]["expired_at"], "%Y-%m-%d %H:%M:%S")  # FIXME: use constant after CLOUD-27660
    expected_expired_at = datetime.datetime.now() + datetime.timedelta(seconds=100)
    _datetimes_equal(resp_expired_at, expected_expired_at)

    # increase hb_timeout in db
    cursor = bootstrap_db.conn.cursor()
    cursor.execute("UPDATE locks SET hb_timeout = 1000 WHERE id = %s;", (lock_id,))
    bootstrap_db.conn.commit()

    # ping and check for updated expiration time
    resp = bootstrap_api_req("post", f"v1/locks/{lock_id}/ping")
    assert resp.json()["code"] == 200
    resp_expired_at = datetime.datetime.strptime(resp.json()["data"]["expired_at"], "%Y-%m-%d %H:%M:%S")  # FIXME: use constant after CLOUD-27660
    expected_expired_at = datetime.datetime.now() + datetime.timedelta(seconds=1000)
    _datetimes_equal(resp_expired_at, expected_expired_at)

    # set expired_at in the past to remove lock on ping
    cursor = bootstrap_db.conn.cursor()
    cursor.execute("UPDATE locks SET expired_at = %s WHERE id = %s;", (datetime.datetime.now() - datetime.timedelta(seconds=1), lock_id,))
    bootstrap_db.conn.commit()

    # try to ping and expect lock already removed
    resp = bootstrap_api_req("post", f"v1/locks/{lock_id}/ping", expected_json={
        "code": 404,
        "data": None,
        "error_message": f"RecordNotFoundError: Lock <{lock_id}> is not found in database",
        "status": "fail"
    })


def test_delete_lock(bootstrap_api):  # FIXME: add check for db contains data after CLOUD-27660
    resp = _add_lock("Lock", ["h1.yandex.net", "h2.yandex.net"], hb_timeout=100, add_hosts=True)
    lock_id = resp.json()["data"]["id"]

    # check lock is removed successfully
    resp = bootstrap_api_req("delete", f"v1/locks/{lock_id}", expected_json={
        "code": 200,
        "data": None,
        "error_message": None,
        "status": "success"
    })

    # check can not remove unexisting lock
    resp = bootstrap_api_req("delete", f"v1/locks/{lock_id}", expected_json={
        "code": 404,
        "data": None,
        "error_message": f"RecordNotFoundError: Lock <{lock_id}> is not found in database",
        "status": "fail"
    })
