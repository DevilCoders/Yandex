from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import get_fixture as common
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def version(status, family_id):
    id = generate_id()
    return {
        "image_id": generate_id(),
        "updated_at": None,
        "logo_uri": "http://storage.example.com/{}".format(id),
        "os_product_family_id": family_id,
        "published_at": timestamp(),
        "created_at": timestamp(),
        "pricing_options": OsProductFamilyVersion.PricingOptions.FREE,
        "billing_account_id": BA_ID,
        "id": id,
        "status": status,
        "resource_spec": {
            "cores": 4,
            "disk_size": 32212254720,
            "memory": 4194304,
            "billing_account_requirements": {
                "usage_status": ["paid"],
            }
        },
        "skus": [
            {
                "id": "d76a7lqhnocdib15b23s",
                "check_formula": "some formula",
            },
        ],
        "logo_id": generate_id(),
        "form_id": generate_id(),
    }


def get_fixtures():
    family_id = generate_id()
    statuses = [
        OsProductFamilyVersion.Status.ACTIVE,
        OsProductFamilyVersion.Status.DEPRECATED,
        OsProductFamilyVersion.Status.PENDING,
    ]

    return {
        "os_product_family_version": [version(s, family_id) for s in statuses],
        **(common()),
    }
