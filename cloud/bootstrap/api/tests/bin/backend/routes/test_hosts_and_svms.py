"""Test routest in <hosts> section"""

from common import bootstrap_api_req


_DYNAMIC_CONFIG_BY_NAME_QUERY_TPL = """
    SELECT hw_props.dynamic_config FROM instances, hw_props
    WHERE
        instances.name = 'h2.yandex.net' AND
        instances.id = hw_props.id;
"""

_STANDS = [
    "stand1",
    "stand2",
    "stand3",
]

_H1_DYNAMIC_CONFIG = None
_H2_DYNAMIC_CONFIG = {
    "ipv4": None,
    "ipv6": {
        "addr": "fc00::1",
        "mask": None,
    }
}
_H3_DYNAMIC_CONFIG = {
    "ipv4": {
        "addr": "10.0.0.1",
        "mask": None,
        "gw": "10.0.0.2"
    },
    "ipv6": None,
}
_H4_DYNAMIC_CONFIG = {
    "ipv6": {
        "addr": "fc00::2",
        "mask": 16
    },
    "ipv4": {
        "addr": "10.0.0.2",
        "mask": 24,
        "gw": "10.0.0.2"
    },
}
_SIMPLE_HOSTS_DATA = [
    {"fqdn": "h1.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": "stand1"},
    {"fqdn": "h2.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand2"},
    {"fqdn": "h3.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand1"},
    {"fqdn": "h4.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": None},
]


_S1_DYNAMIC_CONFIG = None
_S2_DYNAMIC_CONFIG = {
    "ipv4": None,
    "ipv6": {
        "addr": "fc00::5",
        "mask": None,
    },
    "placement": None,
}
_S3_DYNAMIC_CONFIG = {
    "ipv4": {
        "addr": "10.0.0.7",
        "mask": None,
        "gw": "10.0.0.2"
    },
    "ipv6": None,
    "placement": "h1.yandex.net",
}
_S4_DYNAMIC_CONFIG = {
    "ipv6": {
        "addr": "fc00::2",
        "mask": 16
    },
    "ipv4": {
        "addr": "10.0.0.2",
        "mask": 24,
        "gw": "10.0.0.2"
    },
    "placement": "h3.yandex.net"
}
_SIMPLE_SVMS_DATA = [
    {"fqdn": "svm1.yandex.net", "dynamic_config": _S1_DYNAMIC_CONFIG, "stand": "stand3"},
    {"fqdn": "svm2.yandex.net", "dynamic_config": _S2_DYNAMIC_CONFIG, "stand": "stand2"},
    {"fqdn": "svm3.yandex.net", "dynamic_config": _S3_DYNAMIC_CONFIG, "stand": None},
    {"fqdn": "svm4.yandex.net", "dynamic_config": _S4_DYNAMIC_CONFIG, "stand": "stand1"},
]


def _setup_simple():
    """Setup simple configuration"""
    for stand_name in _STANDS:
        bootstrap_api_req("post", "v1/stands", request_data={"name": stand_name})
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=_SIMPLE_HOSTS_DATA)
    bootstrap_api_req("post", "v1/svms:batch-add", request_data=_SIMPLE_SVMS_DATA)


def _ensure_host_configs_version(version: int):
    bootstrap_api_req("get", "v1/host-configs-info/version", expected_data={"version": version})


def test_add_host_simple(bootstrap_api, bootstrap_db):
    _ensure_host_configs_version(0)

    _setup_simple()

    _ensure_host_configs_version(2)

    # check for host already exists
    resp = bootstrap_api_req(
        "post", "v1/hosts", request_data={
            "fqdn": "h2.yandex.net",
            "dynamic_config": None,
            "stand": None,
        }, expected_json={
            "code": 409,
            "data": None,
            "error_message": "RecordAlreadyInDbError: Host <h2.yandex.net> is already in database",
            "status": "fail"
        }
    )

    # check for valid add host
    host_data = {
        "fqdn": "h6.yandex.net",
        "dynamic_config": _H3_DYNAMIC_CONFIG,
        "stand": "stand1",
    }
    resp = bootstrap_api_req("post", "v1/hosts", request_data=host_data)
    assert resp.json()["code"] == 201
    assert resp.json()["status"] == "success"
    assert resp.json()["data"]["dynamic_config"] == _H3_DYNAMIC_CONFIG
    assert resp.json()["data"]["stand"] == "stand1"

    _ensure_host_configs_version(3)


