from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.images import Image
from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.models.task import UpdateImagePoolSizeParams
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.models.results import FinalizeImageResult
from cloud.marketplace.queue.yc_marketplace_queue.models.results import UpdateImagePoolSizeResult
from cloud.marketplace.queue.yc_marketplace_queue.tasks import update_image_pool_size


def test_update_image_pool_size(mocker, monkeypatch):
    # Arrange
    org_id = generate_id()
    image_id = generate_id()
    version_id = generate_id()
    op_id = generate_id()
    update_op = OperationV1Beta1({
        "id": op_id,
        "metadata": {
            "image_id": image_id,
        },
    })
    disk_count = 2

    finalize_task = Task.new(generate_id(), operation_type="finalize_clone_image",
                             response=FinalizeImageResult({
                                 "image_id": image_id,
                                 "status": Image.Status.READY,
                             }).to_primitive())
    bind_task = Task.new(generate_id(), operation_type="bind_skus_to_version", response={"version_id": version_id})

    monkeypatch.setenv("MARKETPLACE_CLOUD_ID", org_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.utils.service_account.service_token")
    mocker.patch.object(ComputeClient, "update_disk_pooling")
    ComputeClient.update_disk_pooling.return_value = update_op

    def depends():
        for i in [bind_task, finalize_task]:
            yield i

    # Act
    resolution = update_image_pool_size(
        depends(),
        {
            "params": UpdateImagePoolSizeParams({
                "pool_size": disk_count,
            }).to_primitive(),
        })

    # Assert
    ComputeClient.update_disk_pooling.assert_called_with(
        image_id=image_id,
        disk_count=disk_count,
        type_id="network-hdd",
    )
    assert resolution == TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=UpdateImagePoolSizeResult({
            "operation_id": op_id,
        }).to_primitive(),
    )
