from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as Version
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks import start_publish_image
from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.images import Image
from yc_common.clients.models.images import ImageOperation
from yc_common.clients.models.operating_system import OperatingSystem


def test_start_publish_image(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    product_id = generate_id()
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })

    mock_image = Image({
        "id": source_image_id,
        "name": "image name",
        "product_ids": [product_id],
        "description": "image description",
        "family": "foo",
        "os": OperatingSystem({"type": OperatingSystem.Type.LINUX}),
        "min_disk_size": 100,
        "labels": {"foo": "bar"},
    })
    mock_version = Version({
        "id": version_id,
        "image_id": source_image_id,
        "status": Version.Status.ACTIVATING,
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch.object(OsProductFamilyVersion, "get")
    mocker.patch(
        "cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = mock_image
    OsProductFamilyVersion.get.return_value = mock_version
    # Act
    resolution = start_publish_image(
        {"params": {
            "version_id": version_id,
            "target_folder_id": target_folder_id,
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name=mock_image.name,
        image_id=mock_version.image_id,
        description=mock_image.description,
        labels=mock_image.labels,
        min_disk_size=mock_image.min_disk_size,
        os_type=mock_image.os.type,
        family=mock_image.family,
    )
    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_publish_image_no_family(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    product_id = generate_id()
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })

    mock_image = Image({
        "id": source_image_id,
        "name": "image name",
        "product_ids": [product_id],
        "description": "image description",
        "family": "",
        "os": OperatingSystem({"type": OperatingSystem.Type.LINUX}),
        "min_disk_size": 100,
        "labels": {"foo": "bar"},
    })
    mock_version = Version({
        "id": version_id,
        "image_id": source_image_id,
        "status": Version.Status.ACTIVATING,
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch.object(OsProductFamilyVersion, "get")
    mocker.patch(
        "cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = mock_image
    OsProductFamilyVersion.get.return_value = mock_version
    # Act
    resolution = start_publish_image(
        {"params": {
            "version_id": version_id,
            "target_folder_id": target_folder_id,
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name=mock_image.name,
        image_id=mock_version.image_id,
        description=mock_image.description,
        labels=mock_image.labels,
        min_disk_size=mock_image.min_disk_size,
        os_type=mock_image.os.type,
    )
    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_publish_image_bad_params():
    resolution = start_publish_image({"params": {}})
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].type == "TaskParamValidationError"


def test_start_publish_image_bad_version(mocker, monkeypatch):
    org_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(OsProductFamilyVersion, "get")
    mocker.patch(
        "cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    OsProductFamilyVersion.get.return_value = Version({
        "id": version_id,
        "status": Version.Status.ACTIVE,
    })

    # Act
    resolution = start_publish_image(
        {"params": {
            "version_id": version_id,
            "target_folder_id": target_folder_id,
        }})

    # Assert
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].message == "Can not publish version from status '%s'" % Version.Status.ACTIVE
