from unittest.mock import Mock

from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.loadbalancer.v1 import (
    target_group_pb2,
    target_group_service_pb2,
    network_load_balancer_pb2,
    network_load_balancer_service_pb2,
)
from .utils import handle_action


def get_operation(state, request):
    action_id = f'loadbalancer-get-operation-{request}'
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def stub(state, action_id):
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def get_target_group(state, request):
    action_id = f'target-group-get-{request}'
    handle_action(state, action_id)
    res = target_group_pb2.TargetGroup(id='test-target-group-id')
    return res


def get_network_load_balancer(state, request):
    action_id = f'network-load-balancer-get-{request}'
    handle_action(state, action_id)
    res = network_load_balancer_pb2.NetworkLoadBalancer(
        id='test-network-load-balancer-id',
        listeners=[network_load_balancer_pb2.Listener(address='ip_address_1')],
    )
    return res


def list_network_load_balancers(state, request):
    response = Mock()
    response.network_load_balancers = [get_network_load_balancer(state, request)]
    response.next_page_token = None
    return response


def list_get_target_groups(state, request):
    response = Mock()
    response.target_groups = [get_target_group(state, request)]
    response.next_page_token = None
    return response


def create_network_load_balancer(state, request):
    action_id = f'create-nlb-{request}'
    handle_action(state, action_id)
    meta = network_load_balancer_service_pb2.CreateNetworkLoadBalancerMetadata()
    meta.network_load_balancer_id = 'test-nlb-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def create_target_group(state, request):
    action_id = f'create-target-group-{request}'
    handle_action(state, action_id)
    meta = target_group_service_pb2.CreateTargetGroupMetadata()
    meta.target_group_id = 'test-target-group-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def loadbalancer(mocker, state):
    network_load_balancer_service = mocker.patch(
        'cloud.mdb.internal.python.loadbalancer.api.network_load_balancer_service_pb2_grpc'
        '.NetworkLoadBalancerServiceStub'
    ).return_value
    network_load_balancer_service.Create.side_effect = lambda request, timeout, metadata: create_network_load_balancer(
        state,
        request,
    )
    network_load_balancer_service.Get.side_effect = lambda request, timeout, metadata: get_network_load_balancer(
        state,
        request,
    )
    network_load_balancer_service.List.side_effect = lambda request, timeout, metadata: list_network_load_balancers(
        state,
        request,
    )
    network_load_balancer_service.Delete.side_effect = lambda request, timeout, metadata: stub(
        state,
        request,
    )

    target_group_service = mocker.patch(
        'cloud.mdb.internal.python.loadbalancer.api.target_group_service_pb2_grpc.TargetGroupServiceStub'
    ).return_value
    target_group_service.Get.side_effect = lambda request, timeout, metadata: get_target_group(state, request)
    target_group_service.Create.side_effect = lambda request, timeout, metadata: create_target_group(state, request)
    target_group_service.List.side_effect = lambda request, timeout, metadata: list_get_target_groups(state, request)
    target_group_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, request)

    operation_service = mocker.patch(
        'cloud.mdb.internal.python.loadbalancer.api.operation_service_pb2_grpc.OperationServiceStub'
    ).return_value
    operation_service.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
