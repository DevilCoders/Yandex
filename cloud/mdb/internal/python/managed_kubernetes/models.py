from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import NamedTuple, Union, List, Optional, Dict

from yandex.cloud.priv.k8s.v1 import (
    cluster_pb2,
    version_pb2,
    node_pb2,
    node_group_pb2,
)

from cloud.mdb.internal.python.compute.models import enum_from_api


KiB = 1024
MiB = 1024 * KiB
GiB = 1024 * MiB


class MasterAuth(NamedTuple):
    cluster_ca_certificate: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.MasterAuth):
        return MasterAuth(
            cluster_ca_certificate=grpc_response.cluster_ca_certificate,
        )


class InternalAddressSpec(NamedTuple):
    subnet_id: str


class ExternalAddressSpec(NamedTuple):
    address: str


class ZonalMasterSpec(NamedTuple):
    zone_id: str
    internal_v4_address_spec: Optional[InternalAddressSpec] = None
    external_v4_address_spec: Optional[ExternalAddressSpec] = None
    external_v6_address_spec: Optional[ExternalAddressSpec] = None


class MasterLocation(NamedTuple):
    zone_id: str
    internal_v4_address_spec: Optional[InternalAddressSpec] = None


class RegionalMasterSpec(NamedTuple):
    region_id: str
    locations: Optional[List[MasterLocation]] = None
    external_v4_address_spec: Optional[ExternalAddressSpec] = None
    external_v6_address_spec: Optional[ExternalAddressSpec] = None


class ZonalMaster(NamedTuple):
    zone_id: str
    internal_v4_address: str
    external_v4_address: str
    service_v6_address: str
    external_v6_address: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.ZonalMaster):
        return ZonalMaster(
            zone_id=grpc_response.zone_id,
            internal_v4_address=grpc_response.internal_v4_address,
            external_v4_address=grpc_response.external_v4_address,
            service_v6_address=grpc_response.service_v6_address,
            external_v6_address=grpc_response.external_v6_address,
        )


class RegionalMaster(NamedTuple):
    region_id: str
    internal_v4_address: str
    external_v4_address: str
    service_v6_address: str
    external_v6_address: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.RegionalMaster):
        return RegionalMaster(
            region_id=grpc_response.region_id,
            internal_v4_address=grpc_response.internal_v4_address,
            external_v4_address=grpc_response.external_v4_address,
            service_v6_address=grpc_response.service_v6_address,
            external_v6_address=grpc_response.external_v6_address,
        )


class Provider(Enum):
    UNKNOWN = cluster_pb2.NetworkPolicy.PROVIDER_UNSPECIFIED
    CALICO = cluster_pb2.NetworkPolicy.CALICO

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, Provider)


class NetworkPolicy(NamedTuple):
    provider: Provider

    @staticmethod
    def from_api(grpc_response: cluster_pb2.NetworkPolicy):
        return NetworkPolicy(
            provider=Provider.from_api(grpc_response.provider),
        )


class KMSProvider(NamedTuple):
    key_id: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.KMSProvider):
        return KMSProvider(
            key_id=grpc_response.key_id,
        )


class RoutingMode(Enum):
    UNKNOWN = cluster_pb2.Cilium.RoutingMode.ROUTING_MODE_UNSPECIFIED
    TUNNEL = cluster_pb2.Cilium.RoutingMode.TUNNEL

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, RoutingMode)


class Cilium(NamedTuple):
    routing_mode: RoutingMode

    @staticmethod
    def from_api(grpc_response: cluster_pb2.Cilium):
        return Cilium(
            routing_mode=RoutingMode.from_api(grpc_response.routing_mode),
        )


