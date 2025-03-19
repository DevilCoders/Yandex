from typing import Union

import grpc
from grpc._common import STATUS_CODE_TO_CYGRPC_STATUS_CODE
from schematics.types import DictType
from schematics.types import ListType
from schematics.types import ModelType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.resource_spec import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr.client import KikimrDataType
from yc_common.clients.kikimr.client import KikimrTableSpec
from yc_common.clients.models.base import BasePublicListingRequest
from yc_common.clients.models.operations import ErrorV1Alpha1
from yc_common.clients.models.operations import ErrorV1Beta1
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.exceptions import LogicalError
from yc_common.misc import timestamp
from yc_common.models import BooleanType
from yc_common.models import IntType
from yc_common.models import IsoTimestampType
from yc_common.models import JsonListType
from yc_common.models import JsonModelType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import MetadataOptions
from yc_common.models import Model
from yc_common.models import SchemalessDictType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType
from yc_common.validation import ResourceNameType


class TaskParams(MktBasePublicModel):
    is_infinite = BooleanType(default=False)
    is_cancelable = BooleanType()
    params = SchemalessDictType()

    Options = get_model_options(public_api_fields=(
        "is_infinite",
        "is_cancelable",
        "params",
    ))


class _TaskForCheckDeps(MktBasePublicModel):
    id = StringType(required=True)
    can_do = BooleanType(required=True)
    depends = ListType(ResourceIdType(), required=True)

    @classmethod
    def db_fields(cls):
        return "id, can_do, depends"


def map_error(err) -> ErrorV1Alpha1:
    type_ = err.get("code", "")
    message = err.get("message", "")
    return ErrorV1Alpha1.new(type=type_, message=message)


class TaskResolution(MktBasePublicModel):
    class Status:
        CANCELED = "canceled"
        NOTIMPLEMENTED = "not implemented"
        RESOLVED = "resolved"
        FAILED = "failed"

        ALL = {CANCELED, NOTIMPLEMENTED, RESOLVED, FAILED}

    result = SchemalessDictType()
    error = ModelType(ErrorV1Beta1)

    @classmethod
    def resolve(cls, status, data: Union[ErrorV1Beta1, dict, list, tuple, None] = None):
        class_fields = {}

        if status == cls.Status.RESOLVED:
            class_fields["result"] = data
        elif isinstance(data, ErrorV1Beta1):
            class_fields["error"] = data.to_primitive()
        else:
            code = STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.UNKNOWN]
            if status == cls.Status.CANCELED:
                code = STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.CANCELLED]
            elif status == cls.Status.NOTIMPLEMENTED:
                code = STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.UNIMPLEMENTED]
            elif status == cls.Status.FAILED:
                code = STATUS_CODE_TO_CYGRPC_STATUS_CODE[grpc.StatusCode.INTERNAL]

            details = []
            if data is not None:
                if not isinstance(data, (list, tuple)):
                    data = (data,)
                details = list(map(map_error, data))

            class_fields["error"] = ErrorV1Beta1({
                "code": code,
                "message": status,
                "details": details,
            }).to_primitive()

        return cls(class_fields)


