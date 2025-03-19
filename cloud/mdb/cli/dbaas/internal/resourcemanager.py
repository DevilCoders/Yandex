from cloud.mdb.cli.dbaas.internal.grpc import grpc_request, grpc_service
from cloud.mdb.cli.dbaas.internal.iam import get_iam_token
from yandex.cloud.priv.resourcemanager.v1.cloud_service_pb2 import GetPermissionStagesRequest
from yandex.cloud.priv.resourcemanager.v1.cloud_service_pb2_grpc import CloudServiceStub


def get_feature_flags(ctx, cloud_id):
    request = GetPermissionStagesRequest(cloud_id=cloud_id)

    service = grpc_service(ctx, 'resourcemanager', CloudServiceStub, get_iam_token(ctx, 'worker'))

    return grpc_request(ctx, service.GetPermissionStages, request).permission_stages
