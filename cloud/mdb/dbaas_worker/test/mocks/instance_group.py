"""
Simple instance service mock
"""

from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.microcosm.instancegroup.v1 import (
    instance_group_pb2,
    instance_group_service_pb2,
)
from .utils import handle_action


def create(state, request):
    """
    Create instance group mock
    """
    action_id = f'instance_group-create-{request}'
    handle_action(state, action_id)
    meta = instance_group_service_pb2.CreateInstanceGroupMetadata()
    meta.instance_group_id = 'test-instance-group-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def get(state, request):
    """
    Get instance group mock
    """
    action_id = f'instance_group-get-{request}'
    handle_action(state, action_id)
    res = instance_group_pb2.InstanceGroup(id='test-instance-group-id')
    return res


def get_operation(state, request):
    """
    Get operation mock
    """
    action_id = f'instance_group-get-operation-{request}'
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def stub(state, action_id):
    """
    Simple stub
    """
    handle_action(state, action_id)


def instance_group(mocker, state):
    """
    Setup instance group service mock
    """
    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.instance_group.'
        'instance_group_service_pb2_grpc.InstanceGroupServiceStub'
    ).return_value
    stub.CreateFromYaml.side_effect = lambda request, timeout, metadata: create(state, request)
    stub.Get.side_effect = lambda request, timeout, metadata: get(state, request)
    stub.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-delete')
    stub.List.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-list')
    stub.ListInstances.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-list-instances')
    stub.Stop.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-stop')
    stub.Start.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-start')
    stub.UpdateFromYaml.side_effect = lambda request, timeout, metadata: stub(state, 'instance-group-update')

    stub = mocker.patch(
        'cloud.mdb.dbaas_worker.internal.providers.instance_group.' 'operation_service_pb2_grpc.OperationServiceStub'
    ).return_value
    stub.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