class Task(AbstractMktBase):
    @property
    def PublicModel(self):
        return TaskResponse

    class Kind:
        INTERNAL = "internal"
        BUILD = "build"

        ALL = {INTERNAL, BUILD}

    id = StringType()
    description = StringType()
    created_at = IsoTimestampType()
    modified_at = IsoTimestampType()
    created_by = StringType()

    done = BooleanType()

    metadata = JsonSchemalessDictType()
    response = JsonSchemalessDictType()
    error = JsonModelType(ErrorV1Beta1)

    data_model = DataModel((
        Table(name="tasks", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "group_id": KikimrDataType.UTF8,
                "kind": KikimrDataType.UTF8,
                "operation_type": KikimrDataType.UTF8,
                "description": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "modified_at": KikimrDataType.UINT64,
                "created_by": KikimrDataType.UTF8,
                "done": KikimrDataType.BOOL,
                "metadata": KikimrDataType.JSON,
                "response": KikimrDataType.JSON,
                "error": KikimrDataType.JSON,
                "can_do": KikimrDataType.BOOL,
                "params": KikimrDataType.JSON,
                "lock": KikimrDataType.UTF8,
                "worker_hostname": KikimrDataType.UTF8,
                "unlock_after": KikimrDataType.UINT64,
                "execution_time_ms": KikimrDataType.UINT32,
                "try_count": KikimrDataType.UINT32,
                "depends": KikimrDataType.JSON,
            },
            primary_keys=["id"],
        )),
    ))

    Filterable_fields = {
        "id",
        "group_id",
        "description",
        "created_at",
        "created_by",
        "done",
        "operation_type",
        "unlock_after",
    }

    # Blacklisted from API
    group_id = ResourceIdType(required=True)
    kind = StringEnumType(required=True, choices=Kind.ALL)
    params = JsonModelType(TaskParams)
    operation_type = StringType(required=True)
    can_do = BooleanType(required=True)
    lock = StringType()
    worker_hostname = StringType()
    unlock_after = IsoTimestampType()
    execution_time_ms = IntType()
    try_count = IntType(required=True)
    depends = JsonListType(ResourceIdType(), default=[])

    @classmethod
    def new(cls, id=None, group_id=None, created_at=None, kind=None, **kwargs) -> "Task":
        depends = kwargs.get("depends") or []
        if depends != [] and group_id is None:
            raise LogicalError("group_id should be provided with dependent tasks")
        return super().new(
            id=id if id is not None else generate_id(),
            group_id=group_id if group_id is not None else generate_id(),
            kind=kind if kind is not None else Task.Kind.INTERNAL,
            try_count=0,
            unlock_after=0,
            created_at=created_at or timestamp(),
            **kwargs,
        )

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())

    def to_operation_v1beta1(self):
        return OperationV1Beta1.new(
            id=self.id,
            description=self.operation_type,
            created_at=self.created_at,
            modified_at=self.modified_at,
            created_by=self.created_by,
            done=self.done,
            error=self.error,
            metadata=self.metadata,
            response=self.response,
        )


class OperationList(MktBasePublicModel):
    next_page_token = StringType()
    operations = ListType(ModelType(OperationV1Beta1), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "operations"))


class TaskMetadata(MktBasePublicModel):
    task_id = ResourceIdType(required=True)


class TaskResponse(MktBasePublicModel):
    id = StringType()
    description = StringType()
    created_at = IsoTimestampType()
    modified_at = IsoTimestampType()
    created_by = StringType()

    done = BooleanType()

    metadata = JsonSchemalessDictType()
    response = JsonSchemalessDictType()
    error = JsonModelType(ErrorV1Beta1)

    group_id = ResourceIdType(required=True)
    kind = StringEnumType(required=True, choices=Task.Kind.ALL)
    params = JsonModelType(TaskParams)
    operation_type = StringType(required=True)
    can_do = BooleanType(required=True)
    lock = StringType()
    worker_hostname = StringType()
    unlock_after = IsoTimestampType()
    execution_time_ms = IntType()
    try_count = IntType(required=True)
    depends = JsonListType(ResourceIdType(), required=True)

    Options = get_model_options(public_api_fields=(
        "id",
        "description",
        "created_at",
        "modified_at",
        "created_by",
        "done",
        "metadata",
        "response",
        "error",
        "group_id",
        "kind",
        "params",
        "operation_type",
        "can_do",
        "lock",
        "worker_hostname",
        "unlock_after",
        "execution_time_ms",
        "try_count",
        "depends",
    ))


class TaskList(MktBasePublicModel):
    next_page_token = StringType()
    tasks = ListType(ModelType(Task), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "tasks"))


class SendMailToDocsParams(Model):
    # from_addr = StringType(required=True)
    publisher_id = ResourceIdType(required=True)


