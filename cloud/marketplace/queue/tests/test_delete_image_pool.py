from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.common.yc_marketplace_common.models.task import DeleteImagePoolParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks import delete_image_pool


def test_delete_image_pool(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    op_id = generate_id()
    delete_op = OperationV1Beta1({
        "id": op_id,
        "metadata": {
            "image_id": image_id,
        },
        "done": True,
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "delete_disk_pooling")
    ComputeClient.delete_disk_pooling.return_value = delete_op

    # Act
    resolution = delete_image_pool(
        {
            "params": DeleteImagePoolParams({
                "image_id": image_id,
            }),
        })

    # Assert
    ComputeClient.delete_disk_pooling.assert_called_with(
        image_id=image_id,
    )
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={},
    )


def test_delete_image_pool_bad_params(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    op_id = generate_id()
    delete_op = OperationV1Beta1({
        "id": op_id,
        "metadata": {
            "image_id": image_id,
        },
        "done": True,
    })

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "delete_disk_pooling")
    ComputeClient.delete_disk_pooling.return_value = delete_op

    # Act
    resolution = delete_image_pool(
        {
            "params": {},
        })

    # Assert
    ComputeClient.delete_disk_pooling.assert_not_called()
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.FAILED,
        data=ErrorV1Beta1({
            "details": [
                {
                    "type": "TaskParamValidationError",
                    "code": 13,
                    "message": "Params validation failed with error: {\"image_id\": [\"This field is "
                               "required.\"]}.",
                }],
            "code": 13,
            "message": "failed",
        }),
    )
