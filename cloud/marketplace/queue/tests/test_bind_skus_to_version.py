import unittest.mock as m

from yc_common.clients.billing import SkuProductLinkRequest

from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as Version
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks.bind_skus_to_version import task as bind_sku_to_version


def test_bind_sku_to_version(mocker, monkeypatch):
    # Arrange
    cloud_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    sku_id = generate_id()
    check_formula = "some formula"

    mock_version = Version({
        "id": version_id,
        "image_id": source_image_id,
        "status": Version.Status.ACTIVE,
        "skus": [{
            "id": sku_id,
            "check_formula": check_formula,
        }],
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", cloud_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(BillingPrivateClient, "link_sku")
    mocker.patch.object(OsProductFamilyVersion, "get")
    OsProductFamilyVersion.get.return_value = mock_version
    # Act
    resolution = bind_sku_to_version(
        {"params": {
            "version_id": version_id,
        }})

    # Assert
    BillingPrivateClient.link_sku.assert_called_with(
        sku_id,
        SkuProductLinkRequest({
            "product_id": version_id,
            "check_formula": check_formula,
        }),
    )
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "version_id": version_id,
        })


def test_bind_multiple_skus(mocker, monkeypatch):
    # Arrange
    cloud_id = generate_id()
    source_image_id = generate_id()
    version_id = generate_id()
    sku_id1 = generate_id()
    sku_id2 = generate_id()
    check_formula1 = "some formula 1"
    check_formula2 = "some formula 2"

    mock_version = Version({
        "id": version_id,
        "image_id": source_image_id,
        "status": Version.Status.ACTIVE,
        "skus": [
            {
                "id": sku_id1,
                "check_formula": check_formula1,
            },
            {
                "id": sku_id2,
                "check_formula": check_formula2,
            },
        ],
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", cloud_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(BillingPrivateClient, "link_sku")
    mocker.patch.object(OsProductFamilyVersion, "get")
    OsProductFamilyVersion.get.return_value = mock_version
    # Act
    resolution = bind_sku_to_version(
        {"params": {
            "version_id": version_id,
        }})

    # Assert
    calls = [
        m.call(sku_id1,
               SkuProductLinkRequest({
                   "product_id": version_id,
                   "check_formula": check_formula1,
               })),
        m.call(sku_id2,
               SkuProductLinkRequest({
                   "product_id": version_id,
                   "check_formula": check_formula2,
               })),
    ]
    BillingPrivateClient.link_sku.assert_has_calls(calls)

    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "version_id": version_id,
        })


def test_bind_sku_bad_params():
    resolution = bind_sku_to_version({})
    assert resolution.result is None
    assert resolution.error.message == "failed"
    assert resolution.error.details[0].type == "TaskParamsValidationError"
