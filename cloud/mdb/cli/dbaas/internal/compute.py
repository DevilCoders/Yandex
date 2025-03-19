from google.protobuf.field_mask_pb2 import FieldMask

from cloud.mdb.cli.dbaas.internal.grpc import grpc_service, grpc_request
from cloud.mdb.cli.dbaas.internal.iam import get_iam_token
from yandex.cloud.priv.compute.v1.disk_service_pb2 import GetDiskRequest, ListDisksRequest
from yandex.cloud.priv.compute.v1.disk_service_pb2_grpc import DiskServiceStub
from yandex.cloud.priv.compute.v1.instance_service_pb2 import (
    GetInstanceRequest,
    FULL,
    RestartInstanceRequest,
    UpdateInstanceMetadataRequest,
    UpdateInstanceNetworkInterfaceRequest,
)
from yandex.cloud.priv.compute.v1.instance_service_pb2_grpc import InstanceServiceStub
from yandex.cloud.priv.vpc.v1.security_group_service_pb2 import GetSecurityGroupRequest, ListSecurityGroupsRequest
from yandex.cloud.priv.vpc.v1.security_group_service_pb2_grpc import SecurityGroupServiceStub


def get_instance(ctx, instance_id):
    request = GetInstanceRequest(instance_id=instance_id, view=FULL)

    service = grpc_service(ctx, 'compute', InstanceServiceStub)

    return grpc_request(ctx, service.Get, request)


def update_instance_metadata(ctx, instance_id, *, delete=None, upsert=None):
    request = UpdateInstanceMetadataRequest(instance_id=instance_id, delete=delete, upsert=upsert)

    service = grpc_service(ctx, 'compute', InstanceServiceStub, get_iam_token(ctx, 'worker'))

    return grpc_request(ctx, service.UpdateMetadata, request)


def update_instance_security_groups(ctx, instance_id, security_group_ids):
    request = UpdateInstanceNetworkInterfaceRequest(
        instance_id=instance_id,
        network_interface_index="0",
        security_group_ids=security_group_ids,
        update_mask=FieldMask(paths=['security_group_ids']),
    )

    service = grpc_service(ctx, 'compute', InstanceServiceStub, get_iam_token(ctx, 'worker'))

    return grpc_request(ctx, service.UpdateNetworkInterface, request)


def restart_instance(ctx, instance_id):
    request = RestartInstanceRequest(instance_id=instance_id)

    service = grpc_service(ctx, 'compute', InstanceServiceStub, get_iam_token(ctx, 'worker'))

    return grpc_request(ctx, service.Restart, request)


def get_disk(ctx, disk_id):
    request = GetDiskRequest(disk_id=disk_id)

    service = grpc_service(ctx, 'compute', DiskServiceStub)

    return grpc_request(ctx, service.Get, request)


def get_disks(ctx, folder_id):
    request = ListDisksRequest(folder_id=folder_id)

    service = grpc_service(ctx, 'compute', DiskServiceStub)

    return grpc_request(ctx, service.List, request)


def get_security_group(ctx, security_group_id):
    request = GetSecurityGroupRequest(security_group_id=security_group_id)

    service = grpc_service(ctx, 'vpc', SecurityGroupServiceStub)

    return grpc_request(ctx, service.Get, request)


def get_security_groups(ctx, folder_id):
    request = ListSecurityGroupsRequest(folder_id=folder_id)

    service = grpc_service(ctx, 'vpc', SecurityGroupServiceStub)

    return grpc_request(ctx, service.List, request)
