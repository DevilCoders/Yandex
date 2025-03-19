from unittest.mock import Mock

from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.images import ImageOperation
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks import start_clone_image


class MockOs:
    type = "mock os type"


class MockImage:
    name = "mock-name"
    os = MockOs()
    description = "mock description"
    family = "mock family"
    product_ids = ["mock id"]
    min_disk_size = 100
    labels = {"foo": "bar"}
    requirements = {}


class MockImageWithRequirements:
    name = "mock-name"
    os = MockOs()
    description = "mock description"
    family = "mock family"
    product_ids = ["mock id"]
    min_disk_size = 100
    labels = {"foo": "bar"}
    requirements = {"min_network_interfaces": "1",
                    "max_network_interfaces": "6"}


def test_start_clone_image(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })
    mock_image = MockImage()
    mock_time = Mock()
    mock_time.return_value = 1514754000

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    mocker.patch("time.time", mock_time)
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = MockImage()

    # Act
    resolution = start_clone_image(
        {"params": {
            "version_id": version_id,
            "source_image_id": source_image_id,
            "target_folder_id": target_folder_id,
            "name": "image-name",
            "product_ids": [version_id],
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name="image-name-1514754000",
        image_id=source_image_id,
        description=mock_image.description,
        family=mock_image.family,
        product_ids=[version_id],
        os_type=mock_image.os.type,
        min_disk_size=mock_image.min_disk_size,
        override_product_ids=True,
        requirements=mock_image.requirements,
        labels=mock_image.labels)

    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_clone_image_has_version_in_pids(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })
    mock_image = MockImage()
    mock_image.product_ids += [version_id]
    mock_time = Mock()
    mock_time.return_value = 1514754000

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    mocker.patch("time.time", mock_time)
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = mock_image

    # Act
    resolution = start_clone_image(
        {"params": {
            "version_id": version_id,
            "source_image_id": source_image_id,
            "target_folder_id": target_folder_id,
            "name": "image-name",
            "product_ids": [version_id],
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name="image-name-1514754000",
        image_id=source_image_id,
        description=mock_image.description,
        family=mock_image.family,
        os_type=mock_image.os.type,
        min_disk_size=mock_image.min_disk_size,
        override_product_ids=True,
        requirements=mock_image.requirements,
        labels=mock_image.labels)

    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_clone_image_with_no_slug(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })
    mock_image = MockImage()
    mock_image.product_ids += [version_id]
    mock_time = Mock()
    mock_time.return_value = 1514754000

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    mocker.patch("time.time", mock_time)
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = mock_image

    # Act
    resolution = start_clone_image(
        {"params": {
            "version_id": version_id,
            "source_image_id": source_image_id,
            "target_folder_id": target_folder_id,
            "product_ids": [version_id],
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name="{}-1514754000".format(mock_image.name),
        image_id=source_image_id,
        description=mock_image.description,
        family=mock_image.family,
        os_type=mock_image.os.type,
        min_disk_size=mock_image.min_disk_size,
        override_product_ids=True,
        requirements=mock_image.requirements,
        labels=mock_image.labels)

    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_clone_image_with_requirements(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    target_folder_id = generate_id()
    resource_spec = ResourceSpec({
        "network_interfaces": {
            "min": 1,
            "max": 6,
        }
    })
    create_op = ImageOperation({
        "metadata": {
            "image_id": image_id,
        },
    })
    mock_image = MockImageWithRequirements()
    mock_time = Mock()
    mock_time.return_value = 1514754000

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "create_image")
    mocker.patch.object(ComputeClient, "get_image")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.os_product_family_version.OsProductFamilyVersion.update")
    mocker.patch("time.time", mock_time)
    ComputeClient.create_image.return_value = create_op
    ComputeClient.get_image.return_value = MockImageWithRequirements()

    # Act
    resolution = start_clone_image(
        {"params": {
            "version_id": version_id,
            "source_image_id": source_image_id,
            "target_folder_id": target_folder_id,
            "name": "image-name",
            "product_ids": [version_id],
            "resource_spec": resource_spec,
        }})

    # Assert
    ComputeClient.create_image.assert_called_with(
        target_folder_id,
        name="image-name-1514754000",
        image_id=source_image_id,
        description=mock_image.description,
        family=mock_image.family,
        product_ids=[version_id],
        os_type=mock_image.os.type,
        min_disk_size=mock_image.min_disk_size,
        override_product_ids=True,
        requirements=mock_image.requirements,
        labels=mock_image.labels)

    OsProductFamilyVersion.update.assert_called_with(version_id, {"image_id": image_id})
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "new_image_id": image_id,
        })


def test_start_clone_image_bad_params():
    resolution = start_clone_image({"params": {}})
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].type == "TaskParamValidationError"
