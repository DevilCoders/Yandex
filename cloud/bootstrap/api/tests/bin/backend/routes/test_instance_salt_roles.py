"""Test routes for salt-roles in <instances> section"""

from common import bootstrap_api_req


_STAND1 = "stand1"

_INSTANCES = [
    ("svm1.yandex.net", ["salt-role1", "salt-role2"]),
    ("svm2.yandex.net", ["salt-role1", "salt-role3"]),
    ("svm3.yandex.net", []),
    ("svm4.yandex.net", ["salt-role1", "salt-role2", "salt-role3"]),
]


_INSTANCE_SALT_ROLE_PACKAGES = [
    ("svm1.yandex.net", [
        ("salt-role1", "yc-salt-formula", "1.2.3"),
        ("salt-role2", "yc-salt-formula", "3.2.3"),
        ("salt-role2", "yc-compute", "4.5.6"),
    ]),
    ("svm2.yandex.net", [
        ("salt-role3", "yc-salt-formula", "1.1.1")
    ]),
    ("svm4.yandex.net", [
        ("salt-role1", "yc-salt-formula", "2.2.2"),
        ("salt-role2", "yc-salt-formula", "2.2.2"),
        ("salt-role3", "yc-salt-formula", "2.2.2"),
    ]),
]


def _setup_simple():
    # add salt roles
    salt_roles = sum([salt_roles for fqdn, salt_roles in _INSTANCES], [])
    salt_roles = sorted(set(salt_roles))
    for salt_role_name in salt_roles:
        bootstrap_api_req("post", "v1/salt-roles", request_data={"name": salt_role_name})

    # add instances and salt roles
    bootstrap_api_req("post", "v1/stands", request_data={"name": _STAND1})
    for fqdn, salt_roles in _INSTANCES:
        bootstrap_api_req("post", "v1/svms", request_data={"fqdn": fqdn, "dynamic_config": None, "stand": _STAND1})
        for salt_role_name in salt_roles:
            bootstrap_api_req("post", "v1/instances/{}/salt-roles".format(fqdn), request_data={"name": salt_role_name})

    # add salt role packages
    for fqdn, packages_data in _INSTANCE_SALT_ROLE_PACKAGES:
        request_data = []
        for salt_role, package_name, target_version in packages_data:
            request_data.append({
                "salt_role": salt_role, "package_name": package_name, "target_version": target_version,
            })
        bootstrap_api_req("put", "v1/instances/{}/salt-role-releases".format(fqdn), request_data=request_data)


def test_get_instance_salt_roles(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check invalid instance
    bootstrap_api_req("get", "v1/instances/unexisting.yandex.net/salt-roles", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
        "status": "fail",
    })

    # check invalid salt role
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles/unexisting-salt-role", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <svm1.yandex.net> does not have role <unexisting-salt-role>",
        "status": "fail",
    })

    # check valid salt role not in instance salt roles
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles/salt-role3", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <svm1.yandex.net> does not have role <salt-role3>",
        "status": "fail",
    })

    # check valid
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles", expected_data=[
        {"id": 1, "name": "salt-role1"},
        {"id": 2, "name": "salt-role2"},
    ])
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles/salt-role2", expected_data={
        "id": 2, "name": "salt-role2",
    })
    bootstrap_api_req("get", "v1/instances/svm3.yandex.net/salt-roles", expected_data=[])


def test_add_instance_salt_role(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check invalid instance
    bootstrap_api_req(
        "post", "v1/instances/unexisting.yandex.net/salt-roles", request_data={"name": "salt-role1"}, expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
            "status": "fail",
        }
    )

    # check invalid salt role
    bootstrap_api_req(
        "post", "v1/instances/svm1.yandex.net/salt-roles", request_data={"name": "unexisting-role"}, expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Salt role <unexisting-role> is not found in database",
            "status": "fail",
        }
    )

    # check instance and salt role already connected to each other
    bootstrap_api_req(
        "post", "v1/instances/svm1.yandex.net/salt-roles", request_data={"name": "salt-role1"}, expected_json={
            "code": 409,
            "data": None,
            "error_message": "RecordAlreadyInDbError: Instance <svm1.yandex.net> already has role <salt-role1>",
            "status": "fail",
        }
    )

    # check valid
    bootstrap_api_req(
        "post", "v1/instances/svm1.yandex.net/salt-roles", request_data={"name": "salt-role3"},
        expected_data={"id": 3, "name": "salt-role3"},
    )
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles", expected_data=[
        {"id": 1, "name": "salt-role1"},
        {"id": 2, "name": "salt-role2"},
        {"id": 3, "name": "salt-role3"},
    ])


