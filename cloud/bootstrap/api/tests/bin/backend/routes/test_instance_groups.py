"""Test routes for instance groups (<stands> section)"""

from common import bootstrap_api_req

_STANDS = [
    "stand1",
    "stand2",
    "stand3",
]


_INSTANCE_GROUPS = [
    ("instance-group1", "stand1"),
    ("instance-group2", "stand1"),
    ("instance-group1", "stand2"),  # same name as for instance group in stand1
]


def _setup_stands():
    for stand_name in _STANDS:
        bootstrap_api_req("post", "v1/stands", request_data=dict(name=stand_name))


def _setup_instance_groups():
    for instance_group_name, stand_name in _INSTANCE_GROUPS:
        bootstrap_api_req(
            "post", "v1/stands/{}/instance-groups".format(stand_name),
            request_data=dict(name=instance_group_name),
        )


def test_get_instance_group(bootstrap_api, bootstrap_db):
    _setup_stands()

    # empty database
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups", expected_data=[])
    bootstrap_api_req("get", "v1/stands/unexisting-stand/instance-groups", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })

    _setup_instance_groups()

    # get all instance groups in stand
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups", expected_data=[
        {"id": 1, "name": "instance-group1", "stand": "stand1"},
        {"id": 2, "name": "instance-group2", "stand": "stand1"},
    ])
    bootstrap_api_req("get", "v1/stands/stand2/instance-groups", expected_data=[
        {"id": 3, "name": "instance-group1", "stand": "stand2"},
    ])

    # get instance group by one
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups/instance-group1", expected_data={
        "id": 1, "name": "instance-group1", "stand": "stand1"
    })
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups/instance-group1", expected_data={
        "id": 1, "name": "instance-group1", "stand": "stand1"
    })
    bootstrap_api_req("get", "v1/stands/stand3/instance-groups/instance-group1", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Instance Group <instance-group1> is not registered in stand <stand3>",
        "status": "fail"
    })


def test_add_instance_group(bootstrap_api, bootstrap_db):
    _setup_stands()
    _setup_instance_groups()

    # add correct
    bootstrap_api_req("post", "v1/stands/stand1/instance-groups", request_data=dict(name="new-instance-group"))

    # add to unexsisting stand
    bootstrap_api_req(
        "post", "v1/stands/unexisting-stand/instance-groups", request_data=dict(name="new-instance-group"),
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
            "status": "fail"
        }
    )

    # add dublicate instance group
    bootstrap_api_req(
        "post", "v1/stands/stand1/instance-groups", request_data=dict(name="new-instance-group"),
        expected_json={
            "code": 409,
            "data": None,
            "error_message": ("RecordAlreadyInDbError: Instance Group <new-instance-group> already registered in stand "
                              "<stand1>"),
            "status": "fail"
        }
    )

    # add incorrect input data
    bootstrap_api_req(
        "post", "v1/stands/stand1/instance-groups", request_data=dict(),
        expected_json={
            "code": 400,
            "data": None,
            "error_message": "BadRequest: Input payload validation failed:\n  name: 'name' is a required property",
            "status": "fail"
        }
    )


def test_delete_instance_group(bootstrap_api, bootstrap_db):
    _setup_stands()
    _setup_instance_groups()

    # correct deletion
    bootstrap_api_req("delete", "v1/stands/stand1/instance-groups/instance-group1")
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups", expected_data=[
        {"id": 2, "name": "instance-group2", "stand": "stand1"},
    ])
    bootstrap_api_req("delete", "v1/stands/stand1/instance-groups/instance-group2")
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups", expected_data=[])

    # invalid deletion
    bootstrap_api_req("delete", "v1/stands/unexisting-stand/instance-groups/instance-group1", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })
    bootstrap_api_req("delete", "v1/stands/stand2/instance-groups/unexisting-instance-group", expected_json={
        "code": 404,
        "data": None,
        "error_message": ("RecordNotFoundError: Instance Group <unexisting-instance-group> is not registered in stand "
                          "<stand2>"),
        "status": "fail"
    })


def test_get_instance_group_release(bootstrap_api, bootstrap_db):
    _setup_stands()
    _setup_instance_groups()

    # empty initial release
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups/instance-group1/release", expected_data={
        "id": 1, "url": None, "image_id": None, "stand": "stand1", "instance_group": "instance-group1"
    })

    # invalid stand or instance group
    bootstrap_api_req("get", "v1/stands/unexisting-stand/instance-groups/instance-group1/release", expected_json={
        "code": 404,
        "data": None,
        "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
        "status": "fail"
    })
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups/unexisting-instance-group/release", expected_json={
        "code": 404,
        "data": None,
        "error_message": ("RecordNotFoundError: Instance Group <unexisting-instance-group> is not registered in stand "
                          "<stand1>"),
        "status": "fail"
    })


def test_put_instance_group_release(bootstrap_api, bootstrap_db):
    _setup_stands()
    _setup_instance_groups()

    # correct updates
    bootstrap_api_req(
        "put", "v1/stands/stand1/instance-groups/instance-group1/release",
        request_data={
            "url": "http://s3/url1", "image_id": "qazwsxedc",
        },
        expected_data={
            "id": 1, "url": "http://s3/url1", "image_id": "qazwsxedc", "stand": "stand1",
            "instance_group": "instance-group1"
        }
    )
    bootstrap_api_req("get", "v1/stands/stand1/instance-groups/instance-group1/release", expected_data={
        "id": 1, "url": "http://s3/url1", "image_id": "qazwsxedc", "stand": "stand1",
        "instance_group": "instance-group1"
    })
    bootstrap_api_req(
        "put", "v1/stands/stand1/instance-groups/instance-group1/release",
        request_data={
            "url": None, "image_id": None,
        },
        expected_data={
            "id": 1, "url": None, "image_id": None, "stand": "stand1", "instance_group": "instance-group1"
        }
    )

    # incorrect updates
    bootstrap_api_req(
        "put", "v1/stands/unexisting-stand/instance-groups/instance-group1/release",
        request_data={
            "url": None, "image_id": None,
        },
        expected_json={
            "code": 404,
            "data": None,
            "error_message": "RecordNotFoundError: Stand <unexisting-stand> is not found in database",
            "status": "fail"
        }
    )
    bootstrap_api_req(
        "put", "v1/stands/stand1/instance-groups/unexisting-instance-group/release",
        request_data={
            "url": None, "image_id": None,
        },
        expected_json={
            "code": 404,
            "data": None,
            "error_message": ("RecordNotFoundError: Instance Group <unexisting-instance-group> is not registered in "
                              "stand <stand1>"),
            "status": "fail"
        }
    )
    bootstrap_api_req(
        "put", "v1/stands/stand1/instance-groups/instance-group1/release", request_data={},
        expected_json={
            "code": 400,
            "data": None,
            "error_message": ("BadRequest: Input payload validation failed:\n  image_id: 'image_id' is a required "
                              "property\n  url: 'url' is a required property"),
            "status": "fail"
        }
    )
