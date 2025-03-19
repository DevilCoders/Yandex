from schematics.types import StringType

from cloud.marketplace.common.yc_marketplace_common.models.abstract_mkt_base import MktBasePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from cloud.marketplace.common.yc_marketplace_common.utils.db import DataModel
from cloud.marketplace.common.yc_marketplace_common.utils.db import Table
from yc_common.clients.kikimr import KikimrDataType
from yc_common.clients.kikimr import KikimrTableSpec
from yc_common.misc import timestamp
from yc_common.models import IsoTimestampType
from yc_common.models import Model
from yc_common.models import StringEnumType
from yc_common.models import get_model_options
from yc_common.validation import ResourceIdType


class ProductSlugAddRequest(MktBasePublicModel):
    slug = StringType(required=True)
    product_id = ResourceIdType(required=True)
    product_type = StringEnumType(required=True, choices=ProductType.ALL)


class ProductSlugRemoveRequest(MktBasePublicModel):
    slug = StringType(required=True)


class ProductSlugResponse(MktBasePublicModel):
    slug = StringType()
    Options = get_model_options(public_api_fields=("slug",))


class ProductSlugScheme(Model):
    slug = StringType(required=True)
    product_id = ResourceIdType(required=True)
    product_type = StringType(required=True)
    created_at = IsoTimestampType(required=True)
    data_model = DataModel(
        [Table(
            name="product_slug",
            spec=KikimrTableSpec(
                columns={
                    "slug": KikimrDataType.UTF8,
                    "product_id": KikimrDataType.UTF8,
                    "created_at": KikimrDataType.UINT64,
                    "product_type": KikimrDataType.UTF8,
                },
                primary_keys=["slug"],
            ),
        )],
    )

    @classmethod
    def db_fields(cls, table_name="") -> str:
        if table_name:
            table_name += "."
        return ",".join(map(lambda x: table_name + x, ("slug", "product_id", "created_at", "product_type")))

    @classmethod
    def from_create_request(cls, slug, product_id, product_type):
        return cls.new(
            slug=slug,
            product_id=product_id,
            created_at=timestamp(),
            product_type=product_type,
        )
