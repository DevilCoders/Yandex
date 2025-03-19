from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion as Scheme
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionResponse as Version
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks.os_product_family_version_create import task as os_version_create


def test_version_create(mocker, monkeypatch):
    # Arrange
    cloud_id = generate_id()
    version_id = generate_id()
    image_id = generate_id()
    logo_id = generate_id()
    billing_account_id = generate_id()
    family_id = generate_id()
    url = "http://example.org/test.png"

    publish_image = Task.new(generate_id(), operation_type="publish_logo", response={"url": url})

    ts = timestamp()

    mock_version = Version({
        "id": version_id,
        "created_at": ts,
        "updated_at": ts,
        "published_at": ts,
        "billing_account_id": billing_account_id,
        "image_id": image_id,
        "logo_id": logo_id,
        "logo_uri": url,
        "status": Scheme.Status.ACTIVE,
        "resource_spec": ResourceSpec({
            "memory": 1,
            "cores": 1,
            "disk_size": 1,
            "user_data_form_id": "linux",
        }).to_primitive(),
        "pricing_options": Scheme.PricingOptions.FREE,
        "os_product_family_id": family_id,
        "skus": [],
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", cloud_id)
    mocker.patch.object(OsProductFamilyVersion, "rpc_set_logo_uri")
    mocker.patch.object(OsProductFamilyVersion, "rpc_get")
    OsProductFamilyVersion.rpc_get.return_value = mock_version

    def depends():
        for i in [publish_image]:
            yield i

    # Act
    resolution = os_version_create(
        depends(),
        {"params": {
            "id": version_id,
        }})

    OsProductFamilyVersion.rpc_set_logo_uri.assert_called_with(version_id, url)
    OsProductFamilyVersion.rpc_get.assert_called_with(version_id)
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=mock_version.to_api(True))