class StartCloneImageParams(Model):
    version_id = ResourceIdType(required=True)
    source_image_id = ResourceIdType(required=True)
    target_folder_id = ResourceIdType(required=True)
    resource_spec = JsonModelType(ResourceSpec)
    name = ResourceNameType()
    product_ids = ListType(ResourceIdType)


class StartCreateImageParams(MktBasePublicModel):
    class FieldTypes:
        DEFAULT = "image_id"
        ALL = ["disk_id", "snapshot_id", "uri", "image_id"]

    name = ResourceNameType(required=True)
    description = StringType(required=True)

    os_type = StringType(required=True)
    min_disk_size = IntType(required=True)
    requirements = DictType(StringType)

    labels = JsonSchemalessDictType()
    family = StringType()

    product_ids = ListType(ResourceIdType, default=[])

    source = StringType(required=True)
    field = StringType(choices=FieldTypes.ALL, default=FieldTypes.DEFAULT)

    version_id = ResourceIdType()
    target_folder_id = ResourceIdType(required=True)


class StartPublishVersionParams(Model):
    version_id = ResourceIdType(required=True)
    target_folder_id = ResourceIdType(required=True)


class UpdateImagePoolSizeParams(Model):
    pool_size = IntType(required=True)


class DeleteImagePoolParams(Model):
    image_id = ResourceIdType(required=True)


class FinalizeImageParams(Model):
    version_id = ResourceIdType(required=True)
    target_status = StringType(choices=OsProductFamilyVersion.Status.ALL, required=True)


class PublishLogoParams(Model):
    avatar_id = ResourceIdType(required=True)
    target_key = StringType(required=True)
    target_bucket = StringType(required=True)
    rewrite = BooleanType(default=False)


class PublishEulaParams(Model):
    eula_id = ResourceIdType(required=True)
    target_key = StringType(required=True)
    target_bucket = StringType(required=True)


class SaasProductCreateOrUpdateParams(Model):
    id = ResourceIdType(required=True)


class SimpleProductCreateOrUpdateParams(Model):
    id = ResourceIdType(required=True)


class OsProductCreateOrUpdateParams(Model):
    id = ResourceIdType(required=True)


class OsProductFamilyVersionCreateParams(Model):
    id = ResourceIdType(required=True)


class PublisherCreateOrUpdateParams(Model):
    id = ResourceIdType(required=True)


class BindSkusToVersionParams(Model):
    version_id = ResourceIdType(required=True)


class CreateSkuFromDraftParams(Model):
    sku_draft_id = ResourceIdType(required=True)


class StartCreateImageTaskCreationRequest(MktBasePublicModel):
    group_id = ResourceIdType()
    params = JsonModelType(StartCreateImageParams, required=True)
    depends = ListType(ResourceIdType, default=[])


class TaskListingRequest(BasePublicListingRequest):
    filter = StringType()
    order_by = StringType()


class TaskCreationRequest(MktBasePublicModel):
    type = StringType(required=True)
    group_id = ResourceIdType()
    params = JsonSchemalessDictType(required=True)
    is_infinite = BooleanType(default=False)
    depends = ListType(ResourceIdType, required=True)


# Need to pass some model to route handler
class TaskLockRequest(MktBasePublicModel):
    worker_hostname = StringType(required=True)


class TaskLockResponse(MktBasePublicModel):
    task = JsonModelType(TaskResponse)
    Options = get_model_options(public_api_fields=["task"])


class TaskFinishRequest(MktBasePublicModel):
    task_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "task_id"})
    lock = ResourceIdType(required=True)
    status = StringEnumType(required=True, choices=TaskResolution.Status.ALL)
    execution_time_ms = IntType(required=True)
    response = JsonSchemalessDictType()


class TaskExtendLockRequest(MktBasePublicModel):
    task_id = ResourceIdType(required=True, metadata={MetadataOptions.QUERY_VARIABLE: "task_id"})
    lock = ResourceIdType(required=True)
    interval = IntType(required=True, metadata={"label": "Add amount of seconds to task lock time"})
