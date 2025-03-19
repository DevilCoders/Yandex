from datetime import datetime
from enum import Enum
from typing import Dict, List, NamedTuple, Optional
from dataclasses import dataclass

from google.protobuf.json_format import MessageToDict
from google.protobuf.wrappers_pb2 import Int64Value

from yandex.cloud.priv.compute.v1 import instance_service_pb2, instance_pb2
from yandex.cloud.priv.reference import reference_pb2

from cloud.mdb.internal.python.compute.models import enum_from_api


class InstanceStatus(Enum):
    STATUS_UNSPECIFIED = instance_pb2.Instance.Status.STATUS_UNSPECIFIED
    PROVISIONING = instance_pb2.Instance.Status.PROVISIONING
    RUNNING = instance_pb2.Instance.Status.RUNNING
    STOPPING = instance_pb2.Instance.Status.STOPPING
    STOPPED = instance_pb2.Instance.Status.STOPPED
    STARTING = instance_pb2.Instance.Status.STARTING
    RESTARTING = instance_pb2.Instance.Status.RESTARTING
    UPDATING = instance_pb2.Instance.Status.UPDATING
    ERROR = instance_pb2.Instance.Status.ERROR
    CRASHED = instance_pb2.Instance.Status.CRASHED
    DELETING = instance_pb2.Instance.Status.DELETING

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, InstanceStatus)


class NetworkType(Enum):
    TYPE_UNSPECIFIED = instance_pb2.NetworkSettings.Type.TYPE_UNSPECIFIED
    STANDARD = instance_pb2.NetworkSettings.Type.STANDARD
    SOFTWARE_ACCELERATED = instance_pb2.NetworkSettings.Type.SOFTWARE_ACCELERATED
    HARDWARE_ACCELERATED = instance_pb2.NetworkSettings.Type.HARDWARE_ACCELERATED

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, NetworkType)


class NetworkSettings(NamedTuple):
    type: NetworkType

    @staticmethod
    def from_api(raw_data):
        return NetworkSettings(type=NetworkType.from_api(raw_data.type))


class Resources(NamedTuple):
    memory: int
    cores: int
    gpus: int
    core_fractions: int
    nvme_disks: int

    @staticmethod
    def from_api(raw_data):
        return Resources(
            memory=raw_data.memory,
            cores=raw_data.cores,
            gpus=raw_data.gpus,
            core_fractions=raw_data.core_fraction,
            nvme_disks=raw_data.nvme_disks,
        )


class DiskMode(Enum):
    UNSPECIFIED = instance_pb2.AttachedDisk.Mode.MODE_UNSPECIFIED
    READ_ONLY = instance_pb2.AttachedDisk.Mode.READ_ONLY
    READ_WRITE = instance_pb2.AttachedDisk.Mode.READ_WRITE

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, DiskMode)


class DiskStatus(Enum):
    UNSPECIFIED = instance_pb2.AttachedDisk.Status.STATUS_UNSPECIFIED
    ATTACHING = instance_pb2.AttachedDisk.Status.ATTACHING
    ATTACHED = instance_pb2.AttachedDisk.Status.ATTACHED
    DETACHING = instance_pb2.AttachedDisk.Status.DETACHING
    DETACH_ERROR = instance_pb2.AttachedDisk.Status.DETACH_ERROR

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, DiskStatus)


@dataclass
class AttachedDiskRequest:
    disk_id: str


class SubnetRequest(NamedTuple):
    v4_cidr: bool
    v6_cidr: bool
    subnet_id: str


class DnsRecordSpec(NamedTuple):
    fqdn: str
    dns_zone_id: str
    ptr: bool
    ttl: int


class InstanceDnsSpec(NamedTuple):
    # After CLOUD-63662 we can use DnsRecordSpec
    # to specify DNS records for V4 public_ips
    eth0_ip6: Optional[DnsRecordSpec] = None
    eth1_ip6: Optional[DnsRecordSpec] = None


class AttachedDisk(NamedTuple):
    auto_delete: bool
    device_name: str
    mode: DiskMode
    disk_id: str
    status: DiskStatus

    @staticmethod
    def from_api(raw_data):
        return AttachedDisk(
            auto_delete=raw_data.auto_delete,
            device_name=raw_data.device_name,
            mode=DiskMode.from_api(raw_data.mode),
            disk_id=raw_data.disk_id,
            status=DiskStatus.from_api(raw_data.status),
        )


class OneToOneNat(NamedTuple):
    address: str

    @staticmethod
    def from_api(raw_data: instance_pb2.OneToOneNat):
        return OneToOneNat(address=raw_data.address)


class DnsRecord(NamedTuple):
    fqdn: str

    @staticmethod
    def from_api(raw_data: instance_pb2.DnsRecord):
        return DnsRecord(fqdn=raw_data.fqdn)


