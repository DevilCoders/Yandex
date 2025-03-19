from yc_common.misc import timestamp

from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures as common
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def get_fixture():
    family_id = generate_id()
    product_id = generate_id()

    return {
        "os_product": [
            {
                "billing_account_id": BA_ID,
                "logo_id": None,
                "logo_uri": None,
                "name": "Product",
                "labels": {"foo": "bar"},
                "score": 0.0,
                "short_description": "",
                "primary_family_id": family_id,
                "description": "Some test description",
                "status": OsProduct.Status.ACTIVE,
                "created_at": timestamp(),
                "id": product_id,
            },
        ],
        "os_product_family": [],
        "os_product_family_version": [],
        **(common()),
    }
