import functools
import time

from cloud.mdb.internal.python import grpcutil
from typing import Callable, Dict, NamedTuple, Optional, Iterable, List, Tuple
from .models import (
    ClusterModel,
    ReleaseChannel,
    IPAllocationPolicy,
    MasterSpec,
    NodeGroupModel,
    Taint,
    NodeTemplate,
    ScalePolicy,
    NodeGroupAllocationPolicy,
    DeployPolicy,
    NodeGroupMaintenancePolicy,
    Node,
)

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from dbaas_common import tracing
from yandex.cloud.priv.k8s.v1 import (
    cluster_service_pb2,
    cluster_service_pb2_grpc,
    node_pb2,
    node_group_pb2,
    node_group_service_pb2,
    node_group_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from ..compute.models import OperationModel
from ..compute.pagination import ComputeResponse, paginate
from ..grpcutil import WrappedGRPCService


class ManagedKubernetesClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0
    page_size: int = 1000


class ManagedKubernetesClient:
    def __init__(
        self,
        config: ManagedKubernetesClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: Dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='ManagedKubernetesClient')
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
    def node_group_service(self):
        return ComputeGRPCService(
            self.logger,
            grpcutil.new_grpc_channel(self.config.transport),
            node_group_service_pb2_grpc.NodeGroupServiceStub,
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
    @tracing.trace('Get kubernetes cluster')
    def get_cluster(self, cluster_id: str) -> ClusterModel:
        tracing.set_tag('cluster_id', cluster_id)
        request = cluster_service_pb2.GetClusterRequest(cluster_id=cluster_id)
        return ClusterModel.from_api(self.cluster_service.Get(request))

    @client_retry
    @tracing.trace('Get kubernetes node group')
    def get_node_group(self, node_group_id: str) -> NodeGroupModel:
        tracing.set_tag('node_group_id', node_group_id)
        request = node_group_service_pb2.GetNodeGroupRequest(node_group_id=node_group_id)
        return NodeGroupModel.from_api(self.node_group_service.Get(request))

    @client_retry
    @tracing.trace('List kubernetes clusters')
    def _list_clusters(self, request: cluster_service_pb2.ListClustersRequest) -> Iterable:
        tracing.set_tag('folder_id', request.folder_id)
        response = self.cluster_service.List(request)
        return ComputeResponse(
            resources=map(ClusterModel.from_api, response.clusters),
            next_page_token=response.next_page_token,
        )

    def list_clusters(self, folder_id: str) -> Iterable[ClusterModel]:
        request = cluster_service_pb2.ListClustersRequest()
        request.folder_id = folder_id
        request.page_size = self.config.page_size
        return paginate(self._list_clusters, request)

    @client_retry
    @tracing.trace('List kubernetes cluster node groups')
    def _list_node_groups(self, request: cluster_service_pb2.ListClusterNodeGroupsRequest) -> Iterable:
        tracing.set_tag('cluster_id', request.cluster_id)
        response = self.cluster_service.ListNodeGroups(request)
        return ComputeResponse(
            resources=map(NodeGroupModel.from_api, response.node_groups),
            next_page_token=response.next_page_token,
        )

    def list_node_groups(self, cluster_id: str) -> Iterable[NodeGroupModel]:
        request = cluster_service_pb2.ListClusterNodeGroupsRequest()
        request.cluster_id = cluster_id
        request.page_size = self.config.page_size
        return paginate(self._list_node_groups, request)

    @client_retry
    @tracing.trace('List kubernetes node group nodes')
    def _list_nodes(self, request: node_group_service_pb2.ListNodeGroupNodesRequest) -> Iterable:
        tracing.set_tag('node_group_id', request.node_group_id)
        response = self.node_group_service.ListNodes(request)
        return ComputeResponse(
            resources=map(Node.from_api, response.nodes),
            next_page_token=response.next_page_token,
        )

    def list_nodes(self, node_group_id: str) -> Iterable[Node]:
        request = node_group_service_pb2.ListNodeGroupNodesRequest()
        request.node_group_id = node_group_id
        request.page_size = self.config.page_size
        return paginate(self._list_nodes, request)

    @client_retry
    @tracing.trace('Create kubernetes cluster')
    def create_cluster(
        self,
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
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('folder_id', folder_id)
        is_both_set = master_spec.zonal_master and master_spec.regional_master
        assert not is_both_set, 'Both zonal_master and regional_master should not be set'

        master_spec_pb = cluster_service_pb2.MasterSpec()
        if master_spec.zonal_master:
            zonal_master_spec = cluster_service_pb2.ZonalMasterSpec(zone_id=master_spec.zonal_master.zone_id)
            if master_spec.zonal_master.internal_v4_address_spec:
                zonal_master_spec.internal_v4_address_spec.subnet_id = (
                    master_spec.zonal_master.internal_v4_address_spec.subnet_id
                )
            if master_spec.zonal_master.external_v4_address_spec:
                zonal_master_spec.external_v4_address_spec.address = (
                    master_spec.zonal_master.external_v4_address_spec.address
                )
            if master_spec.zonal_master.external_v6_address_spec:
                zonal_master_spec.external_v6_address_spec.address = (
                    master_spec.zonal_master.external_v6_address_spec.address
                )
            master_spec_pb.zonal_master_spec.CopyFrom(zonal_master_spec)

        if master_spec.regional_master:
            regional_master_spec = cluster_service_pb2.RegionalMasterSpec(
                region_id=master_spec.regional_master.region_id
            )
            if master_spec.regional_master.region_id:
                regional_master_spec.regional_master.region_id = master_spec.regional_master.region_id
            if master_spec.regional_master.locations:
                locations = [
                    cluster_service_pb2.MasterLocation(
                        zone_id=location.zone_id,
                        internal_v4_address_spec=location.internal_v4_address_spec,
                    )
                    for location in master_spec.regional_master.locations
                ]
                regional_master_spec.regional_master.locations = locations
            if master_spec.regional_master.external_v4_address_spec:
                regional_master_spec.regional_master.external_v4_address_spec.address = (
                    master_spec.regional_master.external_v4_address_spec.address
                )
            if master_spec.regional_master.external_v6_address_spec:
                regional_master_spec.regional_master.external_v6_address_spec.address = (
                    master_spec.regional_master.external_v6_address_spec.address
                )
            master_spec_pb.regional_master.CopyFrom(regional_master_spec)

        request = cluster_service_pb2.CreateClusterRequest(
            folder_id=folder_id,
            network_id=network_id,
            service_account_id=service_account_id,
            node_service_account_id=node_service_account_id,
            ip_allocation_policy=ip_allocation_policy,
            release_channel=release_channel,
            name=name,
            description=description,
            labels=labels,
            master_spec=master_spec_pb,
        )

        response = self.cluster_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = cluster_service_pb2.CreateClusterMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.cluster_id

    @client_retry
    @tracing.trace('Delete kubernetes cluster')
    def delete_cluster(
        self,
        cluster_id: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('kubernetes.cluster_id', cluster_id)
        request = cluster_service_pb2.DeleteClusterRequest(
            cluster_id=cluster_id,
        )
        response = self.cluster_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = cluster_service_pb2.DeleteClusterMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.cluster_id

    @client_retry
    @tracing.trace('Stop kubernetes cluster')
    def stop_cluster(
        self,
        cluster_id: str,
        service_account_id: Optional[str] = None,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('kubernetes.cluster_id', cluster_id)
        request = cluster_service_pb2.StopClusterRequest(
            cluster_id=cluster_id,
            service_account_id=service_account_id,
        )
        response = self.cluster_service.Stop(request, idempotency_key=idempotency_key)
        response_metadata = cluster_service_pb2.StopClusterMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.cluster_id

    @client_retry
    @tracing.trace('Start kubernetes cluster')
    def start_cluster(
        self,
        cluster_id: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('kubernetes.cluster_id', cluster_id)
        request = cluster_service_pb2.StartClusterRequest(
            cluster_id=cluster_id,
        )
        response = self.cluster_service.Start(request, idempotency_key=idempotency_key)
        response_metadata = cluster_service_pb2.StartClusterMetadata()
        response.metadata.Unpack(response_metadata)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.cluster_id

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

    @client_retry
    @tracing.trace('Create kubernetes node group')
    def create_node_group(
        self,
        cluster_id: str,
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
        node_taints: Optional[List[Taint]] = (),
        node_labels: Optional[Dict[str, str]] = None,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('kubernetes.cluster_id', cluster_id)
        request = node_group_service_pb2.CreateNodeGroupRequest(
            cluster_id=cluster_id,
            name=name,
            description=description,
            labels=labels,
            version=version,
            allowed_unsafe_sysctls=allowed_unsafe_sysctls,
            specific_revision=specific_revision,
            node_labels=node_labels,
        )

        request.node_template.CopyFrom(
            node_pb2.NodeTemplate(
                platform_id=node_template.platform_id,
                metadata=node_template.metadata,
            )
        )

        if node_template.resources_spec:
            request.node_template.resources_spec.CopyFrom(
                node_pb2.ResourcesSpec(
                    memory=node_template.resources_spec.memory,
                    cores=node_template.resources_spec.cores,
                    core_fraction=node_template.resources_spec.core_fraction,
                    gpus=node_template.resources_spec.gpus,
                )
            )

        if node_template.boot_disk_spec:
            request.node_template.boot_disk_spec.CopyFrom(
                node_pb2.ResourcesSpec(
                    disk_type_id=node_template.boot_disk_spec.disk_type_id,
                    disk_size=node_template.boot_disk_spec.disk_size,
                )
            )

        if node_template.v4_address_spec:
            request.node_template.v4_address_spec.CopyFrom(
                node_pb2.NodeAddressSpec(
                    one_to_one_nat_spec=node_pb2.OneToOneNatSpec(
                        ip_version=node_template.v4_address_spec.one_to_one_nat_spec.ip_version,
                    )
                )
            )

        if node_template.scheduling_policy:
            request.node_template.scheduling_policy.CopyFrom(
                node_pb2.SchedulingPolicy(
                    preemptible=node_template.scheduling_policy.preemptible,
                )
            )

        if node_template.network_interface_specs:
            request.node_template.network_interface_specs.extend(
                spec.to_api() for spec in node_template.network_interface_specs
            )

        if node_template.placement_policy:
            request.node_template.placement_policy.CopyFrom(
                node_pb2.PlacementPolicy(
                    placement_group_id=node_template.placement_policy.placement_group_id,
                )
            )

        if node_template.network_settings:
            request.node_template.network_settings.CopyFrom(
                node_pb2.NodeTemplate.NetworkSettings(
                    type=node_template.network_settings.type.value,
                )
            )

        if node_template.container_runtime_settings:
            request.node_template.container_runtime_settings.CopyFrom(
                node_pb2.NodeTemplate.ContainerRuntimeSettings(
                    type=node_template.container_runtime_settings.type.value,
                )
            )

        if scale_policy:
            policy = node_group_pb2.ScalePolicy()
            if scale_policy.fixed_scale:
                policy.fixed_scale.CopyFrom(
                    node_group_pb2.ScalePolicy.FixedScale(
                        size=scale_policy.fixed_scale.size,
                    )
                )
            if scale_policy.auto_scale:
                policy.auto_scale.CopyFrom(
                    node_group_pb2.ScalePolicy.AutoScale(
                        min_size=scale_policy.auto_scale.min_size,
                        max_size=scale_policy.auto_scale.max_size,
                        initial_size=scale_policy.auto_scale.initial_size,
                    )
                )
            request.scale_policy.CopyFrom(policy)

        if allocation_policy:
            locations = []
            for location in allocation_policy.locations:
                if location:
                    locations.append(
                        node_group_pb2.NodeGroupLocation(zone_id=location.zone_id, subnet_id=location.subnet_id)
                    )
            request.allocation_policy.CopyFrom(node_group_pb2.NodeGroupAllocationPolicy(locations=locations))

        if deploy_policy:
            request.deploy_policy.CopyFrom(
                node_group_pb2.DeployPolicy(
                    max_unavailable=deploy_policy.max_unavailable,
                    max_expansion=deploy_policy.max_expansion,
                )
            )

        if maintenance_policy:
            request.maintenance_policy.CopyFrom(
                node_group_pb2.NodeGroupMaintenancePolicy(
                    auto_repair=maintenance_policy.auto_repair,
                    auto_upgrade=maintenance_policy.auto_upgrade,
                )
            )

        if node_taints:
            request.node_taints = [
                node_pb2.Taint(
                    key=taint.key,
                    value=taint.value,
                    effect=taint.effect,
                )
                for taint in node_taints
            ]

        response = self.node_group_service.Create(request, idempotency_key=idempotency_key)
        response_metadata = node_group_service_pb2.CreateNodeGroupMetadata()
        response.metadata.Unpack(response_metadata)
        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.node_group_id

    @client_retry
    @tracing.trace('Delete kubernetes node group')
    def delete_node_group(
        self,
        node_group_id: str,
        idempotency_key: Optional[str] = None,
        wait: bool = False,
    ) -> Tuple[str, str]:
        tracing.set_tag('kubernetes.node_group', node_group_id)
        request = node_group_service_pb2.DeleteNodeGroupRequest(
            node_group_id=node_group_id,
        )
        response = self.node_group_service.Delete(request, idempotency_key=idempotency_key)
        response_metadata = node_group_service_pb2.DeleteNodeGroupMetadata()
        response.metadata.Unpack(response_metadata)

        operation = OperationModel().operation_from_api(
            self.logger, response, self.operation_service.error_from_rich_status
        )
        if wait:
            operation = self.wait_operation(operation.operation_id)
        return operation.operation_id, response_metadata.node_group_id
