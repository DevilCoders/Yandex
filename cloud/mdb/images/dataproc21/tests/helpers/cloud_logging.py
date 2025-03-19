import datetime
import random
import time

from yandex.cloud.logging.v1.log_group_service_pb2_grpc import LogGroupServiceStub
from yandex.cloud.logging.v1.log_group_pb2 import LogGroup
from yandex.cloud.logging.v1.log_group_service_pb2 import (
    CreateLogGroupRequest,
    CreateLogGroupMetadata,
    DeleteLogGroupRequest,
    DeleteLogGroupMetadata,
    ListLogGroupsRequest,
)
from yandex.cloud.logging.v1.log_reading_service_pb2_grpc import LogReadingServiceStub
from yandex.cloud.logging.v1.log_reading_service_pb2 import (
    Criteria,
    ReadRequest,
)
from google.protobuf.duration_pb2 import Duration


def read(
    ctx,
    timeout=120,
    entries_number=1,
    log_type=None,
    fqdn=None,
    cluster_id=None,
    message=None,
    yarn_log_type=None,
):
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    log_group_id = conf['environment']['log_group_id']
    logging_read_service = sdk.client(LogReadingServiceStub)
    resource_ids = None

    filters = []
    if log_type:
        filters.append(f'log_type:"{log_type}"')
    if fqdn:
        filters.append(f'hostname:"{fqdn}"')
    if yarn_log_type:
        filters.append(f'yarn_log_type:"{yarn_log_type}"')
    if message:
        filters.append(f'message:"{message}"')
    filter_string = None
    if filters:
        filter_string = ' AND '.join(filters)
    if cluster_id:
        resource_ids = [cluster_id]

    log_entries_filter = Criteria(
        log_group_id=log_group_id,
        page_size=entries_number,
        filter=filter_string,
        resource_ids=resource_ids,
    )

    response = logging_read_service.Read(ReadRequest(criteria=log_entries_filter))

    if timeout and int(timeout) and not response.entries:
        deadline = datetime.datetime.now() + datetime.timedelta(seconds=int(timeout))
        while datetime.datetime.now() < deadline:
            if not response.entries:
                response = logging_read_service.Read(ReadRequest(criteria=log_entries_filter))
            time.sleep(1)
            if response.entries:
                break
    return response.entries


def log_groups_list(ctx, folder_id=None):
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    log_group_service = sdk.client(LogGroupServiceStub)
    if not folder_id:
        folder_id = conf['environment']['folder-id']
    response = log_group_service.List(ListLogGroupsRequest(
        folder_id=folder_id,
    ))
    return response.groups


def create_log_group(ctx, name=None, retention_period_seconds=86400):
    if not name:
        name = f"image-test-{ctx.state.get('cluster', random.randint(0,100))}"
    conf = ctx.conf
    sdk = ctx.state['yandexsdk']
    log_group_id = None
    try:
        log_group_service = sdk.client(LogGroupServiceStub)
        operation = log_group_service.Create(CreateLogGroupRequest(
            name=name,
            folder_id=conf['environment']['folder-id'],
            labels=ctx.labels,
            retention_period=Duration(seconds=retention_period_seconds),
        ))
        operation_result = sdk.wait_operation_and_get_result(
            operation,
            response_type=LogGroup,
            meta_type=CreateLogGroupMetadata,
        )
        log_group_id = operation_result.response.id
        return log_group_id
    finally:
        if log_group_id:
            ctx.state['resources']['log_groups'][name] = log_group_id


def delete_log_group(ctx, log_group_id=None, name=None):
    sdk = ctx.state['yandexsdk']
    log_group_service = sdk.client(LogGroupServiceStub)
    if not log_group_id:
        log_group_id = ctx.state['resources']['log_groups'].get(name)
    if not log_group_id:
        raise RuntimeError(f'Can not find log group to delete. Id {log_group_id}. Name {name}.')
    try:
        operation_result = log_group_service.Delete(DeleteLogGroupRequest(log_group_id=log_group_id))
        result = sdk.wait_operation_and_get_result(
            operation_result,
            meta_type=DeleteLogGroupMetadata,
        )
        return result
    finally:
        ctx.state['resources']['log_groups'].pop(name, None)
