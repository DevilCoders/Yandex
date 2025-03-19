from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.models import Model
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table


class ProductToSkuBindingScheme(Model):
    product_id = ResourceIdType(required=True)
    sku_id = ResourceIdType(required=True)
    publisher_id = ResourceIdType(required=True)
    data_model = DataModel(
        [Table(
            name="product_to_sku_binding",
            spec=KikimrTableSpec(
                columns={
                    "product_id": KikimrDataType.UTF8,
                    "sku_id": KikimrDataType.UTF8,
                    "publisher_id": KikimrDataType.UTF8,
                },
                primary_keys=["product_id", "sku_id"],
            ),
        )],
    )

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."
        return ",".join(map(lambda x: table_name + x, ("product_id", "sku_id", "publisher_id")))