class MasterEndpoints(NamedTuple):
    internal_v4_endpoint: str
    external_v4_endpoint: str
    service_v6_endpoint: str
    external_v6_endpoint: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.MasterEndpoints):
        return MasterEndpoints(
            internal_v4_endpoint=grpc_response.internal_v4_endpoint,
            external_v4_endpoint=grpc_response.external_v4_endpoint,
            service_v6_endpoint=grpc_response.service_v6_endpoint,
            external_v6_endpoint=grpc_response.external_v6_endpoint,
        )


class IPAllocationPolicy(NamedTuple):
    cluster_ipv4_cidr_block: str
    node_ipv4_cidr_mask_size: int
    service_ipv4_cidr_block: str
    cluster_ipv6_cidr_block: str
    service_ipv6_cidr_block: str

    @staticmethod
    def from_api(grpc_response: cluster_pb2.IPAllocationPolicy):
        return IPAllocationPolicy(
            cluster_ipv4_cidr_block=grpc_response.cluster_ipv4_cidr_block,
            node_ipv4_cidr_mask_size=grpc_response.node_ipv4_cidr_mask_size,
            service_ipv4_cidr_block=grpc_response.service_ipv4_cidr_block,
            cluster_ipv6_cidr_block=grpc_response.cluster_ipv6_cidr_block,
            service_ipv6_cidr_block=grpc_response.service_ipv6_cidr_block,
        )


class ReleaseChannel(Enum):
    UNKNOWN = cluster_pb2.ReleaseChannel.RELEASE_CHANNEL_UNSPECIFIED
    RAPID = cluster_pb2.ReleaseChannel.RAPID
    REGULAR = cluster_pb2.ReleaseChannel.REGULAR
    STABLE = cluster_pb2.ReleaseChannel.STABLE

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ReleaseChannel)


class ClusterStatus(Enum):
    UNKNOWN = cluster_pb2.Cluster.Status.STATUS_UNSPECIFIED
    PROVISIONING = cluster_pb2.Cluster.Status.PROVISIONING
    RUNNING = cluster_pb2.Cluster.Status.RUNNING
    RECONCILING = cluster_pb2.Cluster.Status.RECONCILING
    STOPPING = cluster_pb2.Cluster.Status.STOPPING
    STOPPED = cluster_pb2.Cluster.Status.STOPPED
    DELETING = cluster_pb2.Cluster.Status.DELETING
    STARTING = cluster_pb2.Cluster.Status.STARTING

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ClusterStatus)


class ClusterHealth(Enum):
    UNKNOWN = cluster_pb2.Cluster.Health.HEALTH_UNSPECIFIED
    HEALTHY = cluster_pb2.Cluster.Health.HEALTHY
    UNHEALTHY = cluster_pb2.Cluster.Health.UNHEALTHY

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ClusterHealth)


class MasterMaintenancePolicy(NamedTuple):
    auto_upgrade: bool

    @staticmethod
    def from_api(grpc_response: cluster_pb2.MasterMaintenancePolicy):
        return MasterMaintenancePolicy(
            auto_upgrade=grpc_response.auto_upgrade,
        )


class VersionInfo(NamedTuple):
    current_version: str
    current_revision: int
    new_revision_available: bool
    new_revision_summary: str
    version_deprecated: bool

    @staticmethod
    def from_api(grpc_response: version_pb2.VersionInfo):
        return VersionInfo(
            current_version=grpc_response.current_version,
            current_revision=grpc_response.current_revision,
            new_revision_available=grpc_response.new_revision_available,
            new_revision_summary=grpc_response.new_revision_summary,
            version_deprecated=grpc_response.version_deprecated,
        )


