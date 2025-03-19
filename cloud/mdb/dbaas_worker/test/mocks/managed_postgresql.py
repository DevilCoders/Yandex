from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.mdb.postgresql.v1 import (
    cluster_pb2,
    cluster_service_pb2,
    database_pb2,
    database_service_pb2,
    user_pb2,
    user_service_pb2,
)

from .utils import handle_action


def create_cluster(state, request):
    action_id = f'cluster-create-{request}'
    handle_action(state, action_id)
    meta = cluster_service_pb2.CreateClusterMetadata()
    meta.cluster_id = 'test-cluster-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def create_database(state, request):
    action_id = f'database-create-{request}'
    handle_action(state, action_id)
    meta = database_service_pb2.CreateDatabaseMetadata()
    meta.database_name = 'test-database-name'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def create_user(state, request):
    action_id = f'user-create-{request}'
    handle_action(state, action_id)
    meta = user_service_pb2.CreateUserMetadata()
    meta.user_name = 'test-user-name'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def get_cluster(state, request):
    action_id = f'cluster-get-{request}'
    handle_action(state, action_id)
    res = cluster_pb2.Cluster(
        id='test-cluster-id',
        status=cluster_pb2.Cluster.Status.RUNNING,
        health=cluster_pb2.Cluster.Health.ALIVE,
    )
    return res


def get_database(state, request):
    action_id = f'database-get-{request}'
    handle_action(state, action_id)
    res = database_pb2.Database(name='test-database-name')
    return res


def get_user(state, request):
    action_id = f'user-get-{request}'
    handle_action(state, action_id)
    res = user_pb2.User(name='test-user-name')
    return res


def get_operation(state, request):
    action_id = f'managed_postgresql-get-operation-{request}'
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def stub(state, action_id):
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def managed_postgresql(mocker, state):
    cluster_service = mocker.patch(
        'cloud.mdb.internal.python.managed_postgresql.api.' 'cluster_service_pb2_grpc.ClusterServiceStub'
    ).return_value
    cluster_service.Create.side_effect = lambda request, timeout, metadata: create_cluster(state, request)
    cluster_service.Get.side_effect = lambda request, timeout, metadata: get_cluster(state, request)
    cluster_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'cluster-delete')
    cluster_service.List.side_effect = lambda request, timeout, metadata: stub(state, 'cluster-list')

    database_service = mocker.patch(
        'cloud.mdb.internal.python.managed_postgresql.api.' 'database_service_pb2_grpc.DatabaseServiceStub'
    ).return_value
    database_service.Create.side_effect = lambda request, timeout, metadata: create_database(state, request)
    database_service.Get.side_effect = lambda request, timeout, metadata: get_database(state, request)
    database_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'database-delete')
    database_service.List.side_effect = lambda request, timeout, metadata: stub(state, 'database-list')

    user_service = mocker.patch(
        'cloud.mdb.internal.python.managed_postgresql.api.' 'user_service_pb2_grpc.UserServiceStub'
    ).return_value
    user_service.Create.side_effect = lambda request, timeout, metadata: create_user(state, request)
    user_service.Get.side_effect = lambda request, timeout, metadata: get_user(state, request)
    user_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'user-delete')
    user_service.List.side_effect = lambda request, timeout, metadata: stub(state, 'user-list')

    operation_service = mocker.patch(
        'cloud.mdb.internal.python.managed_postgresql.api.' 'operation_service_pb2_grpc.OperationServiceStub'
    ).return_value
    operation_service.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
