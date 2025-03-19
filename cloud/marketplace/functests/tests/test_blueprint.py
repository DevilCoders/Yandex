import pytest

from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintStatus


def make_valid_data(publisher_id):
    return {
        "name": "testBlueprint",
        "publisher_account_id": publisher_id,
        "build_recipe_links": ["s3://blueprints/test.zip", ],
        "test_suites_links": ["https://storage.yandexcloud.net/yc-marketplace-image-tests/generic-linux.zip",
                              "https://storage.yandexcloud.net/yc-marketplace-image-tests/second-test.zip"],
        "test_instance_config": {
            "cores": 1,
            "memory": 2147483648,
        },
        "commit_hash": "abchash",
    }


def test_blueprint_crud(marketplace_private_client: MarketplacePrivateClient, generate_id, db_fixture):
    publisher_id = generate_id()
    data = make_valid_data(publisher_id)

    op = marketplace_private_client.create_blueprint(BlueprintCreateRequest(data))
    blueprint_id = op.metadata.blueprint_id
    blueprint = marketplace_private_client.get_blueprint(blueprint_id)
    print(blueprint)
    assert blueprint.name == data["name"]
    assert blueprint.test_instance_config["cores"] == 1

    filter_query = "publisher_account_id='{}'".format(publisher_id)
    blueprint_list = marketplace_private_client.list_blueprints(filter_query=filter_query)

    assert len(blueprint_list.blueprints) == 1
    assert blueprint_list.blueprints[0].name == data["name"]

    new_build_recipe_links = ["s3://blueprints/test_updated.zip"]

    marketplace_private_client.update_blueprint(blueprint_id,
                                                build_recipe_links=new_build_recipe_links)
    blueprint_updated = marketplace_private_client.get_blueprint(blueprint_id)
    assert blueprint_updated.build_recipe_links[0] == "s3://blueprints/test_updated.zip"


def test_bad_blueprint(marketplace_private_client: MarketplacePrivateClient, generate_id, db_fixture):
    data = {
        "name": "badBlueprint",
        "publisher_account_id": generate_id,
        "build_recipe_links": [],
        "test_suites_links": "",
        "test_instance_config": {
            "cores": 1,
            "memory": 2147483648,
        },
        "commit_hash": "abchash",
    }
    with pytest.raises(Exception):
        marketplace_private_client.create_blueprint(BlueprintCreateRequest(data))


def test_blueprint_reject(marketplace_private_client: MarketplacePrivateClient, generate_id, db_fixture):
    publisher_id = generate_id()
    data = make_valid_data(publisher_id)
    op = marketplace_private_client.create_blueprint(BlueprintCreateRequest(data))
    blueprint_id = op.metadata.blueprint_id

    reject_op = marketplace_private_client.reject_blueprint(blueprint_id)
    assert reject_op.response.status == BlueprintStatus.REJECTED

    blueprint = marketplace_private_client.get_blueprint(blueprint_id)
    assert blueprint.status == BlueprintStatus.REJECTED


def test_blueprint_build_task(marketplace_private_client: MarketplacePrivateClient,
                              generate_id, db_fixture):
    # create 3 blueprints
    publisher_id = generate_id()
    data = make_valid_data(publisher_id)
    op = marketplace_private_client.create_blueprint(BlueprintCreateRequest(data))
    blueprint_id_1 = op.metadata.blueprint_id
    accept_op = marketplace_private_client.accept_blueprint(blueprint_id_1)
    assert accept_op.response.status == BlueprintStatus.ACTIVE

    data2 = make_valid_data(publisher_id)
    op = marketplace_private_client.create_blueprint(BlueprintCreateRequest(data2))
    blueprint_id_2 = op.metadata.blueprint_id
    accept_op = marketplace_private_client.accept_blueprint(blueprint_id_2)
    assert accept_op.response.status == BlueprintStatus.ACTIVE

    data3 = make_valid_data(publisher_id)
    op = marketplace_private_client.create_blueprint(BlueprintCreateRequest(data3))
    blueprint_id_3 = op.metadata.blueprint_id
    accept_op = marketplace_private_client.accept_blueprint(blueprint_id_3)
    assert accept_op.response.status == BlueprintStatus.ACTIVE

    # list them with filter: status=active
    count = 0
    list = marketplace_private_client.list_blueprints(
        filter_query="publisher_account_id='{}' and status='ACTIVE'".format(publisher_id))
    print(list)

    for count, blueprint in enumerate(list.blueprints, start=1):
        build_op = marketplace_private_client.build_blueprint(blueprint.id)
        print(build_op)
        assert build_op.done is False
    assert count == 3
