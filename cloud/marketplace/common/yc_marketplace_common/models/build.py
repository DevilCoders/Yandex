"""Marketplace blueprint model"""
from typing import Set

from schematics.types import ListType
from schematics.types import ModelType
from schematics.transforms import whitelist

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import MetadataOptions
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import _PUBLIC_API_ROLE
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class BuildStatus:
    NEW = "new"
    RUNNING = "running"
    SUCCEEDED = "succeeded"
    FAILED = "failed"

    ALL = {NEW, RUNNING, SUCCEEDED, FAILED}
    FINISHED = {SUCCEEDED, FAILED}


class Build(AbstractMktBase):
    """
    Build carries information about marketplace factory task runs

    """

    @property
    def PublicModel(self):
        return BuildResponse

    Filterable_fields = {
        "id",
        "blueprint_id",
        "mkt_task_id",
        "compute_image_id",
        "created_at",
        "updated_at",
        "blueprint_commit_hash",
        "status",
    }

    data_model = DataModel((
        Table(name="builds", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "blueprint_id": KikimrDataType.UTF8,
                "mkt_task_id": KikimrDataType.UTF8,
                "compute_image_id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "blueprint_commit_hash": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
            },
            primary_keys=["blueprint_id", "blueprint_commit_hash", "id"],
        )),
    ))

    id = ResourceIdType(required=True)
    blueprint_id = ResourceIdType(required=True)
    mkt_task_id = ResourceIdType()
    compute_image_id = StringType()
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    blueprint_commit_hash = StringType()
    status = StringEnumType(choices=BuildStatus.ALL, required=True)

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "blueprint_id",
                "mkt_task_id",
                "compute_image_id",
                "created_at",
                "updated_at",
                "blueprint_commit_hash",
                "status",
            ),
        }

    @classmethod
    def field_names(cls) -> Set[str]:
        return {key for key, _ in cls.fields.items()}

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key in cls.field_names())

    @classmethod
    def new(cls,
            build_id,
            blueprint_id,
            blueprint_commit_hash,
            mkt_task_id,
            ) -> "Build":

        return super().new(
            id=build_id if build_id else generate_id(),
            blueprint_id=blueprint_id,
            blueprint_commit_hash=blueprint_commit_hash,
            mkt_task_id=mkt_task_id,
            created_at=timestamp(),
            status=BuildStatus.NEW,
        )


class BuildResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    blueprint_id = ResourceIdType(required=True)
    mkt_task_id = ResourceIdType()
    compute_image_id = StringType()
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    blueprint_commit_hash = StringType()
    status = StringEnumType(choices=BuildStatus.ALL, required=True)

    Options = get_model_options(public_api_fields=(
        "id",
        "blueprint_id",
        "mkt_task_id",
        "compute_image_id",
        "created_at",
        "updated_at",
        "blueprint_commit_hash",
        "status",
    ))


class BuildUpdateRequest(MktBasePublicModel):
    build_id = ResourceIdType(required=True,
                              metadata={MetadataOptions.QUERY_VARIABLE: "build_id"})
    status = StringEnumType(choices=BuildStatus.ALL)
    compute_image_id = StringType()


class BuildMetadata(MktBasePublicModel):
    build_id = ResourceIdType(required=True)


class BuildOperation(OperationV1Beta1):
    metadata = ModelType(BuildMetadata, required=True)
    response = ModelType(BuildResponse)


class BuildList(MktBasePublicModel):
    next_page_token = StringType()
    builds = ListType(ModelType(BuildResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "builds"))


class BuildListingRequest(BaseMktManageListingRequest):
    pass


class BuildFinishRequest(MktBasePublicModel):
    build_id = ResourceIdType(required=True,
                              metadata={
                                  MetadataOptions.QUERY_VARIABLE: "build_id",
                              })
    compute_image_id = StringType()
    status = StringEnumType(choices=BuildStatus.FINISHED)
