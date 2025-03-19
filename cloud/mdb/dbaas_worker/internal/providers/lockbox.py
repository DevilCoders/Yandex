import uuid
from dataclasses import dataclass
from typing import Any, Callable, Optional, Tuple, Dict, List

from cloud.mdb.internal.python.lockbox.models import (
    PayloadEntryChange,
)
from cloud.mdb.internal.python.grpcutil import NotFoundError
from .compute import UserExposedComputeRunningOperationsLimitError, UserExposedComputeApiError
from .iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.internal.python.lockbox import (
    LockboxClient,
    LockboxClientConfig,
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


class Lockbox(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.lockbox_service.url
        self.grpc_timeout = self.config.lockbox_service.grpc_timeout
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
        self.client = LockboxClient(
            config=LockboxClientConfig(
                transport=transport_config(self.config.lockbox_service.url),
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

    def secret_exists(
        self,
        folder_id: str,
        name: str,
        description: Optional[str] = None,
        version_description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
        kms_key_id: Optional[str] = None,
        version_payload_entries: Optional[List[PayloadEntryChange]] = None,
        deletion_protection: bool = False,
    ) -> Tuple[str, str, Optional[List[PayloadEntryChange]]]:
        context_key = f'lockbox_secret.create.{folder_id}.{name}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, secret_id, version_payload_entries = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        secret, operation_id = self.client.create_secret(
            folder_id=folder_id,
            name=name,
            description=description,
            version_description=version_description,
            labels=labels,
            kms_key_id=kms_key_id,
            version_payload_entries=version_payload_entries,
            deletion_protection=deletion_protection,
            idempotency_key=self._get_idempotence_id(context_key),
            wait=False,
        )
        result = (operation_id, secret.id, version_payload_entries)
        self.add_change(
            Change(
                context_key,
                'create initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result

    def secret_absent(
        self,
        secret_id: str,
    ) -> Optional[Tuple[str, str]]:
        context_key = f'lockbox_secret.delete.{secret_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            operation_id, secret_id = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        try:
            operation_id, secret_id = self.client.delete_secret(
                secret_id=secret_id,
                idempotency_key=self._get_idempotence_id(context_key),
                wait=False,
            )
        except NotFoundError:
            self.logger.info(f'secret {secret_id} was not found')
            return None

        result = (operation_id, secret_id)
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result
