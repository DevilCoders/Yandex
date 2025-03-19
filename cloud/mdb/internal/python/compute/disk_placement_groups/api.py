from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, List, Generator, NamedTuple
from yandex.cloud.priv.compute.v1 import (
    disk_placement_group_service_pb2,
    disk_placement_group_pb2,
    disk_placement_group_service_pb2_grpc,
)
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.disks import DiskModel
from cloud.mdb.internal.python.compute.pagination import ComputeResponse, paginate
from .grpc_service import DiskPlacementGroupsGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import DiskPlacementGroupModel


class DiskPlacementGroupClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class DiskPlacementGroupClient:
    __disk_placement_group_service = None

    def __init__(
        self,
        config: DiskPlacementGroupClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='DiskPlacementGroupClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _disk_placement_group_service(self) -> DiskPlacementGroupsGRPCService:
        if self.__disk_placement_group_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__disk_placement_group_service = DiskPlacementGroupsGRPCService(
                self.logger,
                channel,
                disk_placement_group_service_pb2_grpc.DiskPlacementGroupServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__disk_placement_group_service

    @client_retry
    @tracing.trace('Disk Placement Group Listed')
    def get_by_name(self, folder_id: str, name: str) -> List[DiskPlacementGroupModel]:
        """
        Get disk placement group by name.
        """
        tracing.set_tag('compute.disk_placement_group.name', name)
        return list(self.list_groups(folder_id=folder_id, filter_params=dict(name=name)))

    @client_retry
    @tracing.trace('Disk Placement Group Get')
    def get_by_id(self, group_id: str) -> DiskPlacementGroupModel:
        """
        Get disk placement group by id.
        """
        tracing.set_tag('compute.disk_placement_group.id', group_id)
        request = disk_placement_group_service_pb2.GetDiskPlacementGroupRequest()
        request.disk_placement_group_id = group_id
        response = self._disk_placement_group_service.Get(request)
        return DiskPlacementGroupModel.from_api(response)

    @client_retry
    @tracing.trace('Disk Placement Group list page')
    def __list_groups(
        self, request: disk_placement_group_service_pb2.ListDiskPlacementGroupsRequest
    ) -> ComputeResponse:
        tracing.set_tag('compute.folder.id', request.folder_id)
        response = self._disk_placement_group_service.List(request)
        return ComputeResponse(
            resources=map(DiskPlacementGroupModel.from_api, response.disk_placement_groups),
            next_page_token=response.next_page_token,
        )

    def list_groups(self, folder_id: str, filter_params: dict) -> Generator[DiskPlacementGroupModel, None, None]:
        request = disk_placement_group_service_pb2.ListDiskPlacementGroupsRequest()
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
        return paginate(self.__list_groups, request)

    def __list_disks(
        self, request: disk_placement_group_service_pb2.ListDiskPlacementGroupDisksRequest
    ) -> ComputeResponse:
        tracing.set_tag('compute.disk_placement_group.id', request.disk_placement_group_id)
        response = self._disk_placement_group_service.ListDisks(request)
        return ComputeResponse(
            resources=map(DiskModel.from_api, response.disks),
            next_page_token=response.next_page_token,
        )

    @client_retry
    @tracing.trace('Disks in Disk Placement Group listed')
    def list_disks(self, disk_placement_group_id: str) -> Generator[DiskModel, None, None]:
        """
        Get disks assigned to a placement group
        """
        tracing.set_tag('compute.disk_placement_group.id', disk_placement_group_id)
        request = disk_placement_group_service_pb2.ListDiskPlacementGroupDisksRequest()
        request.disk_placement_group_id = disk_placement_group_id
        request.page_size = self.config.page_size
        return paginate(self.__list_disks, request)

    @client_retry
    @tracing.trace('Disk Placement Group created')
    def create_disk_placement_group(
        self, idempotency_key: str, name: str, folder_id: str, zone_id: str
    ) -> OperationModel:
        """
        Create Disk Placement Group
        """
        tracing.set_tag('compute.disk_placement_group.name', name)
        request = disk_placement_group_service_pb2.CreateDiskPlacementGroupRequest()
        request.folder_id = folder_id
        request.name = name
        request.zone_id = zone_id
        request.spread_placement_strategy.CopyFrom(disk_placement_group_pb2.DiskSpreadPlacementStrategy())
        operation = self._disk_placement_group_service.Create(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._disk_placement_group_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('Disk Placement Group deleted')
    def delete_disk_placement_group(self, idempotency_key: str, dpg_id: str) -> OperationModel:
        """
        Delete disk placement group.
        """
        tracing.set_tag('compute.disk_placement_group.id', dpg_id)
        request = disk_placement_group_service_pb2.DeleteDiskPlacementGroupRequest()
        request.disk_placement_group_id = dpg_id
        operation = self._disk_placement_group_service.Delete(request, idempotency_key=idempotency_key)
        return OperationModel().operation_from_api(
            self.logger, operation, self._disk_placement_group_service.error_from_rich_status
        )
