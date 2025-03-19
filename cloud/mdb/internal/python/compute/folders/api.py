from contextlib import AbstractContextManager
from dbaas_common import tracing
from typing import Callable, Dict, NamedTuple, List, Type, Generator
from yandex.cloud.priv.resourcemanager.v1 import folder_service_pb2, folder_service_pb2_grpc
from yandex.cloud.priv.access import access_pb2

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.pagination import paginate, ComputeResponse
from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .models import ResolvedFolder, AccessBinding


class FoldersClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class FoldersClient(AbstractContextManager):
    __folder_service = None

    def __init__(
        self,
        config: FoldersClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[Type[GRPCError], Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='FoldersClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    def __enter__(self) -> 'FoldersClient':
        channel = grpcutil.new_grpc_channel(self.config.transport)
        self.__folder_service = ComputeGRPCService(
            self.logger,
            channel,
            folder_service_pb2_grpc.FolderServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__folder_service.channel.close()
        self.__folder_service = None

    @property
    def _folder_service(self):
        return self.__folder_service

    @client_retry
    @tracing.trace('Resource Manager Resolve Folders')
    def resolve(self, folder_ids: List[str]) -> List[ResolvedFolder]:
        tracing.set_tag('cloud.folder.ids', folder_ids)

        request = folder_service_pb2.ResolveFoldersRequest()
        request.folder_ids.extend(folder_ids)
        request.resolve_existing_only = True
        response = self._folder_service.Resolve(request)
        return list(map(ResolvedFolder.from_api, response.resolved_folders))

    @client_retry
    @tracing.trace('Resource Manager List Access Bindings Page')
    def _list_access_bindings_page(self, request: access_pb2.ListAccessBindingsRequest) -> ComputeResponse:
        tracing.set_tag('cloud.folder.id', request.resource_id)
        response = self._folder_service.ListAccessBindings(request)
        return ComputeResponse(
            resources=map(AccessBinding.from_api, response.access_bindings),
            next_page_token=response.next_page_token,
        )

    @tracing.trace('Resource Manager List Access Bindings')
    def list_access_bindings(self, folder_id: str) -> Generator[AccessBinding, None, None]:
        tracing.set_tag('cloud.folder.id', folder_id)

        request = access_pb2.ListAccessBindingsRequest()
        request.resource_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_access_bindings_page, request)
