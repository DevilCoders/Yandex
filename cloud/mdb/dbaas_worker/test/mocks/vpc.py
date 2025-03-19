"""
Simple vpc mock
"""

import grpc
from google.protobuf import any_pb2
from yandex.cloud.priv.operation import operation_pb2
from yandex.cloud.priv.vpc.v1 import network_service_pb2, network_pb2, subnet_pb2, security_group_pb2
from .utils import handle_action


def _fill_subnet(subnet, fields):
    subnet.id = fields['id']
    subnet.zone_id = fields['zoneId']
    for v4cidr in fields.get('v4CidrBlock', []):
        subnet.v4_cidr_blocks.append(v4cidr)
    for v6cidr in fields.get('v6CidrBlock', []):
        subnet.v6_cidr_blocks.append(v6cidr)
    subnet.folder_id = fields['folderId']
    subnet.network_id = fields['networkId']


def network_list_subnets(state, request):
    """
    ListSubnets mock
    """
    action_id = f'network_list_subnets_{request}'
    handle_action(state, action_id)
    res = network_service_pb2.ListNetworkSubnetsResponse()

    for fields in state['compute']['subnets'].values():
        if fields['networkId'] != request.network_id:
            continue
        subnet = res.subnets.add()
        _fill_subnet(subnet, fields)
    return res


def subnet_get(state, request):
    """
    ListSubnets mock
    """
    action_id = f'subnet_get_{request}'
    handle_action(state, action_id)
    if request.subnet_id not in state['compute']['subnets']:
        raise grpc.RpcError
    res = subnet_pb2.Subnet()
    _fill_subnet(res, state['compute']['subnets'][request.subnet_id])
    return res


def network_get(state, request):
    """
    Get network mock
    """
    action_id = f'network_get_{request}'
    handle_action(state, action_id)
    if request.network_id not in state['compute']['networks']:
        raise grpc.RpcError
    net = state['compute']['networks'][request.network_id]
    res = network_pb2.Network(
        id=net['id'],
        folder_id=net['folderId'],
        default_security_group_id=net['defaultSecurityGroupId'],
    )
    return res


def create_security_group(state, request):
    """
    Create security group mock
    """
    action_id = f'create_security_group_{request}'
    handle_action(state, action_id)
    op_id = f'sg_id_for_{request.name}'
    sgrules = []
    for rulespec in request.rule_specs:
        sgrules.append(
            security_group_pb2.SecurityGroupRule(
                id=op_id,
                direction=rulespec.direction,
                ports=rulespec.ports,
                protocol_name="TCP",
                protocol_number=6,
                cidr_blocks=rulespec.cidr_blocks,
            )
        )
    sg = security_group_pb2.SecurityGroup(
        id=f'sg_id_{request.name}',
        name=request.name,
        folder_id=request.folder_id,
        network_id=request.network_id,
        rules=sgrules,
    )
    state.setdefault('sgroup_create', {})[op_id] = sg
    res = any_pb2.Any()
    res.Pack(sg)
    return operation_pb2.Operation(id=op_id, done=True, response=res)


def get_operation(state, request):
    """
    Get operation mock
    """
    action_id = f'security_group-get-operation-{request}'
    handle_action(state, action_id)
    op_id = request.operation_id
    if op_id not in state.get('sgroup_create', {}):
        raise grpc.RpcError
    sg = state['sgroup_create'][op_id]
    res = any_pb2.Any()
    res.Pack(sg)
    return operation_pb2.Operation(id=op_id, done=True, response=res)


def vpc(mocker, state):
    """
    Setup vpc mock
    """
    net_stub = mocker.patch('cloud.mdb.internal.python.compute.vpc.api.network_service_pb2_grpc.NetworkServiceStub')
    subnet_stub = mocker.patch('cloud.mdb.internal.python.compute.vpc.api.subnet_service_pb2_grpc.SubnetServiceStub')
    sgroup_stub = mocker.patch(
        'cloud.mdb.internal.python.compute.vpc.api.security_group_service_pb2_grpc.SecurityGroupServiceStub'
    )
    operation_stub = mocker.patch(
        'cloud.mdb.internal.python.compute.vpc.api.operation_service_pb2_grpc.OperationServiceStub'
    )
    feature_flag_stub = mocker.patch(
        'cloud.mdb.internal.python.compute.vpc.api.feature_flag_service_pb2_grpc.FeatureFlagServiceStub'
    )

    net_stub.return_value.ListSubnets.side_effect = lambda request, *_args, **_kwargs: network_list_subnets(
        state, request
    )
    net_stub.return_value.Get.side_effect = lambda request, *_args, **_kwargs: network_get(state, request)
    subnet_stub.return_value.Get.side_effect = lambda request, *_args, **_kwargs: subnet_get(state, request)
    sgroup_stub.return_value.Create.side_effect = lambda request, *_args, **_kwargs: create_security_group(
        state, request
    )
    operation_stub.Get.side_effect = lambda request, timeout, metadata: get_operation(state, request)
    feature_flag_stub.AddToWhiteList.side_effect = (
        lambda request, *_args, **_kwargs: set_superflow_v22_flag_on_instance(state, request)
    )


def set_superflow_v22_flag_on_instance(state, request):
    """
    Set superflow v2.2 flag mock
    """
    action_id = f'set_superflow_v22_flag_on_instance_{request}'
    handle_action(state, action_id)
    op_id = request.operation_id
    return operation_pb2.Operation(id=op_id, done=True)