class Address(NamedTuple):
    address: str
    one_to_one_nat: OneToOneNat
    dns_records: List[DnsRecord]

    @staticmethod
    def from_api(raw_data: instance_pb2.PrimaryAddress):
        return Address(
            address=raw_data.address,
            one_to_one_nat=OneToOneNat.from_api(raw_data.one_to_one_nat),
            dns_records=list(map(DnsRecord.from_api, raw_data.dns_records)),
        )

    def __bool__(self):
        return bool(self.address)


class NetworkInterface(NamedTuple):
    primary_v4_address: Address
    primary_v6_address: Address
    index: int
    subnet_id: str
    security_group_ids: List[str]

    @staticmethod
    def from_api(raw_data):
        result = NetworkInterface(
            index=raw_data.index,
            subnet_id=raw_data.subnet_id,
            primary_v4_address=Address.from_api(raw_data.primary_v4_address),
            primary_v6_address=Address.from_api(raw_data.primary_v6_address),
            security_group_ids=list(raw_data.security_group_ids),
        )
        return result


class InstanceModel(NamedTuple):
    id: str
    folder_id: str
    status: InstanceStatus
    fqdn: str
    name: str
    service_account_id: str
    platform_id: str
    zone_id: str
    metadata: Dict[str, str]
    labels: Dict[str, str]
    network_settings: NetworkSettings
    resources: Resources
    boot_disk: AttachedDisk
    secondary_disks: List[AttachedDisk]
    network_interfaces: List[NetworkInterface]
    created_at: datetime
    _grpc_model = instance_pb2.Instance

    @staticmethod
    def from_api(raw_data: instance_pb2.Instance):
        return InstanceModel(
            id=raw_data.id,
            folder_id=raw_data.folder_id,
            status=InstanceStatus.from_api(raw_data.status),
            fqdn=raw_data.fqdn,
            name=raw_data.name,
            service_account_id=raw_data.service_account_id,
            platform_id=raw_data.platform_id,
            zone_id=raw_data.zone_id,
            metadata=raw_data.metadata,
            labels=raw_data.labels,
            network_settings=NetworkSettings.from_api(raw_data.network_settings),
            resources=Resources.from_api(raw_data.resources),
            boot_disk=AttachedDisk.from_api(raw_data.boot_disk),
            secondary_disks=list(map(AttachedDisk.from_api, raw_data.secondary_disks)),
            network_interfaces=list(map(NetworkInterface.from_api, raw_data.network_interfaces)),
            created_at=raw_data.created_at.ToDatetime(),
        )

    def __repr__(self):
        return f'InstanceModel id={self.id}'


class InstanceView(Enum):
    BASIC = instance_service_pb2.InstanceView.BASIC
    FULL = instance_service_pb2.InstanceView.FULL

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, InstanceView)


class ReferrerType(Enum):
    DATAPROC_CLUSTER = 'dataproc.cluster'
    DATAPROC_SUBCLUSTER = 'dataproc.subcluster'
    MANAGED_MYSQL_CLUSTER = 'managed-mysql.cluster'
    MDB_CMS = 'mdb.cms'

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, ReferrerType)


class Referrer(NamedTuple):
    type: ReferrerType
    id: str

    @staticmethod
    def from_api(raw_data: reference_pb2.Referrer):
        return Referrer(type=raw_data.type, id=raw_data.id)


class ReferenceType(Enum):
    TYPE_UNSPECIFIED = reference_pb2.Reference.Type.TYPE_UNSPECIFIED
    MANAGED_BY = reference_pb2.Reference.Type.MANAGED_BY
    USED_BY = reference_pb2.Reference.Type.USED_BY

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, ReferenceType)


class Reference(NamedTuple):
    referrer: Referrer
    type: ReferenceType

    @staticmethod
    def from_api(raw_data: reference_pb2.Reference) -> 'Reference':
        return Reference(
            referrer=Referrer.from_api(raw_data.referrer),
            type=ReferenceType.from_api(raw_data.type),
        )


class Metric(NamedTuple):
    schema: str
    tags: dict
    cloud_id: str
    folder_id: str

    @staticmethod
    def from_api(raw_data):
        tags = {}
        for key, val in raw_data.tags.items():
            if val.Is(Int64Value.DESCRIPTOR):
                # MessageToDict will serialize Int64Value as string, but we want int
                int_val = Int64Value()
                val.Unpack(int_val)
                tags[key] = int_val.value
            else:
                tags[key] = MessageToDict(val)['value']
        return Metric(
            schema=raw_data.schema,
            tags=tags,
            cloud_id=raw_data.cloud_id,
            folder_id=raw_data.folder_id,
        )
