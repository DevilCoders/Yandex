"""Marketplace product image model"""
from typing import Set

from schematics.transforms import whitelist
from schematics.types import IntType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import AbstractMktBase
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from yc_common import config
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import JsonSchemalessDictType
from yc_common.models import StringEnumType
from yc_common.models import StringType
from yc_common.models import _PUBLIC_API_ROLE
from yc_common.validation import ResourceIdType


def get_avatar_host():
    return config.get_value("endpoints.s3.url")


class AvatarCreateRequest(MktBasePublicModel):
    pass


class Avatar(AbstractMktBase):
    @property
    def PublicModel(self):
        return AvatarResponse

    class Status:
        NEW = "new"
        LINKED = "linked"
        ERROR = "error"

        ALL = {NEW, LINKED, ERROR}

    Filterable_fields = {
        "id",
        "created_at",
        "updated_at",
        "status",
        "linked_object",
    }

    data_model = DataModel((
        Table(name="avatars", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "created_at": KikimrDataType.UINT64,
                "updated_at": KikimrDataType.UINT64,
                "group_id": KikimrDataType.UINT64,
                "meta": KikimrDataType.JSON,
                "linked_object": KikimrDataType.UTF8,
                "status": KikimrDataType.UTF8,
            },
            primary_keys=["id"],
        )),
    ))

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "created_at",
                "updated_at",
                "status",
                "linked_object",
                "meta",
            ),
        }

    id = ResourceIdType(required=True)
    created_at = IsoTimestampType(required=True)
    updated_at = IsoTimestampType()
    status = StringEnumType(choices=Status.ALL, required=True)
    linked_object = StringType()
    group_id = IntType()
    meta = JsonSchemalessDictType()

    @classmethod
    def field_names(cls) -> Set[str]:
        return {key for key, _ in cls.fields.items()}

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key in cls.field_names())

    @classmethod
    def new(cls, **kwargs) -> "Avatar":
        return super().new(
            created_at=timestamp(),
            updated_at=timestamp(),
            status=cls.Status.NEW,
            **kwargs,
        )

    def to_public(self) -> "AvatarResponse":
        return AvatarResponse.new(id=self.id, uri=self.meta["url"])


class AvatarResponse(MktBasePublicModel):
    id = ResourceIdType(required=True)
    uri = StringType(required=True)

    class Options:
        roles = {
            _PUBLIC_API_ROLE: whitelist(
                "id",
                "uri",
            ),
        }
