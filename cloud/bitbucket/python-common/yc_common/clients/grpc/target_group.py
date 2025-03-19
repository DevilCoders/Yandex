from google.protobuf import field_mask_pb2
from yandex.cloud.priv.loadbalancer.v1 import (
    target_group_service_pb2,
    target_group_service_pb2_grpc,
)
from yc_common import config, logging
from yc_common.clients.grpc.base import auth_metadata, BaseGrpcClient, get_client, GrpcEndpointConfig
from yc_common.clients.models import target_groups as target_group_models
from yc_common.clients.models.grpc.common import OperationListGrpc

log = logging.get_logger(__name__)


class TargetGroupClient(BaseGrpcClient):
    service_stub_cls = target_group_service_pb2_grpc.TargetGroupServiceStub

    def get(self, target_group_id) -> target_group_models.TargetGroup:
        request = target_group_service_pb2.GetTargetGroupRequest(target_group_id=target_group_id)
        return target_group_models.TargetGroup.from_grpc(self.reading_stub.Get(request))

    def list(self, folder_id, filter=None, page_size=None, page_token=None) -> target_group_models.TargetGroupList:
        request = target_group_service_pb2.ListTargetGroupsRequest(
            folder_id=folder_id,
            filter=filter,
            page_size=page_size,
            page_token=page_token,
        )
        return target_group_models.TargetGroupList.from_grpc(self.reading_stub.List(request))

    def create(
        self,
        folder_id,
        name=None,
        description=None,
        labels=None,
        region_id=None,
        targets=None,
    ) -> target_group_models.TargetGroupOperation:
        request = target_group_service_pb2.CreateTargetGroupRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            labels=labels,
            region_id=region_id,
            targets=targets,
        )
        return target_group_models.TargetGroupOperation.from_grpc(self.writing_stub.Create(request))

    def update(
        self,
        target_group_id,
        update_mask,
        name=None,
        description=None,
        labels=None,
        targets=None,
    ) -> target_group_models.TargetGroupOperation:
        request = target_group_service_pb2.UpdateTargetGroupRequest(
            target_group_id=target_group_id,
            update_mask=field_mask_pb2.FieldMask(paths=update_mask),
            name=name,
            description=description,
            labels=labels,
            targets=targets,
        )
        return target_group_models.TargetGroupOperation.from_grpc(self.writing_stub.Update(request))

    def delete(self, target_group_id) -> target_group_models.TargetGroupOperation:
        request = target_group_service_pb2.DeleteTargetGroupRequest(target_group_id=target_group_id)
        return target_group_models.TargetGroupOperation.from_grpc(self.writing_stub.Delete(request))

    def add_targets(self, target_group_id, targets) -> target_group_models.TargetGroupOperation:
        request = target_group_service_pb2.AddTargetsRequest(
            target_group_id=target_group_id,
            targets=targets,
        )
        return target_group_models.TargetGroupOperation.from_grpc(self.writing_stub.AddTargets(request))

    def remove_targets(self, target_group_id, targets) -> target_group_models.TargetGroupOperation:
        request = target_group_service_pb2.RemoveTargetsRequest(
            target_group_id=target_group_id,
            targets=targets,
        )
        return target_group_models.TargetGroupOperation.from_grpc(self.writing_stub.RemoveTargets(request))

    def list_operations(self, target_group_id, page_size=None, page_token=None) -> OperationListGrpc:
        request = target_group_service_pb2.ListTargetGroupOperationsRequest(
            target_group_id=target_group_id,
            page_size=page_size,
            page_token=page_token,
        )
        return OperationListGrpc.from_grpc(self.reading_stub.ListOperations(request))


def get_target_group_client(iam_token) -> TargetGroupClient:
    return get_client(
        TargetGroupClient,
        metadata=auth_metadata(iam_token),
        endpoint_config=config.get_value("endpoints.loadbalancer", model=GrpcEndpointConfig),
    )
