from cloud.marketplace.common.yc_marketplace_common.client import MarketplacePrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildStatus


def test_blueprint_build_task_success(marketplace_private_client: MarketplacePrivateClient,
                                      generate_id, db_fixture):
    blueprint_id = db_fixture["blueprints"][0]["id"]

    build_op = marketplace_private_client.build_blueprint(blueprint_id)
    print(build_op)
    assert build_op.done is False

    build = marketplace_private_client.get_build(build_op.metadata.build_id)
    assert build.status == BuildStatus.NEW

    build_op = marketplace_private_client.start_build(build.id)
    build = marketplace_private_client.get_build(build_op.metadata.build_id)
    assert build.status == BuildStatus.RUNNING

    compute_image_id = generate_id()
    build_op = marketplace_private_client.finish_build(build_id=build.id,
                                                       compute_image_id=compute_image_id,
                                                       status=BuildStatus.SUCCEEDED)
    assert build_op.done is True
    build_list = marketplace_private_client.list_builds()

    assert len(build_list.builds) == 1
    assert build_list.builds[0].status == BuildStatus.SUCCEEDED
    assert build_list.builds[0].compute_image_id == compute_image_id


def test_blueprint_build_task_failed(marketplace_private_client: MarketplacePrivateClient,
                                     generate_id, db_fixture):
    blueprint_id = db_fixture["blueprints"][0]["id"]

    build_op = marketplace_private_client.build_blueprint(blueprint_id)
    print(build_op)
    assert build_op.done is False

    build = marketplace_private_client.get_build(build_op.metadata.build_id)
    assert build.status == BuildStatus.NEW

    build_op = marketplace_private_client.start_build(build.id)
    build = marketplace_private_client.get_build(build_op.metadata.build_id)
    assert build.status == BuildStatus.RUNNING

    compute_image_id = ""
    build_op = marketplace_private_client.finish_build(build_id=build.id,
                                                       compute_image_id=compute_image_id,
                                                       status=BuildStatus.FAILED)
    assert build_op.done is True
    build_list = marketplace_private_client.list_builds()

    assert len(build_list.builds) == 1
    assert build_list.builds[0].status == BuildStatus.FAILED
    assert build_list.builds[0].compute_image_id == compute_image_id
