from schematics.types import StringType, IntType, ModelType, ListType
from yc_common.models import StringEnumType

from . import base as base_models
from . import operations
from . import operating_system as os_models


class SnapshotV1Beta1(base_models.BasePublicObjectModelV1Beta1):
    class Status:
        CREATING = "creating"
        READY = "ready"
        ERROR = "error"  # Note: This state used to be called FAILED in v1alpha1
        DELETING = "deleting"

        ALL = [CREATING, READY, ERROR, DELETING]

    status = StringEnumType(required=True)

    disk_size = IntType(default=0)
    storage_size = IntType(default=0)
    source_disk_id = StringType()
    product_ids = ListType(StringType)

    # Special fields for UI
    os = ModelType(os_models.OperatingSystem)


class SnapshotListV1Beta1(base_models.BaseListModel):
    snapshots = ListType(ModelType(SnapshotV1Beta1), required=True, default=list)


Snapshot = SnapshotV1Beta1
SnapshotList = SnapshotListV1Beta1


class SnapshotMetadata(operations.OperationMetadataV1Beta1):
    snapshot_id = StringType(required=True)


class SnapshotOperation(operations.OperationV1Beta1):
    metadata = ModelType(SnapshotMetadata)
    response = ModelType(SnapshotV1Beta1)


class CreateSnapshotMetadata(operations.OperationMetadataV1Beta1):
    disk_id = StringType(required=True)
    snapshot_id = StringType(required=True)


class CreateSnapshotOperation(SnapshotOperation):
    metadata = ModelType(CreateSnapshotMetadata)
