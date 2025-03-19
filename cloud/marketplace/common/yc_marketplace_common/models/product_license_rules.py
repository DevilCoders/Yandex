from schematics.types import ListType
from schematics.types import ModelType
from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr.client import KikimrDataType
from yc_common.clients.kikimr.client import KikimrTableSpec
from yc_common.fields import JsonListType
from yc_common.models import Model
from yc_common.validation import ResourceIdType


class ProductLicenseRule(Model):
    class Rule(Model):
        """Represents a product licensing rule"""

        category = StringType(required=True)
        entity = StringType(required=True)
        path = StringType(required=True)
        expected = ListType(StringType, required=True)

    id = ResourceIdType(required=True)
    rules = JsonListType(ModelType(Rule), required=True)

    data_model = DataModel((
        Table(name="product_license_rules", spec=KikimrTableSpec(
            columns={
                "id": KikimrDataType.UTF8,
                "rules": KikimrDataType.JSON,
            },
            primary_keys=["id"],
        )),
    ))

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."

        return ",".join("{}{}".format(table_name, key) for key, _ in cls.fields.items())
