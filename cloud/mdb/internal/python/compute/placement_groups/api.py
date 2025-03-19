from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, Generator, NamedTuple, Iterable, List
from yandex.cloud.priv.compute.v1 import (
    placement_group_service_pb2,
    placement_group_pb2,
    placement_group_service_pb2_grpc,
)
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.pagination import ComputeResponse, paginate
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.internal.python.compute.instances import InstanceModel
from .models import PlacementGroupModel
from .grpc_service import PlacementGroupsGRPCService


class PlacementGroupClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class PlacementGroupClient:
    __placement_group_service = None

    def __init__(
        self,
        config: PlacementGroupClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='PlacementGroupClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _placement_group_service(self) -> PlacementGroupsGRPCService:
        if self.__placement_group_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__placement_group_service = PlacementGroupsGRPCService(
                self.logger,
                channel,
                placement_group_service_pb2_grpc.PlacementGroupServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__placement_group_service

    @client_retry
    @tracing.trace('Placement Group list page')
    def _list_groups(self, request: placement_group_service_pb2.ListPlacementGroupsRequest) -> Iterable:
        tracing.set_tag('compute.folder.id', request.folder_id)
        response = self._placement_group_service.List(request)
        return ComputeResponse(
            resources=map(PlacementGroupModel.from_api, response.placement_groups),
            next_page_token=response.next_page_token,
        )

    def list_groups(self, folder_id: str, filter_params: dict) -> Generator[PlacementGroupModel, None, None]:
        request = placement_group_service_pb2.ListPlacementGroupsRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        try:
            name = filter_params.pop('name')
        except KeyError:
            pass
        else:
            request.filter = f'name="{name}"'
        if len(filter_params) > 0:
            raise NotImplementedError('unsupported filter params {}'.format(','.join(filter_params.keys())))
        return paginate(self._list_groups, request)

    @client_retry
    @tracing.trace('Placement Group created')
    def create_placement_group(
        self, idempotency_key: str, name: str, folder_id: str, best_effort: bool
    ) -> OperationModel:
        """
        Create Placement Group
        """
        tracing.set_tag('compute.placement_group.name', name)
        request = placement_group_service_pb2.CreatePlacementGroupRequest()
        request.folder_id = folder_id
        request.name = name
        request.spread_placement_strategy.CopyFrom(placement_group_pb2.SpreadPlacementStrategy())
        request.spread_placement_strategy.best_effort = best_effort
        operation = self._placement_group_service.Create(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._placement_group_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Placement Group deleted')
    def delete_placement_group(self, idempotency_key: str, pg_id: str) -> OperationModel:
        """
        Delete placement group.
        """
        tracing.set_tag('compute.placement_group.id', pg_id)
        request = placement_group_service_pb2.DeletePlacementGroupRequest()
        request.placement_group_id = pg_id
        operation = self._placement_group_service.Delete(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger, operation, self._placement_group_service.error_from_rich_status
        )

    @client_retry
    @tracing.trace('Placement Group Listed')
    def get_by_name(self, folder_id: str, name: str) -> List[PlacementGroupModel]:
        """
        Get placement group by name.
        """
        tracing.set_tag('compute.placement_group.name', name)
        return list(self.list_groups(folder_id=folder_id, filter_params=dict(name=name)))

    def __list_vms(self, request: placement_group_service_pb2.ListPlacementGroupInstancesRequest) -> ComputeResponse:
        tracing.set_tag('compute.placement_group.id', request.placement_group_id)
        response = self._placement_group_service.ListInstances(request)
        return ComputeResponse(
            resources=map(InstanceModel.from_api, response.instances),
            next_page_token=response.next_page_token,
        )

    @client_retry
    @tracing.trace('VMs in Placement Group listed')
    def list_vms(self, placement_group_id: str) -> Generator[InstanceModel, None, None]:
        """
        Get vms assigned to a placement group
        """
        tracing.set_tag('compute.placement_group.id', placement_group_id)
        request = placement_group_service_pb2.ListPlacementGroupInstancesRequest()
        request.placement_group_id = placement_group_id
        request.page_size = self.config.page_size
        return paginate(self.__list_vms, request)

    @client_retry
    @tracing.trace('Compute Get Placement Group info')
    def get_placement_group(self, placement_group_id: str) -> PlacementGroupModel:
        """
        Get placement group info
        """
        tracing.set_tag('compute.placement_group.id', placement_group_id)
        request = placement_group_service_pb2.GetPlacementGroupRequest()
        request.placement_group_id = placement_group_id
        return PlacementGroupModel.from_api(self._placement_group_service.Get(request))
