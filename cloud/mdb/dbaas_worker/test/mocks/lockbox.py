from unittest.mock import Mock

from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.lockbox.v1 import (
    secret_pb2,
    secret_service_pb2,
)
from .utils import handle_action


def create(state, request):
    action_id = f'secret-create-{request}'
    handle_action(state, action_id)
    meta = secret_service_pb2.CreateSecretMetadata()
    meta.secret_id = 'test-secret-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def get(state, request):
    action_id = f'lockbox-get-{request}'
    handle_action(state, action_id)
    res = secret_pb2.Secret(id='test-lockbox-id')
    return res


def get_operation(state, request):
    action_id = f'lockbox-get-operation-{request}'
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def stub(state, action_id):
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def list_objects(state, request):
    response = Mock()
    response.secrets = [get(state, request)]
    response.next_page_token = None
    return response


def lockbox(mocker, state):
    secret_service = mocker.patch(
        'cloud.mdb.internal.python.lockbox.api.' 'secret_service_pb2_grpc.SecretServiceStub'
    ).return_value
    secret_service.Create.side_effect = lambda request, timeout, metadata: create(state, request)
    secret_service.Get.side_effect = lambda request, timeout, metadata: get(state, request)
    secret_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'secret-delete')
    secret_service.List.side_effect = lambda request, timeout, metadata: list_objects(state, request)

    operation_service = mocker.patch(
        'cloud.mdb.internal.python.lockbox.api.' 'operation_service_pb2_grpc.OperationServiceStub'
    ).return_value
    operation_service.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