def test_add_svm_simple(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check for svm already exists
    bootstrap_api_req(
        "post", "v1/svms", request_data={
            "fqdn": "svm2.yandex.net",
            "dynamic_config": None,
            "stand": "stand2",
        }, expected_json={
            "code": 409,
            "data": None,
            "error_message": "RecordAlreadyInDbError: Svm <svm2.yandex.net> is already in database",
            "status": "fail",
        }
    )

    # check for valid add svm
    host_data = {
        "fqdn": "svm6.yandex.net",
        "dynamic_config": _S3_DYNAMIC_CONFIG,
        "stand": None,
    }
    bootstrap_api_req("post", "v1/svms", request_data=host_data, expected_json={
        "code": 201,
        "data": {
            "id": 9,
            "fqdn": "svm6.yandex.net",
            "stand": None,
            "dynamic_config": _S3_DYNAMIC_CONFIG,
        },
        "error_message": None,
        "status": "success"
    })

    _ensure_host_configs_version(3)


def test_get_host_simple(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check for non-existing
    bootstrap_api_req("get", "v1/hosts/unexisting.yandex.net", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Hosts <unexisting.yandex.net> do not exist in database",
        "status": "fail"
    })

    # check existing
    resp = bootstrap_api_req("get", "v1/hosts/h2.yandex.net", expected_data={
        "dynamic_config": {
            "ipv4": None,
            "ipv6": {
                "addr": "fc00::1",
                "mask": None
            }
        },
        "id": 2,
        "fqdn": "h2.yandex.net",
        "stand": "stand2",
    })

    # check get all hosts
    resp = bootstrap_api_req("get", "v1/hosts")
    assert {(elem["id"], elem["fqdn"], elem["stand"]) for elem in resp.json()["data"]} == {
        (1, "h1.yandex.net", "stand1"),
        (2, "h2.yandex.net", "stand2"),
        (3, "h3.yandex.net", "stand1"),
        (4, "h4.yandex.net", None),
    }

    _ensure_host_configs_version(2)


def test_update_host_simple(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check not updating <dynamic_config>
    bootstrap_api_req("patch", "v1/hosts/h2.yandex.net", request_data={}, expected_data={
        "dynamic_config": _H2_DYNAMIC_CONFIG,
        "id": 2,
        "fqdn": "h2.yandex.net",
        "stand": "stand2"
    })

    # check update <dynamic_config> to None
    bootstrap_api_req("patch", "v1/hosts/h2.yandex.net", request_data={"dynamic_config": None}, expected_data={
        "dynamic_config": None,
        "id": 2,
        "fqdn": "h2.yandex.net",
        "stand": "stand2",
    })

    # check update <dynamic_config> to something complex
    request_data = {
        "dynamic_config": _H3_DYNAMIC_CONFIG,
        "stand": "stand1",
    }
    bootstrap_api_req("patch", "v1/hosts/h2.yandex.net", request_data=request_data, expected_data={
        "dynamic_config": _H3_DYNAMIC_CONFIG,
        "id": 2,
        "fqdn": "h2.yandex.net",
        "stand": "stand1"
    })

    # check update <stand>
    request_data = {
        "stand": "stand1",
    }
    bootstrap_api_req("patch", "v1/hosts/h4.yandex.net", request_data=request_data, expected_data={
        "dynamic_config": _H4_DYNAMIC_CONFIG,
        "id": 4,
        "fqdn": "h4.yandex.net",
        "stand": "stand1"
    })

    # check update to invalid stand
    request_data = {
        "stand": "unexisting-stand",
    }
    bootstrap_api_req("patch", "v1/hosts/h4.yandex.net", request_data=request_data, expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })

    _ensure_host_configs_version(6)


def test_update_svm_simple(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check not updating <dynamic_config>
    bootstrap_api_req("patch", "v1/svms/svm2.yandex.net", request_data={}, expected_data={
        "dynamic_config": _S2_DYNAMIC_CONFIG,
        "id": 6,
        "fqdn": "svm2.yandex.net",
        "stand": "stand2",
    })

    # check update <placement>
    request_data = {
        "dynamic_config": {
            "placement": "hh.yandex.net",
        }
    }
    bootstrap_api_req("patch", "v1/svms/svm1.yandex.net", request_data=request_data, expected_data={
        "dynamic_config": {
            "ipv4": None,
            "ipv6": None,
            "placement": "hh.yandex.net",
        },
        "id": 5,
        "fqdn": "svm1.yandex.net",
        "stand": "stand3"
    })

    # check update <stand>
    request_data = {
        "stand": None
    }
    bootstrap_api_req("patch", "v1/svms/svm1.yandex.net", request_data=request_data, expected_data={
        "dynamic_config": {
            "ipv4": None,
            "ipv6": None,
            "placement": "hh.yandex.net",
        },
        "stand": None,
        "id": 5,
        "fqdn": "svm1.yandex.net"
    })

    # check udpate to invalid <stand>
    request_data = {
        "stand": "unexisting-stand"
    }
    bootstrap_api_req("patch", "v1/svms/svm1.yandex.net", request_data=request_data, expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })

    _ensure_host_configs_version(5)


def test_delete_host_simple(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check unexisting
    bootstrap_api_req("delete", "v1/hosts/unexisting.yandex.net", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Hosts <unexisting.yandex.net> do not exist in database",
        "status": "fail"
    })

    # check existing
    bootstrap_api_req("delete", "v1/hosts/h2.yandex.net")
    bootstrap_api_req("get", "v1/hosts/h2.yandex.net", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Hosts <h2.yandex.net> do not exist in database",
        "status": "fail"
    })
    resp = bootstrap_api_req("get", "v1/svms")
    assert len(resp.json()["data"]) == 4

    _ensure_host_configs_version(3)


