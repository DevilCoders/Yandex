"""
Simple IAM mock
"""

import logging
import time
from types import SimpleNamespace

from google.protobuf import any_pb2

from cloud.mdb.internal.python.grpcutil.exceptions import NotFoundError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from yandex.cloud.priv.operation import (
    operation_pb2,
)
from yandex.cloud.priv.iam.v1 import (
    iam_token_service_pb2,
    service_account_pb2,
    service_account_service_pb2,
    key_service_pb2,
)
from yandex.cloud.priv.iam.v1.awscompatibility import access_key_service_pb2

log = MdbLoggerAdapter(logging.getLogger(__name__), {})


def create_token():
    """
    Create token service mock
    """
    res = iam_token_service_pb2.CreateIamTokenResponse()
    res.iam_token = 'dummy-token'
    res.expires_at.FromSeconds(int(time.time()) + 24 * 3600)
    return res


def create_service_account():
    """
    Create service account service mock.
    """
    service_account = service_account_pb2.ServiceAccount()
    service_account.id = 'dummy-account'
    sa_response = SimpleNamespace()
    sa_response.value = service_account.SerializeToString()
    res = SimpleNamespace()
    res.response = sa_response
    return res


def find_service_account():
    """
    List service account service mock.
    """
    service_account = service_account_pb2.ServiceAccount()
    service_account.id = 'dummy-account'
    res = service_account_service_pb2.ListServiceAccountsResponse()
    res.service_accounts.append(service_account)
    return res


def create_access_key():
    """
    Create access key service mock.
    """
    res = access_key_service_pb2.CreateAccessKeyResponse()
    res.access_key.id = 'dummy-id'
    res.access_key.key_id = 'dummy-access-key'
    res.secret = 'dummy-secret'
    return res


def create_key():
    """
    Create key service mock.
    """
    res = key_service_pb2.CreateKeyResponse()
    res.key.id = 'dummy-id'
    res.private_key = 'dummy-private-key'
    return res


def set_access_bindings(state):
    """
    Mock response of SetAccessBindings method.
    """
    return ret_grpc_operation(state)


def ret_grpc_operation(state, metadata=None, response=None):
    metadata_message = any_pb2.Any()
    if metadata is not None and not isinstance(metadata, dict):
        metadata_message.Pack(metadata)
    response_message = any_pb2.Any()
    if response is not None:
        response_message.Pack(response)

    operation_id = f'operation-{max([int(x.split("-")[1]) for x in state["compute"]["operations"]] + [0]) + 1}'
    state.setdefault('iam', {}).setdefault('operations', {})[operation_id] = {
        'id': operation_id,
        'metadata': metadata or {},
        'response': response,
        'done': True,
    }

    operation = operation_pb2.Operation(
        id=operation_id,
        metadata=metadata_message,
        response=response_message,
        done=True,
    )
    return operation


def get_operation(request, state):
    """
    Mock OperationService.Get.
    """
    element_obj = state['iam']['operations'].get(request.operation_id)
    if element_obj is None:
        err = NotFoundError(
            log,
            f'worker mock for iam operation {request.operation_id} not found error',
            err_type='NOT_FOUND',
            code=0,
        )
        raise err
    return operation_pb2.Operation(id=request.operation_id, done=True, response=element_obj['response'])


def iam(mocker, state):
    """
    Setup IAM provider service mocks
    """
    iam_token_service = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.iam.iam_token_service_pb2_grpc.IamTokenServiceStub'
    )
    iam_token_service.return_value.CreateForServiceAccount.side_effect = lambda *_args, **_kwargs: create_token()

    service_account_service = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.iam.service_account_service_pb2_grpc.ServiceAccountServiceStub'
    )
    service_account_service.return_value.Create.side_effect = lambda *_args, **_kwargs: create_service_account()
    service_account_service.return_value.List.side_effect = lambda *_args, **_kwargs: find_service_account()

    access_key_service = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.iam.access_key_service_pb2_grpc.AccessKeyServiceStub'
    )
    access_key_service.return_value.Create.side_effect = lambda *_args, **_kwargs: create_access_key()

    key_service = mocker.patch('cloud.mdb.dbaas_worker.internal.providers.iam.key_service_pb2_grpc.KeyServiceStub')
    key_service.return_value.Create.side_effect = lambda *_args, **_kwargs: create_key()

    access_binding_service = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.iam.access_binding_service_pb2_grpc.AccessBindingServiceStub'
    )
    access_binding_service.return_value.SetAccessBindings.side_effect = lambda *_args, **_kwargs: set_access_bindings(
        state
    )

    operation_service = mocker.patch(
        'cloud.mdb.internal.python.compute.iam.operations.api.operation_service_pb2_grpc.OperationServiceStub'
    )
    operation_service.return_value.Get.side_effect = lambda request, **_kwargs: get_operation(request, state)
