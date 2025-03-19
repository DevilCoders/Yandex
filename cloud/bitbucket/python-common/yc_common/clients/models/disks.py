import enum
import re

from schematics import exceptions as schematics_exceptions
from schematics.types import StringType, IntType, BooleanType, ModelType, ListType
from yc_common import constants, formatting
from yc_common.clients.models import base as base_models
from yc_common.validation import ResourceIdType, ResourceNameType, \
    ResourceDescriptionType, ZoneIdType
from yc_common.models import StringEnumType
from yc_common.clients.models import operations
from yc_common.clients.models import operating_system as os_models
from yc_common.clients.models import labels as label_models


NBS_SIZE_DIVISOR = 4 * constants.MEGABYTE
NBS_MAX_SIZE = 4 * constants.TERABYTE
DEVICE_NAME_RE = re.compile(r"^[a-z][a-z0-9-_]{,19}$")


@enum.unique
class DiskTypeId(str, enum.Enum):
    def __new__(cls, value, description, billing_name):
        obj = str.__new__(cls, value)
        obj._value_ = value
        obj.description = description
        obj.billing_name = billing_name
        return obj

    def __str__(self):
        return self._value_

    NETWORK_SSD = ("network-nvme", "Network storage with NVME backend", "network-nvme")
    NETWORK_SSD_NEW = ("network-ssd", "Network storage with SSD backend", "network-nvme")
    NETWORK_HYBRID = ("network-hdd", "Network storage with HDD backend", "network-hdd")


# Validation


class DiskSizeType(IntType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, min_value=NBS_SIZE_DIVISOR, max_value=NBS_MAX_SIZE, **kwargs)

    def validate_size(self, value, context):
        if value % NBS_SIZE_DIVISOR:
            human_divisor = formatting.human_size(NBS_SIZE_DIVISOR)
            raise schematics_exceptions.ValidationError("Size must be a multiple of {!s}".format(human_divisor))


class DiskTypeIdType(StringType):
    def __init__(self, *args, default=str(DiskTypeId.NETWORK_HYBRID), **kwargs):
        super().__init__(*args, choices=[str(type_id) for type_id in DiskTypeId], default=default, **kwargs)

    def to_native(self, value, context=None):
        if isinstance(value, DiskTypeId):
            return str(value)
        else:
            return super().to_native(value, context=context)


class DeviceNameType(StringType):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, min_length=1, max_length=20, **kwargs)

    def validate_name(self, value, context):
        if DEVICE_NAME_RE.search(value) is None:
            raise schematics_exceptions.ValidationError("Invalid device name.")


# Responses


class DiskType(base_models.BasePublicModel):
    id = StringType()
    description = StringType()


class DiskTypeList(base_models.BaseListModel):
    disk_types = ListType(ModelType(DiskType), required=True, default=list)


class DiskV1Beta1(base_models.ZonalPublicObjectModelV1Beta1):
    class Status:
        CREATING = "creating"
        READY = "ready"
        DELETING = "deleting"
        ERROR = "error"

        ALL = [CREATING, READY, DELETING, ERROR]

    status = StringEnumType(required=True)
    size = IntType(required=True)
    type_id = StringType()

    source_image_id = StringType()
    source_snapshot_id = StringType()
    product_ids = ListType(StringType)
    instance_ids = ListType(StringType())

    # Special fields for UI
    boot = BooleanType()
    os = ModelType(os_models.OperatingSystem)


class DiskListV1Beta1(base_models.BaseListModel):
    disks = ListType(ModelType(DiskV1Beta1), required=True, default=list)


Disk = DiskV1Beta1
DiskList = DiskListV1Beta1


class AttachedDiskMode:
    READ_ONLY = "read-only"
    READ_WRITE = "read-write"

    ALL = [READ_ONLY, READ_WRITE]


class AttachedDisk(base_models.BasePublicModel):
    class Status:
        ATTACHING = "attaching"
        ATTACHED = "attached"
        DETACHING = "detaching"
        DETACHED = "detached"
        DETACH_ERROR = "detach-error"
    disk_id = StringType(required=True)
    status = StringType() # FIXME: make field mandatory
    device_name = StringType(required=True)
    auto_delete = BooleanType()
    mode = StringEnumType(default=AttachedDiskMode.READ_WRITE)


class DiskMetadata(operations.OperationMetadataV1Beta1):
    disk_id = StringType(required=True)


class DiskOperation(operations.OperationV1Beta1):
    metadata = ModelType(DiskMetadata)
    response = ModelType(DiskV1Beta1)


# Requests


class DiskSpec(base_models.BasePublicModel):
    name = ResourceNameType()
    description = ResourceDescriptionType()
    labels = label_models.LabelsType()
    type_id = DiskTypeIdType()
    size = DiskSizeType(required=True)
    snapshot_id = ResourceIdType()
    image_id = ResourceIdType()


class AttachedDiskSpec(base_models.BasePublicModel):
    disk_id = ResourceIdType()
    disk_spec = ModelType(DiskSpec)
    mode = StringEnumType(choices=[AttachedDiskMode.READ_WRITE], default=AttachedDiskMode.READ_WRITE)
    device_name = DeviceNameType()
    auto_delete = BooleanType()


class PoolingEntry(base_models.BasePublicModel):
    image_id = ResourceIdType(required=True)
    zone_id = ZoneIdType(required=True)
    type_id = DiskTypeIdType(required=True)
    available = ListType(ResourceIdType, required=True, default=list)
    creating = ListType(ResourceIdType, required=True, default=list)


class PoolingResponse(base_models.BasePublicModel):
    items = ListType(ModelType(PoolingEntry), required=True, default=list)
