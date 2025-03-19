from schematics import types as schematics_types

from yc_common import exceptions
from yc_common import models as common_models
from yc_common.clients.models import base as base_models


class ErrorV1Alpha1(base_models.BasePublicModel):
    type = schematics_types.StringType(required=True)
    message = schematics_types.StringType(required=True)
    code = schematics_types.IntType(default=int(exceptions.GrpcStatus.INTERNAL))  # FIXME: Mark field as required after transition period


class OperationV1Alpha1(base_models.BasePublicObjectModelV1Alpha1):
    class Status:
        PENDING = "pending"
        RUNNING = "running"
        FAILED = "failed"
        DONE = "done"
        CANCELLING = "cancelling"
        CANCELLED = "cancelled"

        ALL = [PENDING, RUNNING, FAILED, DONE, CANCELLING, CANCELLED]
        ALL_COMPLETED = [DONE, FAILED, CANCELLED]
        ALL_READY_FOR_EXECUTION = [RUNNING, CANCELLING]

    TYPE_NAME = "operation"

    created_by = schematics_types.StringType(required=True)
    operation_type = schematics_types.StringType(required=True)
    target_id = schematics_types.StringType(required=False)

    started_at = common_models.IsoTimestampType(required=True)
    modified_at = common_models.IsoTimestampType(required=True)

    status = common_models.StringEnumType(required=True)
    progress = schematics_types.IntType(required=False)
    status_message = schematics_types.StringType(required=False)
    errors = schematics_types.ListType(schematics_types.ModelType(ErrorV1Alpha1), required=False)

    @schematics_types.serializable
    def done(self):
        return self.status in self.Status.ALL_COMPLETED

    def get_error(self):
        if self.status != self.Status.FAILED:
            return None

        if not self.errors:
            return {
                "code": int(exceptions.GrpcStatus.INTERNAL),
                "type": "UnknownError",
                "message": "Operation failed, but no error details"
            }
        else:
            return self.errors[-1]


class ErrorV1Beta1(base_models.BasePublicModel):
    """
    Model for google.rpc.Status

    See https://github.com/grpc/grpc/blob/master/src/proto/grpc/status/status.proto
    """

    code = schematics_types.IntType()
    message = schematics_types.StringType()
    details = schematics_types.ListType(schematics_types.ModelType(ErrorV1Alpha1))


class OperationV1Beta1(base_models.BasePublicModel):
    id = schematics_types.StringType()
    description = schematics_types.StringType()
    created_at = common_models.IsoTimestampType()
    modified_at = common_models.IsoTimestampType()
    created_by = schematics_types.StringType()

    done = schematics_types.BooleanType()

    metadata = common_models.SchemalessDictType()
    response = common_models.SchemalessDictType()
    error = schematics_types.ModelType(ErrorV1Beta1)

    def get_error(self):
        if self.error is None:
            return None
        elif self.error.code == int(exceptions.GrpcStatus.CANCELLED):
            # FIXME: Move error constants out from compute client to avoid import cycle
            return {
                "code": self.error.code,
                "type": "OperationCancelled",
                "message": "Operation has been cancelled"
            }
        else:
            return self.error.details[-1]


class OperationMetadataV1Beta1(base_models.BasePublicModel):
    type = schematics_types.StringType()


class OperationListV1Beta1(base_models.BaseListModel):
    operations = schematics_types.ListType(schematics_types.ModelType(OperationV1Beta1), required=True, default=list)
