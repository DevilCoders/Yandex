import functools
import time

from cloud.mdb.internal.python import grpcutil
from typing import Callable, Dict, NamedTuple, Optional, Iterable, Tuple
from .models import (
    Cluster,
    Database,
    DatabaseSpec,
    User,
    UserSpec,
    ClusterStatus,
    ClusterHealth,
)
from google.protobuf import wrappers_pb2

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from dbaas_common import tracing
from yandex.cloud.priv.mdb.postgresql.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
    database_pb2,
    database_service_pb2,
    database_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
    user_pb2,
    user_service_pb2,
    user_service_pb2_grpc,
)

from ..compute.models import OperationModel
from ..compute.pagination import ComputeResponse, paginate
from ..grpcutil import WrappedGRPCService


class ManagedPostgresqlClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class ManagedPostgresqlClient:
    def __init__(
        self,
        config: ManagedPostgresqlClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='ManagedPostgresqlClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @functools.cached_property
    def cluster_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            cluster_service_pb2_grpc.ClusterServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @functools.cached_property
    def database_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            database_service_pb2_grpc.DatabaseServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @functools.cached_property
    def user_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            user_service_pb2_grpc.UserServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @functools.cached_property
    def operation_service(self):
        return WrappedGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            operation_service_pb2_grpc.OperationServiceStub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @tracing.trace('Wait operation')
    def wait_operation(self, operation_id: str, timeout: int = 900) -> OperationModel:
        deadline = time.time() + timeout
        while time.time() < deadline:
            operation = self.operation_service.Get(
                operation_service_pb2.GetOperationRequest(operation_id=operation_id),
                timeout=self.config.timeout,
            )
            if operation.done:
                return operation
            time.sleep(1)

    @tracing.trace('Wait alive status')
    def wait_alive_status(self, cluster_id: str, timeout: int = 900) -> Cluster:
        deadline = time.time() + timeout
        while time.time() < deadline:
            cluster = self.get_cluster(cluster_id)
            if cluster.status == ClusterStatus.RUNNING and cluster.health == ClusterHealth.ALIVE:
                return cluster
            time.sleep(1)
        raise TimeoutError(f'Cluster did not became alive in {timeout} seconds.')

    @client_retry
    @tracing.trace('Get postgresql cluster')
    def get_cluster(self, cluster_id: str) -> Cluster:
        tracing.set_tag('cluster_id', cluster_id)
        request = cluster_service_pb2.GetClusterRequest(cluster_id=cluster_id)
        return Cluster.from_api(self.cluster_service.Get(request))

    @client_retry
    @tracing.trace('Get postgresql database')
    def get_database(self, cluster_id: str, database_name: str) -> Database:
        tracing.set_tag('cluster_id', cluster_id)
        request = database_service_pb2.GetDatabaseRequest(cluster_id=cluster_id, database_name=database_name)
        return Database.from_api(self.database_service.Get(request))

    @client_retry
    @tracing.trace('Get postgresql user')
    def get_user(self, cluster_id: str, user_name: str) -> User:
        tracing.set_tag('cluster_id', cluster_id)
        request = user_service_pb2.GetUserRequest(cluster_id=cluster_id, user_name=user_name)
        return User.from_api(self.user_service.Get(request))

    @client_retry
    @tracing.trace('List postgresql clusters')
    def _list_clusters(self, request: cluster_service_pb2.ListClustersRequest) -> Iterable:
        tracing.set_tag('folder_id', request.folder_id)
        response = self.cluster_service.List(request)
        return ComputeResponse(
            resources=map(Cluster.from_api, response.clusters),
            next_page_token=response.next_page_token,
        )

    def list_clusters(self, folder_id: str) -> Iterable[Cluster]:
        request = cluster_service_pb2.ListClustersRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_clusters, request)

    @client_retry
    @tracing.trace('List postgresql cluster databases')
    def _list_databases(self, request: database_service_pb2.ListDatabasesRequest) -> Iterable:
        tracing.set_tag('cluster_id', request.cluster_id)
        response = self.database_service.List(request)
        return ComputeResponse(
            resources=map(Database.from_api, response.databases),
            next_page_token=response.next_page_token,
        )

    def list_databases(self, cluster_id: str) -> Iterable[Database]:
        request = database_service_pb2.ListDatabasesRequest()
        request.cluster_id = cluster_id
        request.page_size = self.config.page_size
        return paginate(self._list_databases, request)

    @client_retry
    @tracing.trace('List postgresql cluster users')
    def _list_users(self, request: user_service_pb2.ListUsersRequest) -> Iterable:
        tracing.set_tag('cluster_id', request.cluster_id)
        response = self.user_service.List(request)
        return ComputeResponse(
            resources=map(User.from_api, response.users),
            next_page_token=response.next_page_token,
        )

    def list_users(self, cluster_id: str) -> Iterable[User]:
        request = user_service_pb2.ListUsersRequest()
        request.cluster_id = cluster_id
        request.page_size = self.config.page_size
        return paginate(self._list_users, request)

    @client_retry
    @tracing.trace('Create postgresql database')
    def create_database(
        self,
        cluster_id: str,
        database_spec: DatabaseSpec,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('postgresql.cluster_id', cluster_id)
        request = database_service_pb2.CreateDatabaseRequest(
            cluster_id=cluster_id,
            database_spec=database_pb2.DatabaseSpec(
                name=database_spec.name,
                owner=database_spec.owner,
                lc_collate=database_spec.lc_collate,
                lc_ctype=database_spec.lc_ctype,
                extensions=[
                    database_pb2.Extension(
                        name=extension.name,
                        version=extension.version,
                    )
                    for extension in database_spec.extensions
                ],
            ),
        )

        response = self.database_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = database_service_pb2.CreateDatabaseMetadata()
        response.metadata.Unpack(response_metadata)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.database_name

    @client_retry
    @tracing.trace('Delete postgresql database')
    def delete_database(
        self,
        cluster_id: str,
        database_name: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('postgresql.database', database_name)
        request = database_service_pb2.DeleteDatabaseRequest(
            cluster_id=cluster_id,
            database_name=database_name,
        )
        response = self.database_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = database_service_pb2.DeleteDatabaseMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.database_name

    @client_retry
    @tracing.trace('Create postgresql user')
    def create_user(
        self,
        cluster_id: str,
        user_spec: UserSpec,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('postgresql.cluster_id', cluster_id)
        request = user_service_pb2.CreateUserRequest(
            cluster_id=cluster_id,
            user_spec=user_pb2.UserSpec(
                name=user_spec.name,
                password=user_spec.password,
                permissions=[
                    user_pb2.Permission(
                        name=permission.database_name,
                    )
                    for permission in user_spec.permissions
                ],
                conn_limit=wrappers_pb2.Int64Value(value=user_spec.conn_limit),
                login=user_spec.login,
                grants=user_spec.grants,
            ),
        )

        response = self.user_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = user_service_pb2.CreateUserMetadata()
        response.metadata.Unpack(response_metadata)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.user_name

    @client_retry
    @tracing.trace('Delete postgresql user')
    def delete_user(
        self,
        cluster_id: str,
        user_name: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('postgresql.user', user_name)
        request = user_service_pb2.DeleteUserRequest(
            cluster_id=cluster_id,
            user_name=user_name,
        )
        response = self.user_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = user_service_pb2.DeleteUserMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.user_name
