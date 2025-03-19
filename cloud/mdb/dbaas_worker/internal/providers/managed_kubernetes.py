"""
Provider for managed kubernetes service
"""

import uuid
from dataclasses import dataclass
from typing import Any, Callable, Optional, Tuple, Dict, List, Iterable

from cloud.mdb.internal.python.managed_kubernetes.models import (
    NodeTemplate,
    ScalePolicy,
    MasterSpec,
    IPAllocationPolicy,
    ReleaseChannel,
    NodeGroupAllocationPolicy,
    DeployPolicy,
    NodeGroupMaintenancePolicy,
    Taint,
)
from cloud.mdb.internal.python.grpcutil.exceptions import AlreadyExistsError
from .compute import UserExposedComputeRunningOperationsLimitError, UserExposedComputeApiError
from .iam_jwt import IamJwt
from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException, ExposedException
from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider, Change
from cloud.mdb.internal.python.managed_kubernetes import (
    ManagedKubernetesClient,
    ManagedKubernetesClientConfig,
)
from cloud.mdb.internal.python.grpcutil import exceptions as grpcutil_errors
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.dbaas_worker.internal.providers.metadb_kubernetes import MetadbKubernetes
from cloud.mdb.dbaas_worker.internal.providers.kubernetes_master import KubernetesMaster


class ManagedKubernetesApiError(ExposedException):
    """
    Base compute managed kubernetes service error
    """


class ClusterNotFoundError(UserExposedException):
    """
    Cluster not found error
    """


class NodeGroupNotFoundError(UserExposedException):
    """
    NodeGroup not found error
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


class ManagedKubernetes(BaseProvider):
    """
    Managed Kubernetes provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.url = self.config.managed_kubernetes_service.url
        self.grpc_timeout = self.config.managed_kubernetes_service.grpc_timeout
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
        self.client = ManagedKubernetesClient(
            config=ManagedKubernetesClientConfig(
                transport=transport_config(self.config.managed_kubernetes_service.url),
            ),
            logger=self.logger,
            token_getter=self.get_token,
            error_handlers=error_handlers,
        )
        self.metadb_kubernetes = MetadbKubernetes(config, task, queue)
        self.kubernetes_master = KubernetesMaster(config, task, queue)
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

    def cluster_exists(
        self,
        subcluster_id: str,
        folder_id: str,
        network_id: str,
        service_account_id: str,
        node_service_account_id: str,
        master_spec: MasterSpec,
        ip_allocation_policy: Optional[IPAllocationPolicy] = None,
        release_channel: Optional[ReleaseChannel] = None,
        name: Optional[str] = None,
        description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
    ) -> Tuple[str, str]:
        context_key = f'kubernetes_cluster.create.{subcluster_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, kubernetes_cluster_id = result_from_context
            self.metadb_kubernetes.update(
                subcid=subcluster_id,
                kubernetes_cluster_id=kubernetes_cluster_id,
            )
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        operation_id, kubernetes_cluster_id = self.client.create_cluster(
            folder_id=folder_id,
            network_id=network_id,
            service_account_id=service_account_id,
            node_service_account_id=node_service_account_id,
            master_spec=master_spec,
            ip_allocation_policy=ip_allocation_policy,
            release_channel=release_channel,
            name=name,
            description=description,
            labels=labels,
        )
        result = (operation_id, kubernetes_cluster_id)
        self.add_change(
            Change(
                context_key,
                'create initiated',
                context={context_key: result},
            )
        )
        self.metadb_kubernetes.update(
            subcid=subcluster_id,
            kubernetes_cluster_id=kubernetes_cluster_id,
        )
        self.client.wait_operation(operation_id=operation_id)
        return result

    def node_group_exists(
        self,
        subcluster_id: str,
        kubernetes_cluster_id: str,
        node_template: NodeTemplate,
        scale_policy: ScalePolicy,
        name: Optional[str] = None,
        description: Optional[str] = None,
        labels: Optional[Dict[str, str]] = None,
        allocation_policy: Optional[NodeGroupAllocationPolicy] = None,
        deploy_policy: Optional[DeployPolicy] = None,
        version: Optional[str] = None,
        maintenance_policy: Optional[NodeGroupMaintenancePolicy] = None,
        allowed_unsafe_sysctls: Optional[List[str]] = None,
        specific_revision: Optional[int] = None,
        node_taints: Optional[Iterable[Taint]] = (),
        node_labels: Optional[Dict[str, str]] = None,
    ) -> Tuple[str, str]:
        context_key = f'kubernetes_node_group.create.{subcluster_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'create initiated'))
            operation_id, node_group_id = result_from_context
            self.metadb_kubernetes.update(
                subcid=subcluster_id,
                kubernetes_cluster_id=kubernetes_cluster_id,
                node_group_id=node_group_id,
            )
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        operation_id, node_group_id = None, None
        try:
            operation_id, node_group_id = self.client.create_node_group(
                cluster_id=kubernetes_cluster_id,
                node_template=node_template,
                scale_policy=scale_policy,
                name=name,
                description=description,
                labels=labels,
                allocation_policy=allocation_policy,
                deploy_policy=deploy_policy,
                version=version,
                maintenance_policy=maintenance_policy,
                allowed_unsafe_sysctls=allowed_unsafe_sysctls,
                specific_revision=specific_revision,
                node_taints=node_taints,
                node_labels=node_labels,
            )
        except AlreadyExistsError:
            for node_group in self.client.list_node_groups(cluster_id=kubernetes_cluster_id):
                if node_group.name == name:
                    node_group_id = node_group.id
        if not node_group_id:
            raise Exception(f'Can not find node group {name}')
        self.metadb_kubernetes.update(
            subcid=subcluster_id,
            kubernetes_cluster_id=kubernetes_cluster_id,
            node_group_id=node_group_id,
        )
        result = (operation_id, node_group_id)
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

    def cluster_absent(
        self,
        kubernetes_cluster_id: str,
    ) -> Tuple[str, str]:
        context_key = f'kubernetes_cluster.delete.{kubernetes_cluster_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            operation_id, kubernetes_cluster_id = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        operation_id, kubernetes_cluster_id = self.client.delete_cluster(
            cluster_id=kubernetes_cluster_id,
        )
        result = (operation_id, kubernetes_cluster_id)
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result

    def node_group_absent(
        self,
        node_group_id: str,
    ) -> Tuple[str, str]:
        context_key = f'kubernetes_node_group.delete.{node_group_id}'
        result_from_context = self.context_get(context_key)
        if result_from_context:
            self.add_change(Change(context_key, 'delete initiated'))
            operation_id, node_group_id = result_from_context
            self.client.wait_operation(operation_id=operation_id)
            return result_from_context

        operation_id, node_group_id = self.client.delete_node_group(
            node_group_id=node_group_id,
        )
        result = (operation_id, node_group_id)
        self.add_change(
            Change(
                context_key,
                'delete initiated',
                context={context_key: result},
            )
        )
        self.client.wait_operation(operation_id=operation_id)
        return result

    def initialize_kubernetes_master(self, kubernetes_cluster_id: str, namespace: str):
        self.kubernetes_master.set_namespace(namespace)

        cluster_info = self.client.get_cluster(kubernetes_cluster_id)
        self.kubernetes_master.set_cluster_connection_details(
            master_endpoint=cluster_info.master.endpoints.external_v4_endpoint
            or cluster_info.master.endpoints.internal_v4_endpoint,
            api_key=self.get_token(),
            cluster_certificate=cluster_info.master.master_auth.cluster_ca_certificate,
        )
