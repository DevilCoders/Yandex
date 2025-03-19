from yc_common.misc import timestamp

from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def get_fixture():
    version_id = generate_id()
    family_id = generate_id()
    product_id = generate_id()
    avatar_id = generate_id()
    OTHER_BA_ID = generate_id()

    return {
        "avatars": [
            {
                "id": avatar_id,
                "created_at": timestamp(),
                "group_id": None,
                "linked_object": version_id,
                "meta": {
                    "url": "https://storage.cloud-preprod.yandex.net/yc-marketplace-default-images/dqn01hg0afq6hbehcc5h.svg",
                    "key": "dqn01hg0afq6hbehcc5h",
                    "bucket": "yc-marketplace-default-images",
                    "ext": "svg"
                },
                "status": "linked",
                "updated_at": timestamp(),
            }
        ],
        "saas_product": [
            {
                "billing_account_id": BA_ID,
                "logo_id": None,
                "logo_uri": None,
                "name": "Saas",
                "labels": {"foo": "bar"},
                "score": 0.0,
                "short_description": "",
                "description": "Some test description",
                "status": OsProduct.Status.ACTIVE,
                "created_at": timestamp(),
                "id": generate_id(),
            },
            {
                "billing_account_id": BA_ID,
                "logo_id": None,
                "logo_uri": None,
                "name": "Saas",
                "labels": {"foo": "bar"},
                "score": 0.0,
                "short_description": "",
                "description": "Some test description",
                "status": OsProduct.Status.ACTIVE,
                "created_at": timestamp(),
                "id": generate_id(),
            },
            {
                "billing_account_id": OTHER_BA_ID,
                "logo_id": None,
                "logo_uri": None,
                "name": "Saas",
                "labels": {"foo": "bar"},
                "score": 0.0,
                "short_description": "",
                "description": "Some test description",
                "status": OsProduct.Status.ACTIVE,
                "created_at": timestamp(),
                "id": generate_id(),
            },
        ],
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
                "logo_uri": "http://storage.example.com/{}".format(version_id),
                "os_product_family_id": family_id,
                "published_at": timestamp(),
                "created_at": timestamp(),
                "pricing_options": OsProductFamilyVersion.PricingOptions.FREE,
                "billing_account_id": BA_ID,
                "id": version_id,
                "status": OsProductFamilyVersion.Status.ACTIVE,
                "resource_spec": {
                    "cores": 4,
                    "disk_size": 32212254720,
                    "memory": 4194304,
                },
                "skus": [],
                "logo_id": avatar_id,
                "form_id": generate_id(),
            },
        ],
        **(default_publisher_fixtures()),
    }