def test_upsert_instance_salt_roles(bootstrap_api, bootstrap_db):
    _setup_simple()

    # check invalid instance
    bootstrap_api_req(
        "put", "v1/instances/unexisting.yandex.net/salt-roles",
        request_data=[
            {"name": "salt-role1"},
        ],
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
            "status": "fail",
        }
    )

    # check invalid salt role
    bootstrap_api_req(
        "put", "v1/instances/svm1.yandex.net/salt-roles",
        request_data=[
            {"name": "salt-role1"},
            {"name": "unexisting-salt-role1"},
            {"name": "salt-role3"},
            {"name": "unexisting-salt-role2"},
        ],
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Salt role <unexisting-salt-role1> is not found in database",
            "status": "fail",
        }
    )

    # check valid
    bootstrap_api_req(
        "put", "v1/instances/svm1.yandex.net/salt-roles",
        request_data=[
            {"name": "salt-role1"},
            {"name": "salt-role3"},
        ],
        expected_data=[
            {"id": 1, "name": "salt-role1"},
            {"id": 3, "name": "salt-role3"},
        ]
    )
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles", expected_data=[
        {"id": 1, "name": "salt-role1"},
        {"id": 3, "name": "salt-role3"},
    ])

    # check valid (make empty list)
    bootstrap_api_req("put", "v1/instances/svm4.yandex.net/salt-roles", request_data=[], expected_data=[])
    bootstrap_api_req("get", "v1/instances/svm4.yandex.net/salt-roles", expected_data=[])


def test_delete_instance_salt_role(bootstrap_db, bootstrap_api):
    _setup_simple()

    # check invalid instance
    bootstrap_api_req("delete", "v1/instances/unexisting.yandex.net/salt-roles/salt-role1", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
        "status": "fail",
    })

    # check invalid salt role
    bootstrap_api_req("delete", "v1/instances/svm1.yandex.net/salt-roles/unexisting-salt-role", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <svm1.yandex.net> does not have role <unexisting-salt-role>",
        "status": "fail",
    })

    # check valid
    bootstrap_api_req("delete", "v1/instances/svm1.yandex.net/salt-roles/salt-role1", expected_json=None)
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles", expected_data=[
        {"id": 2, "name": "salt-role2"},
    ])
    bootstrap_api_req("delete", "v1/instances/svm1.yandex.net/salt-roles/salt-role2", expected_json=None)
    bootstrap_api_req("get", "v1/instances/svm1.yandex.net/salt-roles", expected_data=[])


def test_get_instance_salt_role_packages(bootstrap_db, bootstrap_api):
    _setup_simple()

    # test invalid instance
    bootstrap_api_req("get", "v1/instances/unexisting.yandex.net/salt-role-releases", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
        "status": "fail"
    })

    idx = 1
    for fqdn, packages_data in _INSTANCE_SALT_ROLE_PACKAGES:
        expected_data = []
        for salt_role, package_name, target_version in packages_data:
            expected_data.append({
                "id": idx, "salt_role": salt_role, "package_name": package_name, "target_version": target_version,
            })
            idx += 1

        bootstrap_api_req("get", "v1/instances/{}/salt-role-releases".format(fqdn), expected_data=expected_data)


def test_put_instance_salt_role_packages(bootstrap_db, bootstrap_api):
    _setup_simple()

    # test invalid instance
    bootstrap_api_req("put", "v1/instances/unexisting.yandex.net/salt-role-releases", request_data=[], expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance <unexisting.yandex.net> is not found in database",
        "status": "fail"
    })

    # test unexisgint salt role
    request_data = [
        {"salt_role": "unexisting-salt-role", "package_name": "yc-salt-formula", "target_version": "1.2.3"},
    ]
    bootstrap_api_req(
        "put", "v1/instances/svm1.yandex.net/salt-role-releases",
        request_data=[
            {"salt_role": "unexisting-salt-role", "package_name": "yc-salt-formula", "target_version": "1.2.3"},
        ],
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Salt role <unexisting-salt-role> is not found in database",
            "status": "fail"
        },
    )

    # test valid
    request_data = [
        {"salt_role": "salt-role1", "package_name": "yc-salt-formula", "target_version": "1.2.3"},
        {"salt_role": "salt-role1", "package_name": "other-package", "target_version": "3.3.3"},
        {"salt_role": "salt-role3", "package_name": "one-more-package", "target_version": "6.6.6"},
    ]
    expected_data = [
        {"id": 8, "salt_role": "salt-role1", "package_name": "yc-salt-formula", "target_version": "1.2.3"},
        {"id": 9, "salt_role": "salt-role1", "package_name": "other-package", "target_version": "3.3.3"},
        {"id": 10, "salt_role": "salt-role3", "package_name": "one-more-package", "target_version": "6.6.6"},
    ]
    bootstrap_api_req(
        "put", "v1/instances/svm4.yandex.net/salt-role-releases", request_data=request_data, expected_data=expected_data
    )
