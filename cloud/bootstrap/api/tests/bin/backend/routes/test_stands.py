"""Test routes in <stands> section"""

import requests
from typing import Dict, Optional

from common import bootstrap_api_req


_STANDS = [
    "stand1",
    "stand2",
    "stand3",
]

_HOSTS_DATA = [
    {"fqdn": "h1.yandex.net", "dynamic_config": None, "stand": "stand1"},
    {"fqdn": "h2.yandex.net", "dynamic_config": None, "stand": "stand3"},
    {"fqdn": "h3.yandex.net", "dynamic_config": None, "stand": "stand2"},
    {"fqdn": "h4.yandex.net", "dynamic_config": None, "stand": None},
]

_SVMS_DATA = [
    {"fqdn": "svm1.yandex.net", "dynamic_config": None, "stand": None},
    {"fqdn": "svm2.yandex.net", "dynamic_config": None, "stand": None},
    {"fqdn": "svm3.yandex.net", "dynamic_config": {"placement": "h1.yandex.net"}, "stand": "stand1"},
    {"fqdn": "svm4.yandex.net", "dynamic_config": None, "stand": "stand1"},
]


def _add_stand(name: str, expected_status: Optional[str] = "success",
               expected_json: Optional[Dict] = None) -> requests.Response:
    request_data = {
        "name": name,
    }
    return bootstrap_api_req("post", "v1/stands", expected_status=expected_status, expected_json=expected_json,
                             request_data=request_data)


def _setup_simple():
    for stand_name in _STANDS:
        _add_stand(stand_name)
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=_HOSTS_DATA)
    bootstrap_api_req("post", "v1/svms:batch-add", request_data=_SVMS_DATA)


def test_add_stand(bootstrap_db, bootstrap_api):
    # check add valid stands
    resp = _add_stand("stand1", None)
    assert resp.json()["data"] == {"id": 1, "name": "stand1"}
    resp = _add_stand("stand2")
    assert resp.json()["data"] == {"id": 2, "name": "stand2"}
    assert bootstrap_db.select_by_condition("SELECT * FROM stands ORDER BY id", ()) == [
        (1, "stand1"),
        (2, "stand2"),
    ]

    # try add existing stand
    resp = _add_stand("stand1", None, expected_json={
        "code": 500,
        "data": None,
        "error_message": ("UniqueViolation: duplicate key value violates unique constraint \"stands_uq_name\"\nDETAIL: "
                          " Key (name)=(stand1) already exists.\n"),
        "status": "error"
    })


def test_get_stands(bootstrap_api, bootstrap_db):
    # empty database
    bootstrap_api_req("get", "v1/stands", expected_data=[])

    # database with data
    _setup_simple()
    bootstrap_api_req("get", "v1/stands", expected_data=[
        {"id": 1, "name": "stand1"},
        {"id": 2, "name": "stand2"},
        {"id": 3, "name": "stand3"},
    ])

    # get stands by one
    bootstrap_api_req("get", "v1/stands/stand1", expected_data={
        "id": 1, "name": "stand1"
    })
    bootstrap_api_req("get", "v1/stands/stand3", expected_data={
        "id": 3, "name": "stand3"
    })

    # get unexisting stand
    bootstrap_api_req("get", "v1/stands/unexisting-stand", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })


def test_delete_stand(bootstrap_api, bootstrap_db):
    _setup_simple()

    # delete normal
    bootstrap_api_req("delete", "v1/stands/stand1")
    bootstrap_api_req("delete", "v1/stands/stand3")

    # delete unexisting
    bootstrap_api_req("delete", "v1/stands/unexisting-stand", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })

    assert bootstrap_db.select_by_condition("SELECT * FROM stands ORDER BY id", ()) == [
        (2, "stand2"),
    ]


def test_get_stand_instances(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check unexisging
    bootstrap_api_req("get", "v1/stands/unexisting-stand/hosts", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail",
    })
    bootstrap_api_req("get", "v1/stands/unexisting-stand/svms", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail",
    })

    # stand1
    bootstrap_api_req("get", "v1/stands/stand1/hosts", expected_data=[
        {"dynamic_config": None, "fqdn": "h1.yandex.net", "id": 1, "stand": "stand1"},
    ])
    bootstrap_api_req("get", "v1/stands/stand1/svms", expected_data=[
        {
            "dynamic_config": {
                "ipv4": None,
                "ipv6": None,
                "placement": "h1.yandex.net",
            },
            "fqdn": "svm3.yandex.net",
            "id": 7,
            "stand": "stand1"
        },
        {
            "dynamic_config": None,
            "fqdn": "svm4.yandex.net",
            "id": 8,
            "stand": "stand1"
        }
    ])

    # stand2
    bootstrap_api_req("get", "v1/stands/stand2/hosts", expected_data=[
        {"dynamic_config": None, "fqdn": "h3.yandex.net", "id": 3, "stand": "stand2"},
    ])
    bootstrap_api_req("get", "v1/stands/stand2/svms", expected_data=[])

    # stand3
    bootstrap_api_req("get", "v1/stands/stand3/hosts", expected_data=[
        {"dynamic_config": None, "fqdn": "h2.yandex.net", "id": 2, "stand": "stand3"},
    ])
    bootstrap_api_req("get", "v1/stands/stand3/svms", expected_data=[])


def test_get_stand_hosts_and_svms(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check unexisting
    bootstrap_api_req("get", "v1/stands/unexisting-stand/hosts-and-svms-with-version", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail",
    })

    # stand1
    bootstrap_api_req("get", "v1/stands/stand1/hosts-and-svms-with-version", expected_data={
        "cluster_configs_version": 2,
        "hosts": [
            {
                "dynamic_config": None,
                "fqdn": "h1.yandex.net",
                "id": 1,
                "stand": "stand1"
            }
        ],
        "svms": [
            {
                "dynamic_config": {
                    "ipv4": None,
                    "ipv6": None,
                    "placement": "h1.yandex.net",
                },
                "fqdn": "svm3.yandex.net",
                "id": 7,
                "stand": "stand1"
            },
            {
                "dynamic_config": None,
                "fqdn": "svm4.yandex.net",
                "id": 8,
                "stand": "stand1"
            }
        ]
    })

    # stand2
    bootstrap_api_req("get", "v1/stands/stand2/hosts-and-svms-with-version", expected_data={
        "cluster_configs_version": 2,
        "hosts": [
            {
                "dynamic_config": None,
                "fqdn": "h3.yandex.net",
                "id": 3,
                "stand": "stand2"
            }
        ],
        "svms": []
    })

    # update and check again
    host1_dynamic_config = {
        "ipv4": None,
        "ipv6": {
            "addr": "fc00::1",
            "mask": None,
        }
    }
    bootstrap_api_req(
        "patch", "v1/hosts/h3.yandex.net",
        request_data={
            "dynamic_config": host1_dynamic_config,
        },
        expected_data={
            "dynamic_config": host1_dynamic_config,
            "id": 3,
            "fqdn": "h3.yandex.net",
            "stand": "stand2"
        },
    )
    # stand2
    bootstrap_api_req("get", "v1/stands/stand2/hosts-and-svms-with-version", expected_data={
        "cluster_configs_version": 3,
        "hosts": [
            {
                "dynamic_config": host1_dynamic_config,
                "fqdn": "h3.yandex.net",
                "id": 3,
                "stand": "stand2"
            }
        ],
        "svms": []
    })