class Master(NamedTuple):
    master: Union[ZonalMaster, RegionalMaster]
    version: str
    endpoints: MasterEndpoints
    master_auth: MasterAuth
    version_info: VersionInfo
    maintenance_policy: MasterMaintenancePolicy
    security_group_ids: List[str]

    @staticmethod
    def from_api(grpc_response: cluster_pb2.Master):
        if hasattr(grpc_response, 'zonal_master'):
            master = ZonalMaster.from_api(grpc_response.zonal_master)
        elif hasattr(grpc_response, 'regional_master'):
            master = RegionalMaster.from_api(grpc_response.regional_master)
        else:
            raise TypeError('master_type must be either ZonalMaster or RegionalMaster')
        return Master(
            master=master,
            version=grpc_response.version,
            endpoints=MasterEndpoints.from_api(grpc_response.endpoints),
            master_auth=MasterAuth.from_api(grpc_response.master_auth),
            version_info=VersionInfo.from_api(grpc_response.version_info),
            maintenance_policy=MasterMaintenancePolicy.from_api(grpc_response.maintenance_policy),
            security_group_ids=grpc_response.security_group_ids,
        )


class MasterSpec(NamedTuple):
    zonal_master: Optional[ZonalMasterSpec] = None
    regional_master: Optional[RegionalMasterSpec] = None
    version: Optional[str] = None
    specific_revision: Optional[int] = None
    security_group_ids: Optional[List[str]] = None


class ClusterModel(NamedTuple):
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: dict
    status: ClusterStatus
    health: ClusterHealth
    network_id: str
    log_group_id: str
    service_account_id: str
    node_service_account_id: str
    master: Master
    ip_allocation_policy: IPAllocationPolicy
    release_channel: ReleaseChannel
    network_policy: NetworkPolicy
    kms_provider: KMSProvider
    internet_gateway: Union[str]
    network_implementation: Union[Cilium]

    @staticmethod
    def from_api(grpc_response: cluster_pb2.Cluster):
        return ClusterModel(
            id=grpc_response.id,
            name=grpc_response.name,
            folder_id=grpc_response.folder_id,
            labels=grpc_response.labels,
            description=grpc_response.description,
            created_at=grpc_response.created_at.ToDatetime(),
            status=ClusterStatus.from_api(grpc_response.status),
            health=ClusterHealth.from_api(grpc_response.health),
            network_id=grpc_response.network_id,
            log_group_id=grpc_response.log_group_id,
            service_account_id=grpc_response.service_account_id,
            node_service_account_id=grpc_response.node_service_account_id,
            master=Master.from_api(grpc_response.master),
            ip_allocation_policy=IPAllocationPolicy.from_api(grpc_response.ip_allocation_policy),
            release_channel=ReleaseChannel.from_api(grpc_response.release_channel),
            network_policy=NetworkPolicy.from_api(grpc_response.network_policy),
            kms_provider=KMSProvider.from_api(grpc_response.kms_provider),
            internet_gateway=grpc_response.gateway_ipv4_address,
            network_implementation=Cilium.from_api(grpc_response.cilium),
        )


class DeployPolicy(NamedTuple):
    max_unavailable: int
    max_expansion: int

    @staticmethod
    def from_api(grpc_response: node_group_pb2.DeployPolicy):
        return DeployPolicy(
            max_unavailable=grpc_response.max_unavailable,
            max_expansion=grpc_response.max_expansion,
        )


class NodeGroupMaintenancePolicy(NamedTuple):
    auto_upgrade: bool
    auto_repair: bool

    @staticmethod
    def from_api(grpc_response: node_group_pb2.NodeGroupMaintenancePolicy):
        return NodeGroupMaintenancePolicy(
            auto_upgrade=grpc_response.auto_upgrade,
            auto_repair=grpc_response.auto_repair,
        )


class NodeGroupLocation(NamedTuple):
    zone_id: str
    subnet_id: str

    @staticmethod
    def from_api(grpc_response: node_group_pb2.NodeGroupLocation):
        return NodeGroupLocation(
            zone_id=grpc_response.zone_id,
            subnet_id=grpc_response.subnet_id,
        )


class NodeGroupAllocationPolicy(NamedTuple):
    locations: List[NodeGroupLocation]

    @staticmethod
    def from_api(grpc_response: node_group_pb2.NodeGroupAllocationPolicy):
        return NodeGroupAllocationPolicy(
            locations=grpc_response.locations,
        )


