"""Test routes for cluster_maps (<stands> section)"""

from common import bootstrap_api_req

_STANDS = [
    "stand1",
    "stand2",
]


def _setup_stands():
    for stand_name in _STANDS:
        bootstrap_api_req("post", "v1/stands", request_data=dict(name=stand_name))


def test_get_cluster_map(bootstrap_api, bootstrap_db):
    _setup_stands()

    # check for unexisting cluster_map
    bootstrap_api_req("get", "v1/stands/unexisting-stand/cluster-map", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })
    bootstrap_api_req("get", "v1/stands/unexisting-stand/cluster-map/version", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })

    # check empty cluster_map
    bootstrap_api_req("get", "v1/stands/stand1/cluster-map", expected_data={
        "id": 1,
        "stand": "stand1",
        "grains": None,
        "cluster_configs_version": None,
        "yc_ci_version": None,
        "bootstrap_templates_version": None,
    })
    bootstrap_api_req("get", "v1/stands/stand1/cluster-map/version", expected_data={
        "cluster_configs_version": None,
        "yc_ci_version": None,
        "bootstrap_templates_version": None,
    })


def test_update_cluster_map(bootstrap_api, bootstrap_db):
    _setup_stands()

    # check for invalid updates
    bootstrap_api_req(
        "patch", "v1/stands/unexisting-stand/cluster-map", request_data={},
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
            "status": "fail",
        }
    )
    bootstrap_api_req(
        "patch", "v1/stands/stand1/cluster-map", request_data={},
        expected_json={
            "code": 500,
            "data": None,
            "error_message": ("BootstrapAssertionError: Got empty list of table columns when updating "
                              "<stand_cluster_maps>"),
            "status": "error",
        }
    )
    bootstrap_api_req(
        "patch", "v1/stands/stand1/cluster-map", request_data={
            "cluster_configs_version": 123,
            "unexisting_attr": "some_value",
        },
        expected_json={
            "code": 500,
            "data": None,
            "error_message": ("BootstrapAssertionError: Columns <unexisting_attr> not found in table "
                              "<stand_cluster_maps>"),
            "status": "error",
        }
    )
    bootstrap_api_req(
        "patch", "v1/stands/stand1/cluster-map", request_data={
            "cluster_configs_version": "hohoho",
            "yc_ci_version": 123,
            "bootstrap_templates_version": 456,
        },
        expected_json={
            "code": 400,
            "data": None,
            "error_message": ("BadRequest: Input payload validation failed:\n  cluster_configs_version: 'hohoho' is "
                              "not of type 'integer', 'null'\n  yc_ci_version: 123 is not of type 'string', 'null'\n  "
                              "bootstrap_templates_version: 456 is not of type 'string', 'null'"),
            "status": "fail",
        }
    )

    # check for valid udpates
    bootstrap_api_req(
        "patch", "v1/stands/stand1/cluster-map", request_data={
            "grains": {"key1": {"subkey1": "v1", "subkey2": "v2"}, "key2": "v3"},
            "yc_ci_version": "hash1"
        },
        expected_data={
            "id": 1,
            "stand": "stand1",
            "grains": {
                "key1": {
                    "subkey1": "v1",
                    "subkey2": "v2"
                },
                "key2": "v3"
            },
            "cluster_configs_version": None,
            "yc_ci_version": "hash1",
            "bootstrap_templates_version": None,
        }
    )
    bootstrap_api_req(
        "patch", "v1/stands/stand1/cluster-map", request_data={
            "grains": None,
            "cluster_configs_version": 123,
            "yc_ci_version": "yc_ci_hash",
            "bootstrap_templates_version": "bootstrap_templates_hash",
        },
        expected_data={
            "id": 1,
            "stand": "stand1",
            "grains": None,
            "cluster_configs_version": 123,
            "bootstrap_templates_version": "bootstrap_templates_hash",
            "yc_ci_version": "yc_ci_hash",
        }
    )

    bootstrap_api_req("get", "v1/stands/stand1/cluster-map/version", expected_data={
        "cluster_configs_version": 123,
        "bootstrap_templates_version": "bootstrap_templates_hash",
        "yc_ci_version": "yc_ci_hash",
    })
    bootstrap_api_req("get", "v1/stands/stand2/cluster-map/version", expected_data={
        "cluster_configs_version": None,
        "bootstrap_templates_version": None,
        "yc_ci_version": None,
    })
