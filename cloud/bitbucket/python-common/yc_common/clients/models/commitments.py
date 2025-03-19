from yc_common import models as common_models

from . import base as base_models
from . import operations


class Commitment(base_models.BasePublicObjectModel):
    class Status:
        CREATING = "creating"
        DELETING = "deleting"
        READY = "ready"

        ALL = [CREATING, READY]

    cloud_id = common_models.StringType(required=True)
    description = common_models.StringType()

    started_at = common_models.IsoTimestampType()
    ended_at = common_models.IsoTimestampType()
    status = common_models.StringEnumType(required=True)

    platform_id = common_models.StringType(required=True)
    cores = common_models.IntType(required=True)
    memory = common_models.IntType(required=True)


class CommitmentList(base_models.BaseListModel):
    commitments = common_models.ListType(common_models.ModelType(Commitment), required=True, default=list)


class CommitmentMetadata(operations.OperationMetadataV1Beta1):
    commitment_id = common_models.StringType(required=True)


class CommitmentOperation(operations.OperationV1Beta1):
    metadata = common_models.ModelType(CommitmentMetadata)
    response = common_models.ModelType(Commitment)


class CommitmentOperationList(base_models.BaseListModel):
    operations = common_models.ListType(common_models.ModelType(CommitmentOperation), required=True, default=list)
