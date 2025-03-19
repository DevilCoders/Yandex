import enum

from schematics.types import BooleanType

from yc_common.clients.models import base as base_models
from yc_common.clients.models.networks import NetworkSettings, NetworkInterface, UnderlayNetwork
from yc_common.clients.models.disks import AttachedDisk
from yc_common.clients.models import operations as operations_models
from yc_common.clients.models import operating_system as os_models

from yc_common.fields import ModelType, ListType, StringType, DictType, IntType
from yc_common.models import StringEnumType, get_metadata
from yc_common.validation import HostnameType, ResourceIdType


class InstanceStatus:
    PROVISIONING = "provisioning"
    STARTING = "starting"
    RUNNING = "running"
    MIGRATING = "migrating"  # Private API only
    STOPPING = "stopping"
    STOPPED = "stopped"
    RESTARTING = "restarting"
    ERROR = "error"
    CRASHED = "crashed"
    DELETING = "deleting"
    UPDATING = "updating"

    ALL_ERROR = [ERROR, CRASHED]
    ALL = ALL_ERROR + [PROVISIONING, STARTING, RUNNING, UPDATING, STOPPING, STOPPED, RESTARTING, DELETING]


@enum.unique
class InstanceView(enum.IntEnum):
    BASIC = 0
    FULL = 1


class InstanceResourcesSpec(base_models.BasePublicModel):
    cores = IntType(min_value=1, required=True)
    core_fraction = IntType(min_value=1, max_value=100, default=100)

    memory = IntType(required=True)
    nvme_disks = IntType(min_value=0, metadata=get_metadata(internal=True))

    gpus = IntType(min_value=0)


class InstanceResources(base_models.BasePublicModel):
    sockets = IntType(required=True)
    cores = IntType(required=True)
    core_fraction = IntType()

    memory = IntType(required=True)
    nvme_disks = IntType(min_value=0, metadata=get_metadata(internal=True))

    # FIXME(zasimov-a): gpus is temporary internal.
    gpus = IntType(metadata=get_metadata(internal=True))


# FIXME: Don't mark properties as internal
class SchedulingPolicyUpdate(base_models.BasePublicModel):
    deny_deallocation = BooleanType(metadata=get_metadata(internal=True))
    start_without_compute = BooleanType(metadata=get_metadata(internal=True), serialize_when_none=False)  # serialize_when_none=False for DB live migration
    disable_auto_recovery = BooleanType(serialize_when_none=False)  # serialize_when_none=False for DB live migration


class SchedulingPolicy(SchedulingPolicyUpdate):
    preemptible = BooleanType()
    preemption_deadline = IntType(
        min_value=1, metadata=get_metadata(internal=True), serialize_when_none=False)  # serialize_when_none=False for DB live migration
    termination_grace_period = IntType(
        min_value=1, max_value=1800, serialize_when_none=False)



class PlacementPolicyUpdate(base_models.BasePublicModel):
    placement_group_id = StringType()


class PlacementPolicy(base_models.BasePublicModel):
    compute_nodes = ListType(HostnameType(), min_size=1)
    host_group = StringType(min_length=1)
    placement_group_id = ResourceIdType()


class InstancePlacementRestrictions(base_models.BasePublicModel):
    compute_nodes = ListType(HostnameType(), min_size=1)
    host_group = StringType(min_length=1)


class InstanceV1Beta1(base_models.ZonalPublicObjectModelV1Beta1):
    status = StringEnumType(required=True)

    boot_disk = ModelType(AttachedDisk)  # Note: this field is required, but can be missing during instance removal process
    secondary_disks = ListType(ModelType(AttachedDisk))

    underlay_networks = ListType(ModelType(UnderlayNetwork))

    platform_id = StringType(required=True)
    resources = ModelType(InstanceResources, required=True)
    network_settings = ModelType(NetworkSettings, required=True)
    hypervisor_type = StringEnumType()
    nested_virtualization = BooleanType()
    disable_seccomp = BooleanType()
    placement_policy = ModelType(PlacementPolicy)
    scheduling_policy = ModelType(SchedulingPolicy)

    network_interfaces = ListType(ModelType(NetworkInterface))
    hostname = StringType()
    fqdn = StringType()

    metadata = DictType(StringType)

    # Special fields for UI

    os = ModelType(os_models.OperatingSystem)
    product_ids = ListType(StringType)
    disk_size = IntType()


class InstanceListV1Beta1(base_models.BaseListModel):
    instances = ListType(ModelType(InstanceV1Beta1), required=True, default=list)


Instance = InstanceV1Beta1
InstanceList = InstanceListV1Beta1


class InstanceSerialPort(base_models.BasePublicModel):
    contents = StringType()


class InstanceMetadata(operations_models.OperationMetadataV1Beta1):
    instance_id = StringType()


class InstanceOperation(operations_models.OperationV1Beta1):
    metadata = ModelType(InstanceMetadata)
    response = ModelType(InstanceV1Beta1)


class AttachInstanceDiskMetadata(InstanceMetadata):
    disk_id = StringType(required=True)


class AttachInstanceDiskOperation(InstanceOperation):
    metadata = ModelType(AttachInstanceDiskMetadata)


class DetachInstanceDiskMetadata(InstanceMetadata):
    disk_id = StringType(required=True)


class DetachInstanceDiskOperation(InstanceOperation):
    metadata = ModelType(DetachInstanceDiskMetadata)
