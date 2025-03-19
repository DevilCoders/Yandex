"""Test routes in <salt-roles> section"""

import requests
from typing import Dict, Optional

from common import bootstrap_api_req


_SALT_ROLES = ["common", "compute", "head1", "head2"]


def _add_salt_role(name: str, expected_status: Optional[str] = "success", expected_json: Optional[Dict] = None) \
        -> requests.Response:
    request_data = {
        "name": name,
    }
    return bootstrap_api_req("post", "v1/salt-roles", expected_status=expected_status, expected_json=expected_json,
                             request_data=request_data)


def _setup_simple():
    for salt_role_name in _SALT_ROLES:
        _add_salt_role(salt_role_name)


def test_add_salt_role(bootstrap_db, bootstrap_api):
    # check add valid salt_roles
    resp = _add_salt_role("salt_role1", None)
    assert resp.json()["data"] == {"id": 1, "name": "salt_role1"}
    resp = _add_salt_role("salt_role2")
    assert resp.json()["data"] == {"id": 2, "name": "salt_role2"}
    assert bootstrap_db.select_by_condition("SELECT * FROM salt_roles ORDER BY id", ()) == [
        (1, "salt_role1"),
        (2, "salt_role2"),
    ]

    # try add existing salt_role
    resp = _add_salt_role("salt_role1", None, expected_json={
        "code": 409,
        "data": None,
        "error_message": "RecordAlreadyInDbError: Salt role <salt_role1> is already in database",
        "status": "fail",
    })


def test_get_salt_roles(bootstrap_api, bootstrap_db):
    # empty database
    bootstrap_api_req("get", "v1/salt-roles", expected_data=[])

    # database with data
    _setup_simple()
    bootstrap_api_req("get", "v1/salt-roles", expected_data=[
        {"id": 1, "name": "common"},
        {"id": 2, "name": "compute"},
        {"id": 3, "name": "head1"},
        {"id": 4, "name": "head2"},
    ])

    # get salt_roles by one
    bootstrap_api_req("get", "v1/salt-roles/common", expected_data={
        "id": 1, "name": "common",
    })
    bootstrap_api_req("get", "v1/salt-roles/head2", expected_data={
        "id": 4, "name": "head2",
    })

    # get unexisting salt_role
    bootstrap_api_req("get", "v1/salt-roles/unexisting-salt-role", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Salt role <unexisting-salt-role> is not found in database",
        "status": "fail",
    })


def test_delete_salt_role(bootstrap_api, bootstrap_db):
    _setup_simple()

    # delete normal
    bootstrap_api_req("delete", "v1/salt-roles/compute")
    bootstrap_api_req("delete", "v1/salt-roles/head2")

    # delete unexisting
    bootstrap_api_req("delete", "v1/salt-roles/unexisting-salt-role", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Salt role <unexisting-salt-role> is not found in database",
        "status": "fail",
    })

    assert bootstrap_db.select_by_condition("SELECT * FROM salt_roles ORDER BY id", ()) == [
        (1, "common"),
        (3, "head1"),
    ]
