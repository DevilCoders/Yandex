"""
Provider for instance group service
"""
import time
import uuid
from dataclasses import dataclass
from typing import Any, List, Optional

import yaml

import grpc

from dbaas_common import retry, tracing
from yandex.cloud.priv.microcosm.instancegroup.v1 import (
    instance_group_pb2,
    instance_group_service_pb2,
    instance_group_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException, ExposedException
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.dbaas_worker.internal.providers.iam import Iam
from cloud.mdb.dbaas_worker.internal.providers.metadb_instance_group import MetadbInstanceGroup


class InstanceGroupApiError(ExposedException):
    """
    Base compute instance group service error
    """


class InstanceGroupNotFoundError(UserExposedException):
    """
    InstanceGroup not found error
    """


class OperationNotFoundError(UserExposedException):
    """
    Operation not found error
    """


class OperationTimeoutError(UserExposedException):
    """
    Operation timeout error
    """


@dataclass
class GrpcResponse:
    data: Any = None
    meta: Any = None


statuses = instance_group_pb2.InstanceGroup.Status  # type: ignore


class InstanceGroup(BaseProvider):
    """
    Instance group provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.instance_group_service.url
        self.worker_service_account_id = self.config.iam_jwt.service_account_id
        self.grpc_timeout = self.config.instance_group_service.grpc_timeout
        self.metadb_instance_group = MetadbInstanceGroup(config, task, queue)
        self.service_account_id = None
        self.service_account_iam_token = None
        self.instance_group_service = None
        self.operation_service = None
        self.iam_service = Iam(config, task, queue)
        self._idempotence_ids = dict()

        self.managed_instance_type = instance_group_pb2.ManagedInstance

    def _init_channels(self, token):
        call_credentials = grpc.access_token_call_credentials(token)
        composite_credentials = grpc.composite_channel_credentials(
            grpc.ssl_channel_credentials(),
            call_credentials,
        )
        channel = grpc.secure_channel(self.url, composite_credentials)
        channel = tracing.grpc_channel_tracing_interceptor(channel)
        self.instance_group_service = instance_group_service_pb2_grpc.InstanceGroupServiceStub(channel)
        self.operation_service = operation_service_pb2_grpc.OperationServiceStub(channel)

    def _ensure_fresh_token(self):
        """
        Initializes services. Must be called before any method initialization.
        If called inside _handle_instance_group_request channel reinitialization will not affect initialized methods
        """
        if not self.service_account_id:
            raise RuntimeError('Service account must be specified')
        self.logger.info('Getting IAM token for service account %s', self.service_account_id)
        service_account_iam_token = self.iam_service.get_iam_token_for_service_account(self.service_account_id)
        if service_account_iam_token != self.service_account_iam_token:
            self.service_account_iam_token = service_account_iam_token
            self._init_channels(token=self.service_account_iam_token)

    def _get_idempotence_id(self, key):
        """
        Get local idempotence id by key
        """
        if key not in self._idempotence_ids:
            self._idempotence_ids[key] = str(uuid.uuid4())
        return self._idempotence_ids[key]

    def work_as_worker_service_account(self):
        """
        Work as MDB admin service account
        to be able to delete IG even when user folder is inactive and deletion is triggered by Resource Manager Reaper
        MDB-15917
        """
        self.service_account_id = self.worker_service_account_id
        self.iam_service.reconnect()
        self.logger.info('Working as mdb admin service account %s from now on', self.service_account_id)

    def work_as_user_specified_account(self, service_account_id):
        """
        Work as user specified service account
        """
        self.service_account_id = service_account_id
        self.iam_service.reconnect()
        self.logger.info('Working as service account %s from now on', self.service_account_id)

    def wait(self, operation_id: str, timeout: int = 600):
        """
        Wait operation by id for specified timeout in seconds
        """
        deadline = time.time() + timeout
        with self.interruptable:
            while time.time() < deadline:
                operation = self.get_operation(operation_id)
                if operation.data.done:
                    return
                time.sleep(1)
        message = f'Operation {operation_id} was not finished before timeout {timeout} seconds expired.'
        raise OperationTimeoutError(message)

    def _wait_instance_group_status(self, instance_group_id: str, status, timeout: int = 600):
        """
        Wait IG to achieve the specified status
        """
        deadline = time.time() + timeout
        with self.interruptable:
            while time.time() < deadline:
                if self.get(instance_group_id).data.status == status:
                    return
                time.sleep(1)
        message = f'Instance group {instance_group_id} did not achieve status {status} until {timeout} seconds expired.'
        raise OperationTimeoutError(message)

    def get_operation(self, operation_id: str):
        """
        Get operation by id
        """
        self._ensure_fresh_token()
        return self._handle_instance_group_request(
            request_type=operation_service_pb2.GetOperationRequest(operation_id=operation_id),
            method_type=self.operation_service.Get,
        )

    @retry.on_exception(InstanceGroupApiError, factor=10, max_wait=60, max_tries=6)
    def save_instance_group_meta(self, subcid, instance_group_id):
        """
        Save instance group
        """
        instance = self.get(instance_group_id)
        if not instance:
            raise InstanceGroupApiError(f'Unable to find instance group {instance_group_id}')
        self.metadb_instance_group.update(subcid, instance_group_id)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    def _handle_instance_group_request(
        self, request_type, method_type, response_meta_type=None, headers: dict = None, context_key: str = None
    ) -> GrpcResponse:
        try:
            response = GrpcResponse()
            metadata = headers or {}
            if context_key:
                metadata['idempotency_key'] = self._get_idempotence_id(context_key)
            response.data = method_type(
                request=request_type,
                timeout=self.grpc_timeout,
                metadata=tuple(metadata.items()) or None,
            )
            self.logger.info('InstanceGroup API url=%s request=%s response=%s', self.url, request_type, response.data)
            if response_meta_type:
                response.meta = response_meta_type()
                response.data.metadata.Unpack(response.meta)
            return response
        except grpc.RpcError as rpc_error:
            code = None
            # Workaround for too general error class in gRPC
            if hasattr(rpc_error, 'code') and callable(rpc_error.code):  # type: ignore
                code = rpc_error.code()  # type: ignore
            if code == grpc.StatusCode.NOT_FOUND:
                if isinstance(request_type, operation_service_pb2.GetOperationRequest):
                    raise OperationNotFoundError(f'Operation {request_type.operation_id} is not found')
                if hasattr(request_type, 'instance_group_id'):
                    raise InstanceGroupNotFoundError(f'Instance group {request_type.instance_group_id} is not found')
                elif hasattr(request_type, 'folder_id'):
                    raise InstanceGroupNotFoundError(f'Folder {request_type.folder_id} is not found')
            raise

    def get(self, instance_group_id: str):
        """
        Get instance group by id
        """
        self._ensure_fresh_token()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.GetInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=self.instance_group_service.Get,
        )

    def list_references(self, instance_group_id: str):
        self._ensure_fresh_token()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupReferencesRequest(
                instance_group_id=instance_group_id,
            ),
            method_type=self.instance_group_service.ListReferences,
        )

    def list(self, folder_id: str):
        """
        List instance groups in folder
        """
        self._ensure_fresh_token()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupsRequest(folder_id=folder_id),
            method_type=self.instance_group_service.List,
        )

    def list_instances(self, instance_group_id: str):
        """
        List instances in instance group
        """
        self._ensure_fresh_token()
        return self._handle_instance_group_request(
            request_type=instance_group_service_pb2.ListInstanceGroupInstancesRequest(
                instance_group_id=instance_group_id,
            ),
            method_type=self.instance_group_service.ListInstances,
        )

    def delete_instances(
        self,
        instance_group_id: str,
        managed_instance_ids: List[str],
        wait: bool = True,
        referrer_id: str = None,
    ) -> str:
        """
        Delete instances in instance group
        managed_instance_ids are not compute instance ids but ids returned as id field from list_instances
        """
        context_key = f'{instance_group_id}.instance_group.delete_instances'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'delete instances initiated'))
            return operation_from_context
        self._ensure_fresh_token()
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.DeleteInstancesRequest(
                instance_group_id=instance_group_id,
                managed_instance_ids=managed_instance_ids,
            ),
            method_type=self.instance_group_service.DeleteInstances,
            response_meta_type=instance_group_service_pb2.DeleteInstancesMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'delete instances initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id

    def stop_instances(
        self,
        instance_group_id: str,
        managed_instance_ids: List[str],
        wait: bool = True,
        referrer_id: str = None,
    ) -> str:
        """
        Stop instances in instance group
        managed_instance_ids are not compute instance ids but ids returned as id field from list_instances
        """
        context_key = f'{instance_group_id}.instance_group.stop_instances'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'stop instances initiated'))
            return operation_from_context
        self._ensure_fresh_token()
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.StopInstancesRequest(
                instance_group_id=instance_group_id,
                managed_instance_ids=managed_instance_ids,
            ),
            method_type=self.instance_group_service.StopInstances,
            response_meta_type=instance_group_service_pb2.StopInstancesMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'stop instances initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id

    def delete(
        self,
        instance_group_id: str,
        wait: bool = True,
        referrer_id: str = None,
        service_account_id: str = None,  # MDB-16407 workaround to work with inactive folders.
        # IG will delete compute instances as this service account, not the service account attached to the group
    ) -> str:
        """
        Delete instance group
        """
        context_key = f'{instance_group_id}.instance_group.delete'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            return operation_from_context
        self._ensure_fresh_token()
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.DeleteInstanceGroupRequest(
                instance_group_id=instance_group_id,
                service_account_id=service_account_id,
            ),
            method_type=self.instance_group_service.Delete,
            response_meta_type=instance_group_service_pb2.DeleteInstanceGroupMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id

    def stop(
        self,
        instance_group_id: str,
        wait: bool = True,
        referrer_id: str = None,
    ) -> Optional[str]:
        """
        Stop instance group
        """
        context_key = f'{instance_group_id}.instance_group.stop'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'stop initiated'))
            if wait:
                self.wait(operation_id=operation_from_context)
                return instance_group_id
            return operation_from_context
        self._ensure_fresh_token()

        instance_group = self.get(instance_group_id)
        if instance_group.data.status == statuses.STOPPING:
            self._wait_instance_group_status(instance_group_id, status=statuses.STOPPED)
            return None
        elif instance_group.data.status == statuses.STOPPED:
            return None

        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.StopInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=self.instance_group_service.Stop,
            response_meta_type=instance_group_service_pb2.StopInstanceGroupMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'stop initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id

    def start(
        self,
        instance_group_id: str,
        wait: bool = True,
        referrer_id: str = None,
    ) -> Optional[str]:
        """
        Start instance group
        """
        context_key = f'{instance_group_id}.instance_group.start'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'start initiated'))
            if wait:
                self.wait(operation_id=operation_from_context)
                return instance_group_id
            return operation_from_context

        self._ensure_fresh_token()

        instance_group = self.get(instance_group_id)
        if instance_group.data.status == statuses.STARTING:
            self._wait_instance_group_status(instance_group_id, status=statuses.ACTIVE)
            return None
        elif instance_group.data.status == statuses.ACTIVE:
            return None

        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.StartInstanceGroupRequest(instance_group_id=instance_group_id),
            method_type=self.instance_group_service.Start,
            response_meta_type=instance_group_service_pb2.StartInstanceGroupMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'start initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
        return operation_id

    def exists(
        self,
        folder_id: str,
        instance_group_config: dict,
        subcluster_id: str,
        wait: bool = True,
    ):
        """
        Create instance group if it is absent or return existing
        """
        context_key = f'instance_group.create.{subcluster_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, instance_group_id = result_from_context
            self.save_instance_group_meta(subcluster_id, instance_group_id)
            if wait:
                self.wait(operation_id=operation_id)
                return instance_group_id
            return result_from_context

        operation_id = None
        # Return existing instance group if it already was added to metadb
        instance_group_id = self.metadb_instance_group.get(subcid=subcluster_id)
        if instance_group_id:
            instance_group = None
            try:
                instance_group = self.get(instance_group_id)
            except InstanceGroupNotFoundError:
                self.logger.error(f'instance group {instance_group_id} was not found')
                self.metadb_instance_group.update(
                    subcid=subcluster_id,
                    instance_group_id=None,
                )
            if instance_group:
                return operation_id, instance_group_id

        instance_groups = self.list(folder_id).data.instance_groups
        try:
            for instance_group in instance_groups:
                if instance_group.name == instance_group_config['name']:
                    references = self.list_references(instance_group.id).data.references
                    for reference in references:
                        if reference.referrer.id == subcluster_id:
                            self.logger.info(
                                f'instance group with name {instance_group.name} already exists. '
                                f'taking its id {instance_group.id} for subcluster {subcluster_id}'
                            )
                            instance_group_id = instance_group.id
                            self.metadb_instance_group.update(
                                subcid=subcluster_id,
                                instance_group_id=instance_group_id,
                            )
                            return operation_id, instance_group_id
        except Exception:
            self.logger.exception(f'subcid={subcluster_id}')

        return self.create(
            folder_id,
            instance_group_config,
            subcluster_id,
            wait,
        )

    def create(
        self,
        folder_id: str,
        instance_group_config: dict,
        subcluster_id: str,
        wait: bool = True,
    ):
        """
        Create instance group using config as a YAML string.
        """
        context_key = f'instance_group.create.{subcluster_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, instance_group_id = result_from_context
            self.save_instance_group_meta(subcluster_id, instance_group_id)
            if wait:
                self.wait(operation_id=operation_id)
                return instance_group_id
            return result_from_context

        self._ensure_fresh_token()
        instance_group_config_yaml = yaml.safe_dump(instance_group_config)
        self.logger.debug(f'instance_group_config_yaml={instance_group_config_yaml}')
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.CreateInstanceGroupFromYamlRequest(
                folder_id=folder_id,
                instance_group_yaml=instance_group_config_yaml,
            ),
            method_type=self.instance_group_service.CreateFromYaml,
            response_meta_type=instance_group_service_pb2.CreateInstanceGroupMetadata,
            context_key=context_key,
        )
        operation_id = operation_response.data.id
        instance_group_id = operation_response.meta.instance_group_id
        result = (operation_id, instance_group_id)
        self.add_change(
            Change(
                context_key,
                'create initiated',
                context={context_key: result},
            )
        )
        self.save_instance_group_meta(subcluster_id, instance_group_id)
        if wait:
            self.wait(operation_id=operation_id)
            return instance_group_id
        return result

    def update(
        self,
        instance_group_id: str,
        yaml_config: str,
        wait: bool = True,
        referrer_id: str = None,
    ):
        """
        Update instance group using config as a YAML string.
        """
        context_key = f'{instance_group_id}.instance_group.update'
        operation_from_context = self.context_get(context_key)
        if operation_from_context:
            self.add_change(Change(context_key, 'update initiated'))
            if wait:
                self.wait(operation_id=operation_from_context)
                return instance_group_id
            return operation_from_context
        self._ensure_fresh_token()
        operation_response = self._handle_instance_group_request(
            request_type=instance_group_service_pb2.UpdateInstanceGroupFromYamlRequest(
                instance_group_id=instance_group_id,
                instance_group_yaml=yaml_config,
            ),
            method_type=self.instance_group_service.UpdateFromYaml,
            response_meta_type=instance_group_service_pb2.UpdateInstanceGroupMetadata,
            context_key=context_key,
            headers={'referrer-id': referrer_id} if referrer_id else {},
        )
        operation_id = operation_response.data.id
        self.add_change(
            Change(
                context_key,
                'update initiated',
                context={context_key: operation_id},
            )
        )
        if wait:
            self.wait(operation_id=operation_id)
            return instance_group_id
        return operation_id
