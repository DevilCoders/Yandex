"""Example of create task"""

from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.operation import OperationList
from cloud.marketplace.common.yc_marketplace_common.models.task import StartCreateImageTaskCreationRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskCreationRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskExtendLockRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskFinishRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskList
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskLockRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskLockResponse
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResponse
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id


@private_api_handler(
    "GET",
    "/operations",
    params_model=TaskListingRequest,
    response_model=OperationList)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_operations(request: TaskListingRequest, **kwargs):
    tasks = lib.TaskUtils.list(request, filter_query=request.filter, order_by=request.order_by)

    return tasks.to_api(False)


@private_api_handler("GET", "/operations/<operation_id>",
                     context_params=["operation_id"],
                     response_model=OperationV1Beta1)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_operation(operation_id, request_context):
    return lib.TaskUtils.get(operation_id).to_operation_v1beta1().to_api(False)


@private_api_handler("GET", "/operations/<operation_id>/group",
                     params_model=BasePublicListingRequest,
                     context_params=["operation_id"],
                     response_model=OperationList)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_operation_group(request: BasePublicListingRequest, operation_id, request_context):
    task = lib.TaskUtils.get(operation_id)
    return lib.TaskUtils.group(request, task.group_id).to_api(False)


@private_api_handler("POST", "/operations",
                     model=TaskCreationRequest,
                     response_model=OperationV1Beta1)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def create_operation(request: TaskCreationRequest, request_context: object) -> OperationV1Beta1:
    return lib.TaskUtils.create(
        request.type,
        group_id=request.group_id,
        params=request.params,
        is_infinite=request.is_infinite,
        depends=request.depends,
    ).to_api(False)


@private_api_handler("POST", "/operations/start_create_image",
                     model=StartCreateImageTaskCreationRequest,
                     response_model=OperationV1Beta1)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def create_typed_operation(request: StartCreateImageTaskCreationRequest, request_context: object) -> OperationV1Beta1:
    return lib.TaskUtils.create(
        "start_create_image",
        group_id=request.group_id or generate_id(),
        params=request.params.to_primitive(),
        is_infinite=False,
        depends=request.depends,
    ).to_api(False)


@private_api_handler("POST", "/operations/<operation_id>:cancel",
                     context_params=["operation_id"],
                     response_model=OperationV1Beta1)
def cancel_task(operation_id: str, request_context: object) -> OperationV1Beta1:
    return lib.TaskUtils.create(
        "cancel",
        group_id=None,
        params={"id": operation_id},
    ).to_api(False)


@private_api_handler(
    "GET",
    "/buildTasks",
    params_model=TaskListingRequest,
    response_model=TaskList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_build_tasks(request: TaskListingRequest, **kwargs):
    tasks = lib.TaskUtils.list(request, kind=Task.Kind.BUILD, filter_query=request.filter, order_by=request.order_by)
    return tasks.to_api(False)


@private_api_handler(
    "POST",
    "/buildTasks/lock",
    model=TaskLockRequest,
    response_model=TaskLockResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def lock_build_task(request: TaskLockRequest, request_context) -> TaskLockResponse:
    tl = lib.TaskUtils.list(query_args={}, filter_query="unlock_after < {} and done = false".format(timestamp()),
                            kind=Task.Kind.BUILD)
    if len(tl.operations):
        task = lib.TaskUtils.lock(tl.operations[0].id, request.worker_hostname, 60 * 10)
    else:
        task = None

    response = TaskLockResponse({
        "task": task,
    })

    return response.to_api(True)


@private_api_handler("GET", "/buildTasks/<task_id>",
                     context_params=["task_id"],
                     response_model=TaskResponse)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_task(task_id: str, request_context):
    return lib.TaskUtils.get(task_id).to_public().to_api(False)


@private_api_handler("POST", "/buildTasks",
                     model=TaskCreationRequest,
                     response_model=OperationV1Beta1)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def create_task(request: TaskCreationRequest, request_context: object) -> OperationV1Beta1:
    return lib.TaskUtils.create(
        request.type,
        group_id=request.group_id,
        params=request.params,
        is_infinite=request.is_infinite,
        depends=request.depends,
        kind=Task.Kind.BUILD,
    ).to_api(False)


@private_api_handler("POST", "/buildTasks/<task_id>:finish",
                     model=TaskFinishRequest,
                     response_model=OperationV1Beta1,
                     query_variables=True)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def finish_task(request: TaskFinishRequest, request_context: object):
    resolution = TaskResolution.resolve(
        status=request.status,
        data=request.response)
    lib.TaskUtils.finish(
        request.task_id,
        request.lock,
        resolution,
        request.execution_time_ms,
    )

    return {"result": "ok"}


@private_api_handler("POST", "/buildTasks/<task_id>:extendLock",
                     model=TaskExtendLockRequest,
                     response_model=OperationV1Beta1,
                     query_variables=True)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def extend_task_lock(request: TaskExtendLockRequest, request_context: object):
    lib.TaskUtils.extend_lock(
        request.task_id,
        request.lock,
        request.interval,
    )

    return {"result": "ok"}
