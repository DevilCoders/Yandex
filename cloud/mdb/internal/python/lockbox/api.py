import functools
import time

from cloud.mdb.internal.python import grpcutil
from typing import Callable, Dict, NamedTuple, Optional, Iterable, List, Tuple
from .models import (
    Secret,
    Version,
    PayloadEntryChange,
)

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from dbaas_common import tracing
from yandex.cloud.priv.lockbox.v1 import (
    secret_service_pb2,
    secret_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from ..compute.models import OperationModel
from ..compute.pagination import ComputeResponse, paginate
from ..grpcutil import WrappedGRPCService


def _payload_entry_changes_to_pb(
    payload_entry_changes: List[PayloadEntryChange],
) -> List[secret_service_pb2.PayloadEntryChange]:
    payload_entry_changes_pb = []
    for payload_entry_change in payload_entry_changes:
        if payload_entry_change.text_value:
            payload_entry_changes_pb.append(
                secret_service_pb2.PayloadEntryChange(
                    key=payload_entry_change.key,
                    text_value=payload_entry_change.text_value,
                )
            )
        elif payload_entry_change.binary_value:
            payload_entry_changes_pb.append(
                secret_service_pb2.PayloadEntryChange(
                    key=payload_entry_change.key,
                    binary_value=payload_entry_change.binary_value,
                )
            )
        elif payload_entry_change.reference:
            payload_entry_changes_pb.append(
                secret_service_pb2.PayloadEntryChange(
                    key=payload_entry_change.key,
                    reference=payload_entry_change.reference,
                )
            )
    return payload_entry_changes_pb


class LockboxClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class LockboxClient:
    def __init__(
        self,
        config: LockboxClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='LockboxClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    @functools.cached_property
    def secret_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            secret_service_pb2_grpc.SecretServiceStub,
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

    @client_retry
    @tracing.trace('Get lockbox secret')
    def get_secret(self, secret_id: str) -> Secret:
        tracing.set_tag('secret_id', secret_id)
        request = secret_service_pb2.GetSecretRequest(secret_id=secret_id)
        return Secret.from_api(self.secret_service.Get(request))

    @client_retry
    @tracing.trace('List lockbox secrets')
    def _list_secrets(self, request: secret_service_pb2.ListSecretsRequest) -> Iterable:
        tracing.set_tag('folder_id', request.folder_id)
        response = self.secret_service.List(request)
        return ComputeResponse(
            resources=map(Secret.from_api, response.secrets),
            next_page_token=response.next_page_token,
        )

    def list_secrets(self, folder_id: str) -> Iterable[Secret]:
        request = secret_service_pb2.ListSecretsRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_secrets, request)

    def get_secret_by_name(self, folder_id: str, secret_name: str) -> Optional[Secret]:
        for secret in self.list_secrets(folder_id):
            if secret.name == secret_name:
                return secret

    @client_retry
    @tracing.trace('List lockbox versions')
    def _list_versions(self, request: secret_service_pb2.ListVersionsRequest) -> Iterable:
        tracing.set_tag('secret_id', request.secret_id)
        response = self.secret_service.ListVersions(request)
        return ComputeResponse(
            resources=map(Version.from_api, response.versions),
            next_page_token=response.next_page_token,
        )

    def list_versions(self, secret_id: str) -> Iterable[Secret]:
        request = secret_service_pb2.ListVersionsRequest()
        request.secret_id = secret_id
        request.page_size = self.config.page_size
        return paginate(self._list_versions, request)

    @tracing.trace('Wait operation')
    def wait_operation(self, operation_id: str, timeout: int = 900) -> OperationModel:
        deadline = time.time() + timeout
        while time.time() < deadline:
            response = self.operation_service.Get(
                operation_service_pb2.GetOperationRequest(operation_id=operation_id),
                timeout=self.config.timeout,
            )
            operation = OperationModel().operation_from_api(
                self.logger, response, self.operation_service.error_from_rich_status
            )
            if operation.error:
                raise RuntimeError(f'Lockbox operation failed: {operation.error}')
            if operation.done:
                return operation
            time.sleep(1)

    @client_retry
    @tracing.trace('Create lockbox secret')
    def create_secret(
        self,
        folder_id: str,
        name: str,
        description: Optional[str] = None,
        version_description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
        kms_key_id: Optional[str] = None,
        version_payload_entries: Optional[List[PayloadEntryChange]] = None,
        deletion_protection: bool = False,
        idempotency_key: Optional[str] = None,
        wait: bool = True,
    ) -> Tuple[Secret, str]:
        tracing.set_tag('folder_id', folder_id)

        request = secret_service_pb2.CreateSecretRequest(
            folder_id=folder_id,
            name=name,
            description=description,
            version_description=version_description,
            kms_key_id=kms_key_id,
            version_payload_entries=_payload_entry_changes_to_pb(version_payload_entries),
            deletion_protection=deletion_protection,
            labels=labels,
        )

        response = self.secret_service.Create(request, idempotency_key=idempotency_key)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        operation.parse_response(Secret)
        if wait:
            self._ensure_operation_succeeded(operation)
        return operation.response, operation.operation_id

    @client_retry
    @tracing.trace('Delete lockbox secret')
    def delete_secret(
        self,
        secret_id: str,
        idempotency_key: str,
        wait: bool = True,
    ) -> Tuple[str, str]:
        tracing.set_tag('secret_id', secret_id)
        request = secret_service_pb2.DeleteSecretRequest(
            secret_id=secret_id,
        )
        response = self.secret_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = secret_service_pb2.DeleteSecretMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            self._ensure_operation_succeeded(operation)
        return operation.operation_id, response_metadata.secret_id

    @client_retry
    @tracing.trace('Create lockbox version')
    def create_version(
        self,
        secret_id: str,
        payload_entries: List[PayloadEntryChange],
        idempotency_key: str,
        description: str = "",
        base_version_id: str = "",
        incremental: bool = False,
        wait: bool = True,
    ) -> Tuple[Version, str]:
        if incremental:
            payload_change_kind = secret_service_pb2.AddVersionRequest.PayloadChangeKind.INCREMENTAL
        else:
            payload_change_kind = secret_service_pb2.AddVersionRequest.PayloadChangeKind.FULL

        req = secret_service_pb2.AddVersionRequest(
            secret_id=secret_id,
            payload_entries=_payload_entry_changes_to_pb(payload_entries),
            description=description,
            base_version_id=base_version_id,
            payload_change_kind=payload_change_kind,
        )
        response = self.secret_service.AddVersion(req, idempotency_key=idempotency_key)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        operation.parse_response(Version)
        if wait:
            self._ensure_operation_succeeded(operation)
        return operation.response, operation.operation_id

    def _ensure_operation_succeeded(self, operation: OperationModel):
        if not operation.done:
            self.wait_operation(operation.operation_id)
        elif operation.error:
            raise RuntimeError(f'Lockbox operation failed: {operation.error}')
