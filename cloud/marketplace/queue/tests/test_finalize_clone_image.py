from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.images import Image
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion as Version
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.models.results import FinalizeImageResult
from cloud.marketplace.queue.yc_marketplace_queue.tasks import finalize_clone_image


def test_finalize_clone_image(mocker, monkeypatch):
    # Arrange
    cloud_id = generate_id()
    new_image_id = generate_id()
    version_id = generate_id()
    product_id = generate_id()
    start_task = Task.new(generate_id(), operation_type="start_clone_image",
                          response={"new_image_id": new_image_id})
    bind_task = Task.new(generate_id(), operation_type="bind_skus_to_version", response={"version_id": version_id})
    image = Image({
        "id": new_image_id,
        "status": Image.Status.READY,
        "storage_size": 2 ** 20,
        "min_disk_size": 2 ** 20,
        "family": "",
        "product_ids": [product_id]})

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", cloud_id)
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    mocker.patch("yc_common.clients.kikimr.client._KikimrTxConnection")
    ComputeClient.get_image.return_value = image

    def depends():
        for i in [bind_task, start_task]:
            yield i

    # Act
    resolution = finalize_clone_image(depends(), {
        "is_infinite": False,
        "params": {"target_status": Version.Status.REVIEW, "version_id": version_id},
    })

    # Assert
    # OsProductFamilyVersion.update.assert_called_with(version_id, {"status": Version.Status.REVIEW}, propagate=True)

    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=FinalizeImageResult({
            "image_id": new_image_id,
            "status": Image.Status.READY,
        }).to_primitive(),
    )


def test_finalize_clone_image_error(mocker, monkeypatch):
    # Arrange
    cloud_id = generate_id()
    new_image_id = generate_id()
    version_id = generate_id()
    product_id = generate_id()
    start_task = Task.new(generate_id(), operation_type="start_clone_image",
                          response={"new_image_id": new_image_id})
    image = Image({
        "id": new_image_id,
        "status": Image.Status.ERROR,
        "storage_size": 2 ** 20,
        "min_disk_size": 2 ** 20,
        "family": "",
        "product_ids": [product_id]})

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", cloud_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    ComputeClient.get_image.return_value = image

    def depends():
        for i in [start_task]:
            yield i

    # Act
    resolution = finalize_clone_image(depends(), {
        "is_infinite": False,
        "params": {"target_status": Version.Status.REVIEW, "version_id": version_id},
    })

    # Assert
    OsProductFamilyVersion.update.assert_called_with(version_id, {"status": Version.Status.ERROR})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.FAILED,
        data={
            "message": "Creating image ended with status code error",
        })


def test_finalize_clone_image_bad_deps():
    start_task = Task.new(generate_id(), response={})

    resolution = finalize_clone_image([start_task], {})
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].type == "TaskDepsValidationError"


def test_finalize_clone_image_bad_params():
    new_image_id = generate_id()
    version_id = generate_id()
    start_task = Task.new(generate_id(), response={"new_image_id": new_image_id, "version_id": version_id})

    def depends():
        for i in [start_task]:
            yield i

    resolution = finalize_clone_image(depends(), {})
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].type == "TaskDepsValidationError"
