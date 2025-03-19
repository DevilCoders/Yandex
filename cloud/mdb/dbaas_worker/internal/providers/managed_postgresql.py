import uuid
from dataclasses import dataclass
from typing import Any, Callable, Tuple, Optional

from cloud.mdb.internal.python.managed_postgresql.models import (
    DatabaseSpec,
    UserSpec,
)
from cloud.mdb.internal.python.grpcutil import NotFoundError
from cloud.mdb.internal.python.grpcutil.exceptions import AlreadyExistsError
from .compute import UserExposedComputeRunningOperationsLimitError, UserExposedComputeApiError
from .iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.internal.python.managed_postgresql import (
    ManagedPostgresqlClient,
    ManagedPostgresqlClientConfig,
)
from cloud.mdb.internal.python.grpcutil import exceptions as grpcutil_errors
from cloud.mdb.internal.python import grpcutil


@dataclass
class GrpcResponse:
    data: Any = None
    meta: Any = None


def quota_error(err: grpcutil_errors.ResourceExhausted) -> None:
    if err.message == 'The limit on maximum number of active operations has exceeded.':
        raise UserExposedComputeRunningOperationsLimitError(message=err.message, err_type=err.err_type, code=err.code)
    raise UserExposedComputeApiError(message=err.message, err_type=err.err_type, code=err.code)


def gen_config(ca_path: str) -> Callable:
    def from_url(url: str) -> grpcutil.Config:
        return grpcutil.Config(
            url=url,
            cert_file=ca_path,
        )

    return from_url


class ManagedPostgresql(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.managed_postgresql_service.url
        self.grpc_timeout = self.config.managed_postgresql_service.grpc_timeout
        self.iam_jwt = IamJwt(
            config,
            task,
            queue,
            service_account_id=self.config.compute.service_account_id,
            key_id=self.config.compute.key_id,
            private_key=self.config.compute.private_key,
        )

        transport_config = gen_config(self.config.compute.ca_path)
        error_handlers = {
            grpcutil_errors.ResourceExhausted: quota_error,
        }
        self.client = ManagedPostgresqlClient(
            config=ManagedPostgresqlClientConfig(
                transport=transport_config(self.config.managed_postgresql_service.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self._idempotence_ids = dict()

    def get_token(self):
        return self.iam_jwt.get_token()

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    def database_exists(
        self,
        cluster_id: str,
        database_spec: DatabaseSpec,
    ) -> Tuple[str, str]:
        context_key = f'postgresql_database.create.{cluster_id}.{database_spec.name}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, database_name = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        self.client.wait_alive_status(cluster_id)
        operation_id, database_name = None, database_spec.name
        try:
            operation_id, database_name = self.client.create_database(
                cluster_id=cluster_id,
                database_spec=database_spec,
                idempotency_key=self._get_idempotence_id(context_key),
            )
        except AlreadyExistsError:
            pass

        result = (operation_id, database_name)
        self.add_change(
            Change(
                context_key,
                'create initiated',
                context={context_key: result},
            )
        )
        if operation_id:
            self.client.wait_operation(operation_id=operation_id)
        return result

    def database_absent(
        self,
        cluster_id: str,
        database_name: str,
    ) -> Optional[Tuple[str, str]]:
        context_key = f'postgresql_database.delete.{cluster_id}.{database_name}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            operation_id, database_name = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        self.client.wait_alive_status(cluster_id)
        try:
            operation_id, database_name = self.client.delete_database(
                cluster_id=cluster_id,
                database_name=database_name,
            )
        except NotFoundError:
            self.logger.info(f'db {database_name} of cluster {cluster_id} was not found')
            return None

        result = (operation_id, database_name)
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result

    def user_exists(
        self,
        cluster_id: str,
        user_spec: UserSpec,
    ) -> Tuple[str, str]:
        context_key = f'postgresql_user.create.{cluster_id}.{user_spec.name}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, user_name = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        self.client.wait_alive_status(cluster_id)
        operation_id, user_name = None, user_spec.name
        try:
            operation_id, user_name = self.client.create_user(
                cluster_id=cluster_id, user_spec=user_spec, idempotency_key=self._get_idempotence_id(context_key)
            )
        except AlreadyExistsError:
            pass
        result = (operation_id, user_name)
        self.add_change(
            Change(
                context_key,
                'create initiated',
                context={context_key: result},
            )
        )
        if operation_id:
            self.client.wait_operation(operation_id=operation_id)
        return result

    def user_absent(
        self,
        cluster_id: str,
        user_name: str,
    ) -> Optional[Tuple[str, str]]:
        context_key = f'postgresql_user.delete.{cluster_id}.{user_name}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            operation_id, user_name = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        self.client.wait_alive_status(cluster_id)
        try:
            operation_id, user_name = self.client.delete_user(
                cluster_id=cluster_id,
                user_name=user_name,
            )
        except NotFoundError:
            self.logger.info(f'user {user_name} of cluster {cluster_id} was not found')
            return None

        result = (operation_id, user_name)
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result