def test_batch_update_hosts(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check have unexisting hosts
    request_data = [
        {"fqdn": "h1.yandex.net"},
        {"fqdn": "unexisting1.yandex.net", "dynamic_config": None},
        {"fqdn": "unexisting2.yandex.net", "dynamic_config": None},
        {"fqdn": "h1.yandex.net"},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-update", request_data=request_data, expected_json={
        "code": 404,
        "data": None,
        "error_message": ("RecordNotFoundError: Hosts <unexisting1.yandex.net,unexisting2.yandex.net> are not found "
                          "in database"),
        "status": "fail"

    })

    # check missing params in data
    request_data = [
        {"dynamic_config": None},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-update", request_data=request_data, expected_json={
        "code": 400,
        "data": None,
        "error_message": "BadRequest: Input payload validation failed:\n  fqdn: 'fqdn' is a required property",
        "status": "fail"
    })

    # check valid update
    request_data = [
        {"fqdn": "h1.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand3"},
        {"fqdn": "h2.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG},
        {"fqdn": "h3.yandex.net", "stand": None},
        {"fqdn": "h4.yandex.net", "stand": "stand3"},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-update", request_data=request_data, expected_data=[
        {"id": 1, "fqdn": "h1.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand3"},
        {"id": 2, "fqdn": "h2.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": "stand2"},
        {"id": 3, "fqdn": "h3.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": None},
        {"id": 4, "fqdn": "h4.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": "stand3"},
    ])

    _ensure_host_configs_version(3)


def test_batch_add_hosts(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check already existing hosts
    request_data = [
        {"fqdn": "h1.yandex.net", "dynamic_config": None, "stand": None},
        {"fqdn": "new1.yandex.net", "dynamic_config": None, "stand": None},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=request_data, expected_json={
        "code": 409,
        "data": None,
        "error_message": "RecordAlreadyInDbError: Hosts <h1.yandex.net> are already in database",
        "status": "fail"
    })

    # check missing params
    request_data = [
        {"fqdn": "new1.yandex.net", "stand": None, "dynamic_config": None},
        {"fqdn": "new2.yandex.net"},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=request_data, expected_json={
        "code": 400,
        "data": None,
        "error_message": ("BadRequest: Input payload validation failed:\n  dynamic_config: 'dynamic_config' is a "
                          "required property\n  stand: 'stand' is a required property"),
        "status": "fail"
    })
    request_data = [
        {"fqdn": "new1.yandex.net", "stand": None}
    ]
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=request_data, expected_json={
        "code": 400,
        "data": None,
        "error_message": ("BadRequest: Input payload validation failed:\n  dynamic_config: 'dynamic_config' is a "
                          "required property"),
        "status": "fail"
    })

    # check valid add
    request_data = [
        {"fqdn": "new1.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand1"},
        {"fqdn": "new2.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand2"},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-add", request_data=request_data, expected_data=[
        {"id": 9, "fqdn": "new1.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand1"},
        {"id": 10, "fqdn": "new2.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand2"},
    ])
    bootstrap_api_req("get", "v1/hosts", expected_data=[
        {"id": 1, "fqdn": "h1.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": "stand1"},
        {"id": 2, "fqdn": "h2.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand2"},
        {"id": 3, "fqdn": "h3.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand1"},
        {"id": 4, "fqdn": "h4.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": None},
        {"id": 9, "fqdn": "new1.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand1"},
        {"id": 10, "fqdn": "new2.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand2"},
    ])

    _ensure_host_configs_version(3)