class FixedScale(NamedTuple):
    size: int

    @staticmethod
    def from_api(grpc_response: node_group_pb2.ScalePolicy.FixedScale):
        return FixedScale(
            size=grpc_response.size,
        )


class AutoScale(NamedTuple):
    min_size: int
    max_size: int
    initial_size: int

    @staticmethod
    def from_api(grpc_response: node_group_pb2.ScalePolicy.AutoScale):
        return AutoScale(
            min_size=grpc_response.min_size,
            max_size=grpc_response.max_size,
            initial_size=grpc_response.initial_size,
        )


class ScalePolicy(NamedTuple):
    fixed_scale: FixedScale = None
    auto_scale: AutoScale = None

    @staticmethod
    def from_api(grpc_response: node_group_pb2.ScalePolicy):
        return ScalePolicy(
            fixed_scale=grpc_response.fixed_scale,
            auto_scale=grpc_response.auto_scale,
        )


class NodeGroupStatus(Enum):
    UNKNOWN = node_group_pb2.NodeGroup.STATUS_UNSPECIFIED
    PROVISIONING = node_group_pb2.NodeGroup.PROVISIONING
    RUNNING = node_group_pb2.NodeGroup.RUNNING
    RECONCILING = node_group_pb2.NodeGroup.RECONCILING
    STOPPING = node_group_pb2.NodeGroup.STOPPING
    STOPPED = node_group_pb2.NodeGroup.STOPPED
    DELETING = node_group_pb2.NodeGroup.DELETING
    STARTING = node_group_pb2.NodeGroup.STARTING

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, NodeGroupStatus)


class PlacementPolicy(NamedTuple):
    placement_group_id: str

    @staticmethod
    def from_api(grpc_response: node_pb2.PlacementPolicy):
        return PlacementPolicy(
            placement_group_id=grpc_response.placement_group_id,
        )


class SchedulingPolicy(NamedTuple):
    preemptible: bool

    @staticmethod
    def from_api(grpc_response: node_pb2.SchedulingPolicy):
        return SchedulingPolicy(
            preemptible=grpc_response.preemptible,
        )


class DiskSpec(NamedTuple):
    disk_type_id: str
    disk_size: int

    @staticmethod
    def from_api(grpc_response: node_pb2.DiskSpec):
        return DiskSpec(
            disk_type_id=grpc_response.disk_type_id,
            disk_size=grpc_response.disk_size,
        )


class ResourcesSpec(NamedTuple):
    memory: int
    cores: int
    core_fraction: int = None
    gpus: int = None

    @staticmethod
    def from_api(grpc_response: node_pb2.ResourcesSpec):
        return ResourcesSpec(
            memory=grpc_response.memory,
            cores=grpc_response.cores,
            core_fraction=grpc_response.core_fraction,
            gpus=grpc_response.gpus,
        )


class IpVersion(Enum):
    UNKNOWN = node_pb2.IpVersion.IP_VERSION_UNSPECIFIED
    IPV4 = node_pb2.IpVersion.IPV4
    IPV6 = node_pb2.IpVersion.IPV6

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, IpVersion)


class OneToOneNatSpec(NamedTuple):
    ip_version: IpVersion

    @staticmethod
    def from_api(grpc_response: node_pb2.OneToOneNatSpec):
        if grpc_response:
            return OneToOneNatSpec(
                ip_version=IpVersion.from_api(grpc_response.ip_version),
            )

    def to_api(self) -> node_pb2.OneToOneNatSpec:
        return node_pb2.OneToOneNatSpec(ip_version=self.ip_version.value)


