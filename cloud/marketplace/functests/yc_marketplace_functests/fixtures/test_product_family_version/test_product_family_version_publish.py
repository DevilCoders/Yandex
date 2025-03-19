from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import get_fixture as common
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def get_fixture():
    version_id_1 = generate_id()
    version_id_2 = generate_id()
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
        "os_product_family": [
            {
                "created_at": timestamp(),
                "os_product_id": product_id,
                "labels": None,
                "description": "",
                "billing_account_id": BA_ID,
                "id": family_id,
                "status": OsProductFamily.Status.ACTIVE,
                "name": "product-family",
                "deprecation": None,
                "updated_at": timestamp(),
            },
        ],
        "os_product_family_version": [
            {
                "image_id": generate_id(),
                "updated_at": None,
                "logo_uri": "http://storage.example.com/{}".format(version_id_1),
                "os_product_family_id": family_id,
                "published_at": timestamp(),
                "created_at": timestamp(),
                "pricing_options": OsProductFamilyVersion.PricingOptions.FREE,
                "billing_account_id": BA_ID,
                "id": version_id_1,
                "status": OsProductFamilyVersion.Status.PENDING,
                "resource_spec": {
                    "cores": 4,
                    "disk_size": 32212254720,
                    "memory": 4194304,
                },
                "skus": [
                    {
                        "id": "d76a7lqhnocdib15b23s",
                        "check_formula": "some formula",
                    },
                ],
                "logo_id": generate_id(),
                "form_id": generate_id(),
            },
            {
                "image_id": generate_id(),
                "updated_at": None,
                "logo_uri": "http://storage.example.com/{}".format(version_id_2),
                "os_product_family_id": family_id,
                "published_at": timestamp(),
                "created_at": timestamp(),
                "pricing_options": OsProductFamilyVersion.PricingOptions.FREE,
                "billing_account_id": BA_ID,
                "id": version_id_2,
                "status": OsProductFamilyVersion.Status.REVIEW,
                "resource_spec": {
                    "cores": 4,
                    "disk_size": 32212254720,
                    "memory": 4194304,
                },
                "skus": [
                    {
                        "id": "d76a7lqhnocdib15b23s",
                        "check_formula": "some formula",
                    },
                ],
                "logo_id": generate_id(),
                "form_id": generate_id(),
            },
        ],
        **(common()),
    }
