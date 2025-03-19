from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.models.results import UpdateImagePoolSizeResult
from cloud.marketplace.queue.yc_marketplace_queue.tasks import finalize_image_pool_size


def test_finalize_image_pool_size(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    op_id = generate_id()
    update_op = OperationV1Beta1({
        "id": op_id,
        "metadata": {
            "image_id": image_id,
        },
        "done": True,
    })

    update_image_pool_size_task = Task.new(generate_id(), operation_type="update_image_pool_size",
                                           response=UpdateImagePoolSizeResult({
                                               "operation_id": op_id,
                                           }).to_primitive())

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "get_operation")
    ComputeClient.get_operation.return_value = update_op

    def depends():
        for i in [update_image_pool_size_task]:
            yield i

    # Act
    resolution = finalize_image_pool_size(
        depends(),
        {
            "params": {},
        })

    # Assert
    ComputeClient.get_operation.assert_called_with(
        op_id,
    )
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={},
    )


def test_finalize_image_pool_size_failed(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    op_id = generate_id()

    error = {
        "code": 13,
        "message": "internal",
        "details": [],
    }

    update_op = OperationV1Beta1({
        "id": op_id,
        "metadata": {
            "image_id": image_id,
        },
        "done": True,
        "error": ErrorV1Beta1(error).to_primitive(),
    })

    update_image_pool_size_task = Task.new(generate_id(), operation_type="update_image_pool_size",
                                           response=UpdateImagePoolSizeResult({
                                               "operation_id": op_id,
                                           }).to_primitive())

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "get_operation")
    ComputeClient.get_operation.return_value = update_op

    def depends():
        for i in [update_image_pool_size_task]:
            yield i

    # Act
    resolution = finalize_image_pool_size(
        depends(),
        {
            "params": {},
        })

    # Assert
    ComputeClient.get_operation.assert_called_with(
        op_id,
    )
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.FAILED,
        data=error,
    )
