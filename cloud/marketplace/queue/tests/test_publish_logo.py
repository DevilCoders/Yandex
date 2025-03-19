from cloud.marketplace.common.yc_marketplace_common.lib import Avatar
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks import publish_logo


def test_start_clone_image(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    avatar_id = generate_id()
    target_key = generate_id()
    target_bucket = "target_bucket"
    url = "foo"

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(Avatar, "rpc_copy")
    Avatar.rpc_copy.return_value = url

    # Act
    resolution = publish_logo(
        {"params": {
            "avatar_id": avatar_id,
            "target_key": target_key,
            "target_bucket": target_bucket,
        }})

    # Assert
    Avatar.rpc_copy.assert_called_with(avatar_id, target_key, target_bucket, rewrite=False)

    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "url": url,
        })
