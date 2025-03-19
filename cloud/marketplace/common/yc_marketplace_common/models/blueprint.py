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
from yc_common.models import JsonListType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import IsoTimestampType
from yc_common.models import MetadataOptions
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import _PUBLIC_API_ROLE
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class BlueprintStatus:
    NEW = "new"
    ACTIVE = "active"
    REJECTED = "rejected"

    ALL = {NEW, ACTIVE, REJECTED}
    PUBLIC = {ACTIVE}


class Blueprint(AbstractMktBase):
    """
    Blueprint holds information for building Compute Image with Packer
    and testing it with Goss


    """

    @property
    def PublicModel(self):
        return BlueprintResponse

    Filterable_fields = {
        "id",
        "name",
        "created_at",
        "updated_at",
        "publisher_account_id",
        "commit_hash",
        "status",
    }

    data_model = DataModel((
        Table(name="blueprints", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "name": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "publisher_account_id": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
                "build_recipe_links": KikimrDataType.JSON,
                "test_suites_links": KikimrDataType.JSON,
                "test_instance_config": KikimrDataType.JSON,
                "commit_hash": KikimrDataType.UTF8,
            },
            primary_keys=["id", "publisher_account_id"],
        )),
    ))

    id = ResourceIdType(required=True)
    name = StringType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    publisher_account_id = ResourceIdType(required=True)
    status = StringEnumType(choices=BlueprintStatus.ALL, required=True)

    build_recipe_links = JsonListType(StringType())
    test_suites_links = JsonListType(StringType())
    test_instance_config = JsonSchemalessDictType()
    commit_hash = StringType()

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "name",
                "created_at",
                "updated_at",
                "publisher_account_id",
                "status",
                "build_recipe_links",
                "test_suites_links",
                "test_instance_config",
                "commit_hash",
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


class BlueprintResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    name = StringType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    publisher_account_id = ResourceIdType(required=True)
    status = StringEnumType(choices=BlueprintStatus.ALL, required=True)

    build_recipe_links = JsonListType(StringType())
    test_suites_links = JsonListType(StringType())
    test_instance_config = JsonSchemalessDictType()
    commit_hash = StringType()

    Options = get_model_options(public_api_fields=(
        "id",
        "name",
        "created_at",
        "updated_at",
        "publisher_account_id",
        "status",
        "build_recipe_links",
        "test_suites_links",
        "test_instance_config",
        "commit_hash",
    ))


class BlueprintCreateRequest(MktBasePublicModel):
    name = StringType(required=True)
    publisher_account_id = ResourceIdType(required=True)
    build_recipe_links = JsonListType(StringType())
    test_suites_links = JsonListType(StringType())
    test_instance_config = JsonSchemalessDictType()
    commit_hash = StringType()

    def to_model(self):
        return Blueprint.new(
            id=generate_id(),
            name=self.name,
            publisher_account_id=self.publisher_account_id,
            created_at=timestamp(),
            updated_at=timestamp(),
            status=BlueprintStatus.NEW,
            build_recipe_links=self.build_recipe_links,
            test_suites_links=self.test_suites_links,
            test_instance_config=self.test_instance_config,
            commit_hash=self.commit_hash,
        )


class BlueprintUpdateRequest(MktBasePublicModel):
    blueprint_id = ResourceIdType(required=True,
                                  metadata={MetadataOptions.QUERY_VARIABLE: "blueprint_id"})
    name = StringType()
    build_recipe_links = JsonListType(StringType())
    test_suites_links = JsonListType(StringType())
    test_instance_config = JsonSchemalessDictType()
    commit_hash = StringType()


class BlueprintMetadata(MktBasePublicModel):
    blueprint_id = ResourceIdType(required=True)
    build_id = ResourceIdType()


class BlueprintOperation(OperationV1Beta1):
    metadata = ModelType(BlueprintMetadata, required=True)
    response = ModelType(BlueprintResponse)


class BlueprintList(MktBasePublicModel):
    next_page_token = StringType()
    blueprints = ListType(ModelType(BlueprintResponse), required=True, default=list)

    Options = get_model_options(public_api_fields=("next_page_token", "blueprints"))


class BlueprintListingRequest(BaseMktManageListingRequest):
    pass
