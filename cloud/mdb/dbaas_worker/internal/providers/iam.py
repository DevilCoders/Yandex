"""
IAM token getter with cache
"""

import time
import uuid
from typing import Optional, Tuple

import grpc

from dbaas_common import retry, tracing
from yandex.cloud.priv.iam.v1 import (
    access_binding_service_pb2,
    access_binding_service_pb2_grpc,
    iam_token_service_pb2,
    iam_token_service_pb2_grpc,
    key_service_pb2,
    key_service_pb2_grpc,
    service_account_service_pb2,
    service_account_pb2,
    service_account_service_pb2_grpc,
)
from yandex.cloud.priv.iam.v1.awscompatibility import access_key_service_pb2, access_key_service_pb2_grpc
from yandex.cloud.priv.servicecontrol.v1 import resource_pb2

from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.dbaas_worker.internal.providers.iam_jwt import IamJwt
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.compute.iam.operations import (
    OperationsClient,
    OperationsClientConfig,
)
from ..exceptions import ExposedException


class IAMError(ExposedException):
    """
    Base IAM error
    """


class MultipleServiceAccountsFoundException(Exception):
    """
    Indicates that multiple service accounts with the same name found
    """

    pass


class ServiceAccountNotFoundException(Exception):
    """
    Indicates that service account not found
    """

    pass


