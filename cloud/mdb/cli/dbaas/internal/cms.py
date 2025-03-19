from cloud.mdb.cli.dbaas.internal.grpc import grpc_request, grpc_service
from cloud.mdb.cms.api.grpcapi.v1.instance_operation_service_pb2 import (
    GetInstanceOperationRequest,
    ListInstanceOperationsRequest,
)
from cloud.mdb.cms.api.grpcapi.v1.instance_operation_service_pb2_grpc import InstanceOperationServiceStub
from cloud.mdb.cms.api.grpcapi.v1.instance_service_pb2 import MoveInstanceRequest
from cloud.mdb.cms.api.grpcapi.v1.instance_service_pb2_grpc import InstanceServiceStub


def get_operation(ctx, operation_id):
    request = GetInstanceOperationRequest(id=operation_id)

    service = grpc_service(ctx, 'cms', InstanceOperationServiceStub)

    return grpc_request(ctx, service.Get, request)


def get_operations(ctx):
    request = ListInstanceOperationsRequest()

    service = grpc_service(ctx, 'cms', InstanceOperationServiceStub)

    return grpc_request(ctx, service.List, request)


def resetup_instance(ctx, instance_id, comment):
    request = MoveInstanceRequest(instance_id=instance_id, comment=comment)

    service = grpc_service(ctx, 'cms', InstanceServiceStub)

    return grpc_request(ctx, service.MoveInstance, request)
