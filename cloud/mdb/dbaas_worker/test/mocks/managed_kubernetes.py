from unittest.mock import Mock

from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.k8s.v1 import (
    cluster_pb2,
    cluster_service_pb2,
    node_pb2,
    node_group_pb2,
    node_group_service_pb2,
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


def create_node_group(state, request):
    action_id = f'node-group-create-{request}'
    handle_action(state, action_id)
    meta = node_group_service_pb2.CreateNodeGroupMetadata()
    meta.node_group_id = 'test-node-group-id'
    any_message = any_pb2.Any()
    any_message.Pack(meta)
    res = operation_pb2.Operation(metadata=any_message, id='test-operation-id')
    return res


def get_cluster(state, request):
    action_id = f'cluster-get-{request}'
    handle_action(state, action_id)
    res = cluster_pb2.Cluster(id='test-cluster-id')
    return res


def get_node_group(state, request):
    action_id = f'node-group-get-{request}'
    handle_action(state, action_id)
    res = node_group_pb2.NodeGroup(node_group_id='test-node-group-id')
    return res


def list_nodes(state, request):
    response = Mock()
    instance_dict = {
        'folderId': 'compute-folder-id',
        'id': 'instance_id_1',
        'fqdn': 'fqdn_1',
        'name': 'instance-name-1',
        'platformId': 'compute',
        'zoneId': 'zone',
        'labels': {},
        'metadata': {},
        'networkSettings': {
            'type': 'STANDARD',
        },
        'resources': {
            'memory': 2 * 1024**3,
            'cores': 2,
            'coreFraction': 100,
            'gpus': 0,
            'sockets': 0,
            'nvmeDisks': 0,
        },
        'bootDisk': {
            'autoDelete': True,
            'deviceName': 'boot',
            'diskId': 'disk-id',
            'mode': 'READ_WRITE',
            'status': 'ATTACHED',
        },
        'secondaryDisks': [],
        'networkInterfaces': [
            {
                'subnetId': 'subnet_0',
                'primaryV4Address': {'address': 'ip_addr_0'},
                'index': 0,
            },
            {
                'subnetId': 'subnet_1',
                'primaryV4Address': {'address': 'ip_addr_1'},
                'index': 1,
            },
        ],
        'status': 'RUNNING',
    }
    state['compute']['instances'] = {
        'instance_id_1': instance_dict,
    }
    response.nodes = [node_pb2.Node(cloud_status=node_pb2.Node.CloudStatus(id='instance_id_1'))]
    response.next_page_token = None
    return response


def get_operation(state, request):
    action_id = f'managed_kubernetes-get-operation-{request}'
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def stub(state, action_id):
    handle_action(state, action_id)
    res = operation_pb2.Operation(id='test-operation-id', done=True)
    return res


def managed_kubernetes(mocker, state):
    cluster_service = mocker.patch(
        'cloud.mdb.internal.python.managed_kubernetes.api.' 'cluster_service_pb2_grpc.ClusterServiceStub'
    ).return_value
    cluster_service.Create.side_effect = lambda request, timeout, metadata: create_cluster(state, request)
    cluster_service.Get.side_effect = lambda request, timeout, metadata: get_cluster(state, request)
    cluster_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'cluster-delete')
    cluster_service.List.side_effect = lambda request, timeout, metadata: stub(state, 'cluster-list')

    node_group_service = mocker.patch(
        'cloud.mdb.internal.python.managed_kubernetes.api.' 'node_group_service_pb2_grpc.NodeGroupServiceStub'
    ).return_value
    node_group_service.Create.side_effect = lambda request, timeout, metadata: create_node_group(state, request)
    node_group_service.Get.side_effect = lambda request, timeout, metadata: get_node_group(state, request)
    node_group_service.Delete.side_effect = lambda request, timeout, metadata: stub(state, 'node-group-delete')
    node_group_service.List.side_effect = lambda request, timeout, metadata: stub(state, 'node-group-list')
    node_group_service.ListNodes.side_effect = lambda request, timeout, metadata: list_nodes(state, 'list-nodes')

    operation_service = mocker.patch(
        'cloud.mdb.internal.python.managed_kubernetes.api.' 'operation_service_pb2_grpc.OperationServiceStub'
    ).return_value
    operation_service.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
