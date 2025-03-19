from schematics.types import IntType

from yc_common.clients.kikimr.client import KikimrDataType
from yc_common.clients.kikimr.client import KikimrTableSpec
from yc_common.models import ListType
from yc_common.models import Model
from yc_common.models import ModelType
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table


class Ordering(Model):
    category_id = ResourceIdType(required=True)
    resource_id = ResourceIdType(required=True)
    order = IntType(default=0)

    data_model = DataModel((
        Table(name="ordering", spec=KikimrTableSpec(
            columns={
                "category_id": KikimrDataType.UTF8,
                "resource_id": KikimrDataType.UTF8,
                "order": KikimrDataType.UINT32,
            },
            primary_keys=["category_id", "resource_id"],
        )),
    ))

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())


class OrderingShortView(Model):
    resource_id = ResourceIdType(required=True)
    order = IntType(default=0)

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())


class OrderingList(Model):
    next_page_token = ResourceIdType(required=True)
    resource_orders = ListType(ModelType(OrderingShortView))