def test_batch_upsert_hosts(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check missing params
    request_data = [
        {"fqdn": "new1.yandex.net"}
    ]
    bootstrap_api_req("post", "v1/hosts:batch-upsert", request_data=request_data, expected_json={
        "code": 400,
        "data": None,
        "error_message": ("BadRequest: Input payload validation failed:\n  dynamic_config: 'dynamic_config' is a "
                          "required property\n  stand: 'stand' is a required property"),
        "status": "fail"
    })

    # check valid variant
    request_data = [
        {"fqdn": "h1.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": "stand3"},
        {"fqdn": "h2.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": None},
        {"fqdn": "h5.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand3"},
    ]
    bootstrap_api_req("post", "v1/hosts:batch-upsert", request_data=request_data, expected_data=[
        {"id": 1, "fqdn": "h1.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": "stand3"},
        {"id": 2, "fqdn": "h2.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": None},
        {"id": 9, "fqdn": "h5.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand3"},
    ])
    bootstrap_api_req("get", "v1/hosts", expected_data=[
        {"id": 1, "fqdn": "h1.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": "stand3"},
        {"id": 2, "fqdn": "h2.yandex.net", "dynamic_config": _H1_DYNAMIC_CONFIG, "stand": None},
        {"id": 3, "fqdn": "h3.yandex.net", "dynamic_config": _H3_DYNAMIC_CONFIG, "stand": "stand1"},
        {"id": 4, "fqdn": "h4.yandex.net", "dynamic_config": _H4_DYNAMIC_CONFIG, "stand": None},
        {"id": 9, "fqdn": "h5.yandex.net", "dynamic_config": _H2_DYNAMIC_CONFIG, "stand": "stand3"},
    ])

    _ensure_host_configs_version(3)


def test_batch_delete_hosts(bootstrap_api, bootstrap_db):
    _setup_simple()

    # try to remove svms instead of hosts
    request_data = {
        "fqdns": ["h1.yandex.net", "h2.yandex.net", "svm3.yandex.net", "svm4.yandex.net"],
    }
    bootstrap_api_req("post", "v1/hosts:batch-delete", request_data=request_data, expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Hosts <svm3.yandex.net,svm4.yandex.net> are not found in database",
        "status": "fail"
    })

    # valid remove
    request_data = {
        "fqdns": ["h1.yandex.net", "h2.yandex.net"],
    }
    bootstrap_api_req("post", "v1/hosts:batch-delete", request_data=request_data)
    resp = bootstrap_api_req("get", "v1/hosts")
    assert {elem["fqdn"] for elem in resp.json()["data"]} == {"h3.yandex.net", "h4.yandex.net"}

    _ensure_host_configs_version(3)


def test_batch_delete_svms(bootstrap_api, bootstrap_db):
    _setup_simple()

    # try to remove svms instead of hosts
    request_data = {
        "fqdns": ["h1.yandex.net", "h2.yandex.net", "svm3.yandex.net", "svm4.yandex.net"],
    }
    bootstrap_api_req("post", "v1/svms:batch-delete", request_data=request_data, expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Svms <h1.yandex.net,h2.yandex.net> are not found in database",
        "status": "fail"
    })

    # valid remove
    request_data = {
        "fqdns": ["svm3.yandex.net", "svm4.yandex.net"],
    }
    bootstrap_api_req("post", "v1/svms:batch-delete", request_data=request_data)
    resp = bootstrap_api_req("get", "v1/svms")
    assert {elem["fqdn"] for elem in resp.json()["data"]} == {"svm1.yandex.net", "svm2.yandex.net"}

    _ensure_host_configs_version(3)