class Iam(BaseProvider):
    """
    IAM token provider
    """

    __operations_client = None

    def __init__(self, config, task, queue, iam_jwt: Optional[IamJwt] = None):
        super().__init__(config, task, queue)
        self._token = None
        self.iam_jwt = iam_jwt or IamJwt(config, task, queue)

    def _init_services(self, token=None):
        call_credentials = grpc.access_token_call_credentials(token)
        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        self.iam_token_service = iam_token_service_pb2_grpc.IamTokenServiceStub(
            grpc.secure_channel(self.config.iam_token_service.url, composite_credentials)
        )
        iam_service_channel = grpc.secure_channel(self.config.iam_service.url, composite_credentials)
        iam_service_channel = tracing.grpc_channel_tracing_interceptor(iam_service_channel)
        self.service_account_service = service_account_service_pb2_grpc.ServiceAccountServiceStub(iam_service_channel)
        self.access_key_service = access_key_service_pb2_grpc.AccessKeyServiceStub(iam_service_channel)
        self.key_service = key_service_pb2_grpc.KeyServiceStub(iam_service_channel)
        self.access_binding_service = access_binding_service_pb2_grpc.AccessBindingServiceStub(iam_service_channel)

    def _get_token(self):
        """
        Returns IAM token of MDB service account received from Token Service (via IamJwt provider).
        """
        return self.iam_jwt.get_token()

    @property
    def _operations_client(self):
        if self.__operations_client is None:
            self.__operations_client = OperationsClient(
                config=OperationsClientConfig(
                    transport=grpcutil.Config(
                        url=self.config.iam_service.url,
                        cert_file=self.config.compute.ca_path,
                    ),
                ),
                logger=self.logger,
                token_getter=self._get_token,
                error_handlers={},
            )
        return self.__operations_client

    def reconnect(self):
        """
        Reinitialize IAM services clients using potentially changed token.
        """
        self._init_services(token=self._get_token())

    def _generate_request_metadata(self):
        request_id = str(uuid.uuid4())
        metadata = [('x-request-id', request_id)]
        extra = self.logger.extra.copy()
        extra['request_id'] = request_id
        return metadata, extra

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('IAM request token for service account')
    def _request_iam_token(self, service_account_id: str):
        """
        Request iam token for specified service account
        """

        request = iam_token_service_pb2.CreateIamTokenForServiceAccountRequest(
            service_account_id=service_account_id,
        )

        metadata, extra = self._generate_request_metadata()

        self.logger.logger.info(
            'Requesting token for service account %s in %s',
            service_account_id,
            self.config.iam_token_service.url,
            extra=extra,
        )

        try:
            res = self.iam_token_service.CreateForServiceAccount(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error('Token request failed: %s', repr(exc), extra=extra)
            raise

        self.logger.logger.info(
            'Got token for for service account %s in %s',
            service_account_id,
            self.config.iam_token_service.url,
            extra=extra,
        )

        self._token = res.iam_token
        self._expire_ts = res.expires_at.ToSeconds()
        return self._token

    def get_iam_token_for_service_account(self, service_account_id):
        """
        Get iam token
        """
        if self._token and time.time() + self.config.iam_token_service.expire_thresh < self._expire_ts:
            return self._token
        return self._request_iam_token(service_account_id)

    @tracing.trace('Service account get')
    def get_service_account(self, service_account_id: str) -> str:
        """
        Get service account for specified id, raise if error
        """
        metadata, extra = self._generate_request_metadata()

        self.logger.logger.info(
            'Getting service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        get_request = service_account_service_pb2.GetServiceAccountRequest(service_account_id=service_account_id)

        service_account = self.service_account_service.Get(get_request, metadata=metadata, timeout=5.0)

        self.logger.logger.info(
            'Service account %s got in %s',
            service_account.id,
            self.config.iam_service.url,
            extra=extra,
        )

        return service_account

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Service account create')
    def create_service_account(self, folder_id: str, service_account_name: str) -> str:
        """
        Create service account for specified folder id
        """
        metadata, extra = self._generate_request_metadata()

        self.logger.logger.info(
            'Creating service account %s for folder %s in %s',
            service_account_name,
            folder_id,
            self.config.iam_service.url,
            extra=extra,
        )

        create_request = service_account_service_pb2.CreateServiceAccountRequest(
            folder_id=folder_id, name=service_account_name
        )

        service_account = None

        try:
            metadata.append(('idempotency-key', service_account_name))
            res = self.service_account_service.Create(create_request, metadata=metadata, timeout=15.0)
            service_account = service_account_pb2.ServiceAccount.FromString(res.response.value)  # type: ignore
        except grpc.RpcError as exc:
            # Account with the same name is already exists.
            # It can happen if task was not finished after service account created.
            if exc.code() == grpc.StatusCode.ALREADY_EXISTS:  # type: ignore
                service_account = self.find_service_account_by_name(folder_id, service_account_name)
                if not service_account:
                    raise ServiceAccountNotFoundException(
                        "Service account {name} not found".format(name=service_account_name)
                    )
            else:
                self.logger.logger.error(
                    'Failed to create service account %s: %s', service_account_name, repr(exc), extra=extra
                )
                raise

        self.logger.logger.info(
            'Service account %s created for folder %s in %s',
            service_account.id,
            folder_id,
            self.config.iam_service.url,
            extra=extra,
        )

        self.add_change(Change(f'service_account.{service_account_name}', f'is created with id {service_account.id}'))

        return service_account.id

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Find service account')
    def find_service_account_by_name(self, folder_id: str, service_account_name: str):
        """
        Find service account in folder by provided name.
        """
        list_request = service_account_service_pb2.ListServiceAccountsRequest(
            folder_id=folder_id, filter='name="{}"'.format(service_account_name)
        )

        metadata, extra = self._generate_request_metadata()

        try:
            res = self.service_account_service.List(list_request, metadata=metadata, timeout=10.0)
            if len(res.service_accounts) == 1:
                return res.service_accounts[0]
            elif len(res.service_accounts) > 1:
                raise MultipleServiceAccountsFoundException(
                    "Multiple service accounts with the same name {name} found".format(name=service_account_name)
                )
            else:
                return None
        except grpc.RpcError as exc:
            self.logger.logger.error(
                'Failed to find service account %s: %s', service_account_name, repr(exc), extra=extra
            )
            raise

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Delete service account')
    def delete_service_account(self, service_account_id: str):
        """
        Delete service account.
        """
        delete_request = service_account_service_pb2.DeleteServiceAccountRequest(service_account_id=service_account_id)

        metadata, extra = self._generate_request_metadata()

        self.logger.logger.info(
            'Deleting service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        try:
            self.service_account_service.Delete(delete_request, metadata=metadata, timeout=10.0)
        except grpc.RpcError as exc:
            self.logger.logger.error(
                'Failed to delete service account %s: %s', service_account_id, repr(exc), extra=extra
            )
            raise

        self.logger.logger.info(
            'Service account %s is deleted in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        self.add_change(Change(f'service_account.{service_account_id}', 'is deleted'))

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Access key create')
    def create_access_key(self, service_account_id: str) -> Tuple[str, str]:
        """
        Create service account for specified folder id.
        Returns tuple access key id, secret key.
        """

        request = access_key_service_pb2.CreateAccessKeyRequest(
            service_account_id=service_account_id,
        )

        metadata, extra = self._generate_request_metadata()

        self.logger.logger.info(
            'Creating access key for service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        try:
            res = self.access_key_service.Create(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error(
                'Failed to create access key for %s: %s', service_account_id, repr(exc), extra=extra
            )
            raise

        self.logger.logger.info(
            'Access key created for service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        self.add_change(Change(f'access_key.{res.access_key.id}', f'is created for {service_account_id}'))

        return res.access_key.key_id, res.secret

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Authorized key create')
    def create_key(self, service_account_id: str) -> Tuple[str, str]:
        """
        Create authorized key (https://cloud.yandex.ru/docs/iam/concepts/authorization/key) for service account.
        Returns tuple (key_id, private_key).
        """

        tracing.set_tag('service_account.id', service_account_id)

        request = key_service_pb2.CreateKeyRequest(service_account_id=service_account_id)
        metadata, extra = self._generate_request_metadata()
        self.logger.logger.info(
            'Creating key for service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        try:
            res = self.key_service.Create(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error('Failed to create key for %s: %s', service_account_id, repr(exc), extra=extra)
            raise

        self.logger.logger.info(
            'Key created for service account %s in %s',
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )
        self.add_change(Change(f'key.{res.key.id}', f'is created for {service_account_id}'))
        return res.key.id, res.private_key

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('Grant role')
    def grant_role(self, cluster_resource_type: str, cluster_id: str, service_account_id: str, role: str):
        """
        Set access binding
        """

        tracing.set_tag('service_account.id', service_account_id)
        tracing.set_tag('cluster.id', cluster_id)

        resource = resource_pb2.Resource(id=cluster_id, type=cluster_resource_type)
        subject = access_binding_service_pb2.Subject(type='serviceAccount', id=service_account_id)
        access_binding = access_binding_service_pb2.AccessBinding(subject=subject, role_id=role)
        request = access_binding_service_pb2.SetAccessBindingsRequest(
            resource_path=[resource],
            access_bindings=[access_binding],
            private_call=True,
        )

        metadata, extra = self._generate_request_metadata()
        self.logger.logger.info(
            'Granting role %s on %s %s to service account %s in %s',
            role,
            cluster_resource_type,
            cluster_id,
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )

        try:
            operation = self.access_binding_service.SetAccessBindings(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error(
                'Failed to set access bindings for %s: %s', service_account_id, repr(exc), extra=extra
            )
            raise
        self.operation_wait(operation.id)

        self.logger.logger.info(
            'Role %s on %s %s is granted to service account %s in %s',
            role,
            cluster_resource_type,
            cluster_id,
            service_account_id,
            self.config.iam_service.url,
            extra=extra,
        )
        self.add_change(Change(f'access_binding.{service_account_id}', 'is set'))

    @tracing.trace('IAM Operation Wait')
    def operation_wait(self, operation_id, timeout=600, stop_time=None):
        """
        Wait until operation finishes
        """
        if operation_id is None:
            return

        tracing.set_tag('iam.operation.id', operation_id)

        if stop_time is None:
            stop_time = time.time() + timeout
        with self.interruptable:
            while time.time() < stop_time:
                operation = self._get_operation(operation_id)
                if not operation.done:
                    self.logger.info('Waiting for IAM operation %s', operation_id)
                    time.sleep(1)
                    continue
                if not operation.error:
                    return
                raise IAMError(operation.error.message)

            msg = '{timeout}s passed. IAM operation {id} is still running'
            raise IAMError(msg.format(timeout=timeout, id=operation_id))

    @tracing.trace('IAM Operation Get', ignore_active_span=True)
    def _get_operation(self, operation_id) -> OperationModel:
        """
        Get operation by id
        """
        return self._operations_client.get_operation(operation_id)