class NodeAddressSpec(NamedTuple):
    one_to_one_nat_spec: Optional[OneToOneNatSpec]

    @staticmethod
    def from_api(grpc_response: node_pb2.NodeAddressSpec):
        return NodeAddressSpec(
            one_to_one_nat_spec=OneToOneNatSpec.from_api(grpc_response.one_to_one_nat_spec),
        )

    def to_api(self) -> node_pb2.NodeAddressSpec:
        return node_pb2.NodeAddressSpec(
            one_to_one_nat_spec=self.one_to_one_nat_spec and self.one_to_one_nat_spec.to_api()
        )


class NetworkInterfaceSpec(NamedTuple):
    subnet_ids: List[str]
    security_group_ids: List[str]
    primary_v4_address_spec: NodeAddressSpec = NodeAddressSpec(one_to_one_nat_spec=None)
    primary_v6_address_spec: Optional[NodeAddressSpec] = None

    @staticmethod
    def from_api(grpc_response: node_pb2.NetworkInterfaceSpec):
        return NetworkInterfaceSpec(
            subnet_ids=grpc_response.subnet_ids,
            security_group_ids=grpc_response.security_group_ids,
            primary_v4_address_spec=NodeAddressSpec.from_api(grpc_response.primary_v4_address_spec),
            primary_v6_address_spec=NodeAddressSpec.from_api(grpc_response.primary_v6_address_spec),
        )

    def to_api(self) -> node_pb2.NetworkInterfaceSpec:
        return node_pb2.NetworkInterfaceSpec(
            subnet_ids=self.subnet_ids,
            security_group_ids=self.security_group_ids,
            primary_v4_address_spec=self.primary_v4_address_spec.to_api(),
            primary_v6_address_spec=self.primary_v6_address_spec and self.primary_v6_address_spec.to_api(),
        )


class NetworkSettingsType(Enum):
    UNKNOWN = node_pb2.NodeTemplate.NetworkSettings.Type.TYPE_UNSPECIFIED
    STANDARD = node_pb2.NodeTemplate.NetworkSettings.Type.STANDARD
    SOFTWARE_ACCELERATED = node_pb2.NodeTemplate.NetworkSettings.Type.SOFTWARE_ACCELERATED

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, NetworkSettingsType)


class ContainerRuntimeSettingsType(Enum):
    UNKNOWN = node_pb2.NodeTemplate.ContainerRuntimeSettings.Type.TYPE_UNSPECIFIED
    DOCKER = node_pb2.NodeTemplate.ContainerRuntimeSettings.Type.DOCKER
    CONTAINERD = node_pb2.NodeTemplate.ContainerRuntimeSettings.Type.CONTAINERD

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ContainerRuntimeSettingsType)


class NetworkSettings(NamedTuple):
    type: NetworkSettingsType

    @staticmethod
    def from_api(grpc_response: node_pb2.NodeTemplate.NetworkSettings):
        return NetworkSettings(
            type=NetworkSettingsType.from_api(grpc_response.type),
        )


class ContainerRuntimeSettings(NamedTuple):
    type: ContainerRuntimeSettingsType

    @staticmethod
    def from_api(grpc_response: node_pb2.NodeTemplate.ContainerRuntimeSettings):
        return ContainerRuntimeSettings(
            type=ContainerRuntimeSettingsType.from_api(grpc_response.type),
        )


