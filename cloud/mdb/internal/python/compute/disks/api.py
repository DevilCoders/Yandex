from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Dict, Generator, Iterable, NamedTuple, Optional
from yandex.cloud.priv.compute.v1 import disk_pb2, disk_service_pb2, disk_service_pb2_grpc

from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.pagination import ComputeResponse, paginate
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import CreatedDisk, DiskModel


class DisksClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class DisksClient:
    __disk_service = None

    def __init__(
        self,
        config: DisksClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='DisksClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @property
    def _disk_service(self):
        if self.__disk_service is None:
            channel = grpcutil.new_grpc_channel(self.config.transport)
            self.__disk_service = ComputeGRPCService(
                self.logger,
                channel,
                disk_service_pb2_grpc.DiskServiceStub,
                timeout=self.config.timeout,
                get_token=self.token_getter,
                error_handlers=self.error_handlers,
            )
        return self.__disk_service

    @client_retry
    @tracing.trace('Compute Get Disk')
    def get_disk(self, disk_id: str) -> DiskModel:
        """
        Get disk by id
        """
        tracing.set_tag('compute.disk.id', disk_id)
        request = disk_service_pb2.GetDiskRequest()
        request.disk_id = disk_id
        return DiskModel.from_api(self._disk_service.Get(request))

    @client_retry
    @tracing.trace('Compute List Disks page')
    def _list_disks(self, request: disk_service_pb2.ListDisksRequest) -> Iterable[disk_pb2.Disk]:
        tracing.set_tag('compute.folder.id', request.folder_id)
        response = self._disk_service.List(request)
        return ComputeResponse(
            resources=map(DiskModel.from_api, response.disks),
            next_page_token=response.next_page_token,
        )

    def list_disks(self, folder_id: str) -> Generator[DiskModel, None, None]:
        """
        Get disks in folder
        """
        request = disk_service_pb2.ListDisksRequest()
        request.page_size = self.config.page_size
        request.folder_id = folder_id
        return paginate(self._list_disks, request)

    @client_retry
    @tracing.trace('Compute Delete Disk')
    def delete_disk(self, disk_id: str, idempotency_key: str) -> OperationModel:
        tracing.set_tag('compute.disk.id', disk_id)
        request = disk_service_pb2.DeleteDiskRequest()
        request.disk_id = disk_id
        return OperationModel().operation_from_api(
            self.logger,
            self._disk_service.Delete(request, idempotency_key=idempotency_key),
            self._disk_service.error_from_rich_status,
        )

    @tracing.trace('Compute Disk Set Size')
    def set_disk_size(self, disk_id: str, size: int, idempotency_key: str) -> OperationModel:
        tracing.set_tag('compute.disk.size', size)
        tracing.set_tag('compute.disk.id', disk_id)
        return self.update_disk(
            disk_id,
            size=size,
            idempotency_key=idempotency_key,
        )

    @client_retry
    @tracing.trace('Compute Create Disk')
    def create_disk(
        self,
        geo: str,
        size: int,
        type_id: str,
        folder_id: str,
        zone_id: str,
        idempotency_key: str,
        image_id: Optional[str] = None,
        disk_placement_group_id: str = None,
    ) -> CreatedDisk:
        """
        Create disk
        """
        tracing.set_tag('compute.disk.geo', geo)
        tracing.set_tag('compute.disk.size', size)
        tracing.set_tag('compute.disk.type.id', type_id)

        request = disk_service_pb2.CreateDiskRequest()
        request.folder_id = folder_id
        request.zone_id = zone_id
        request.type_id = type_id
        request.size = size
        if image_id:
            request.image_id = image_id
        if disk_placement_group_id is not None:
            request.disk_placement_policy.placement_group_id = disk_placement_group_id
        operation = self._disk_service.Create(request, idempotency_key=idempotency_key)

        metadata = disk_service_pb2.CreateDiskMetadata()
        operation.metadata.Unpack(metadata)

        return CreatedDisk(disk_id=metadata.disk_id).operation_from_api(
            self.logger,
            operation,
            self._disk_service.error_from_rich_status,
        )

    @client_retry
    def update_disk(self, disk_id: str, size: int, idempotency_key: str) -> OperationModel:
        fields_to_update = set()
        request = disk_service_pb2.UpdateDiskRequest()
        request.disk_id = disk_id
        fields_to_update.add('size')
        request.size = size
        request.update_mask.paths.extend(fields_to_update)
        return OperationModel().operation_from_api(
            self.logger,
            self._disk_service.Update(request, idempotency_key=idempotency_key),
            self._disk_service.error_from_rich_status,
        )
