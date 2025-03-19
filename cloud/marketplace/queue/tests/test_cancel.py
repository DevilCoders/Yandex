import grpc
from grpc._common import STATUS_CODE_TO_CYGRPC_STATUS_CODE

from yc_common.clients.kikimr.client import _KikimrBaseConnection
from yc_common.clients.kikimr.sql import QueryTemplate
from yc_common.clients.kikimr.sql import SqlIn
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.queue.yc_marketplace_queue.tasks import cancel
from cloud.marketplace.queue.yc_marketplace_queue.tasks.cancel import fetch_group_task_ids


def test_fetch_group_task_ids(mocker):
    # Arrange
    old_task_id = generate_id()
    cancelable_task_id = generate_id()
    protected_task_id = generate_id()

    target_id = generate_id()
    cancel_task_instance = Task.new(target_id)

    mocker.patch.object(_KikimrBaseConnection, "select")
    _KikimrBaseConnection.select.return_value = iter([
        {
            "id": cancelable_task_id,
            "params": {
                "is_cancelable": True,
            },
        },
        {
            "id": protected_task_id,
            "params": {
                "is_cancelable": False,
            },
        },
        {
            "id": old_task_id,
            "params": {},
        },
    ])

    # Act
    ids = fetch_group_task_ids(cancel_task_instance)

    assert len(ids) == 2
    assert sorted(ids) == sorted([cancelable_task_id, old_task_id])


def test_cancel(mocker):
    # Arrange
    task_id = generate_id()
    target_id = generate_id()
    cancelable_task_instance = Task.new(target_id)

    mocker.patch.object(_KikimrBaseConnection, "select")
    mocker.patch.object(_KikimrBaseConnection, "select_one")
    mocker.patch.object(_KikimrBaseConnection, "update_object")
    _KikimrBaseConnection.select.return_value = iter([{"id": target_id}])
    _KikimrBaseConnection.select_one.return_value = cancelable_task_instance

    # Act
    resolution = cancel(task_id, {"params": {"id": target_id}})

    # Assert
    cancel_code = STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.CANCELLED]
    for call in _KikimrBaseConnection.update_object.call_args_list:
        args, kwargs = call
        print(args[2].render(QueryTemplate()))
        assert args[1].get("response") is None
        assert args[1].get("error") == {"code": cancel_code, "details": [], "message": "canceled"}
        assert args[2].render(QueryTemplate()) == SqlIn("id", [target_id]).render(QueryTemplate())

    assert TaskResolution.resolve(status=TaskResolution.Status.RESOLVED) == resolution


def test_cancel_bad_params(mocker):
    # Arrange
    task_id = generate_id()
    target_id = generate_id()
    cancel_task_instance = Task.new(target_id)

    mocker.patch.object(_KikimrBaseConnection, "select")
    mocker.patch.object(_KikimrBaseConnection, "select_one")
    mocker.patch.object(_KikimrBaseConnection, "update_object")
    _KikimrBaseConnection.select.return_value = iter([])
    _KikimrBaseConnection.select_one.return_value = cancel_task_instance

    # Act
    resolution = cancel(task_id, {"id": target_id})

    # Assert
    assert resolution.error.message == "failed"