class NodeTemplate(NamedTuple):
    platform_id: str = None
    resources_spec: ResourcesSpec = None
    boot_disk_spec: DiskSpec = None
    metadata: Dict[str, str] = None
    v4_address_spec: NodeAddressSpec = None
    scheduling_policy: SchedulingPolicy = None
    network_interface_specs: List[NetworkInterfaceSpec] = None
    placement_policy: PlacementPolicy = None
    network_settings: NetworkSettings = None
    container_runtime_settings: ContainerRuntimeSettings = None

    @staticmethod
    def from_api(grpc_response: node_pb2.NodeTemplate):
        return NodeTemplate(
            platform_id=grpc_response.platform_id,
            resources_spec=ResourcesSpec.from_api(grpc_response.resources_spec),
            boot_disk_spec=DiskSpec.from_api(grpc_response.boot_disk_spec),
            metadata=grpc_response.metadata,
            v4_address_spec=NodeAddressSpec.from_api(grpc_response.v4_address_spec),
            scheduling_policy=SchedulingPolicy.from_api(grpc_response.scheduling_policy),
            network_interface_specs=[
                NetworkInterfaceSpec.from_api(spec) for spec in grpc_response.network_interface_specs
            ],
            placement_policy=PlacementPolicy.from_api(grpc_response.placement_policy),
            network_settings=NetworkSettings.from_api(grpc_response.network_settings),
            container_runtime_settings=ContainerRuntimeSettings.from_api(grpc_response.container_runtime_settings),
        )


class Effect(Enum):
    UNKNOWN = node_pb2.Taint.Effect.EFFECT_UNSPECIFIED
    NO_SCHEDULE = node_pb2.Taint.Effect.NO_SCHEDULE
    PREFER_NO_SCHEDULE = node_pb2.Taint.Effect.PREFER_NO_SCHEDULE
    NO_EXECUTE = node_pb2.Taint.Effect.NO_EXECUTE

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, Effect)


class Taint(NamedTuple):
    key: str
    value: str
    effect: Effect

    @staticmethod
    def from_api(grpc_response: node_pb2.Taint):
        return Taint(
            key=grpc_response.key,
            value=grpc_response.value,
            effect=Effect.from_api(grpc_response.effect),
        )


class NodeGroupModel(NamedTuple):
    id: str
    cluster_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    status: NodeGroupStatus
    node_template: NodeTemplate
    scale_policy: ScalePolicy
    allocation_policy: NodeGroupAllocationPolicy
    deploy_policy: DeployPolicy
    instance_group_id: str
    node_version: str
    version_info: VersionInfo
    maintenance_policy: NodeGroupMaintenancePolicy
    static_reserved_resources: bool
    allowed_unsafe_sysctls: List[str]
    node_taints: List[Taint]
    node_labels: Dict[str, str]

    @staticmethod
    def from_api(grpc_response: node_group_pb2.NodeGroup):
        return NodeGroupModel(
            id=grpc_response.id,
            cluster_id=grpc_response.cluster_id,
            created_at=grpc_response.created_at.ToDatetime(),
            name=grpc_response.name,
            description=grpc_response.description,
            labels=grpc_response.labels,
            status=NodeGroupStatus.from_api(grpc_response.status),
            node_template=NodeTemplate.from_api(grpc_response.node_template),
            scale_policy=ScalePolicy.from_api(grpc_response.scale_policy),
            allocation_policy=NodeGroupAllocationPolicy.from_api(grpc_response.allocation_policy),
            deploy_policy=DeployPolicy.from_api(grpc_response.deploy_policy),
            instance_group_id=grpc_response.instance_group_id,
            node_version=grpc_response.node_version,
            version_info=VersionInfo.from_api(grpc_response.version_info),
            maintenance_policy=NodeGroupMaintenancePolicy.from_api(grpc_response.maintenance_policy),
            static_reserved_resources=grpc_response.static_reserved_resources,
            allowed_unsafe_sysctls=grpc_response.allowed_unsafe_sysctls,
            node_taints=[Taint.from_api(node_taint) for node_taint in grpc_response.node_taints],
            node_labels=grpc_response.node_labels,
        )


@dataclass
class CloudStatus:
    id: str

    @staticmethod
    def from_api(grpc_response: node_pb2.Node.CloudStatus):
        return CloudStatus(
            id=grpc_response.id,
        )


@dataclass
class Node:
    cloud_status: CloudStatus

    @staticmethod
    def from_api(grpc_response: node_pb2.Node):
        return Node(
            cloud_status=CloudStatus.from_api(grpc_response.cloud_status),
        )
