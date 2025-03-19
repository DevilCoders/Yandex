from yc_common import config

from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID
from cloud.marketplace.functests.yc_marketplace_functests.utils import PUBLISHER_ACCOUNT_ID


def get_fixture():
    os_category = Category.new("all products", Category.Type.PUBLIC, 0, None)
    os_category.id = config.get_value("marketplace.default_os_product_category")

    saas_category = Category.new("saas products", Category.Type.PUBLIC, 0, None)
    saas_category.id = config.get_value("marketplace.default_saas_product_category")

    category = Category.new("simple products", Category.Type.PUBLIC, 0, None)
    category.id = config.get_value("marketplace.default_simple_product_category")

    res = [os_category.to_kikimr(), saas_category.to_kikimr(), category.to_kikimr()]
    categories = {
        "publisher": Category.Type.PUBLISHER,
        "var": Category.Type.VAR,
        "isv": Category.Type.ISV,
    }
    for partner_type in categories:
        category = Category.new("All {}s".format(partner_type), categories[partner_type], 0, None)
        category.id = config.get_value("marketplace.default_{}_category".format(partner_type))
        res.append(category.to_kikimr())

    return {
        "category": res,
    }


def default_publisher_fixtures():
    category = generate_id()
    publisher = generate_id()

    return {
        "category": [
            {
                "id": category,
                "name": "category",
                "parent_id": None,
                "type": Category.Type.PUBLIC,
                "score": 1,
            },

        ] + get_fixture()["category"],
        "publishers": [
            {
                "billing_account_id": BA_ID,
                "billing_publisher_account_id": PUBLISHER_ACCOUNT_ID,
                "contact_info": {
                    "address": None,
                    "phone": None,
                    "uri": "Test uri",
                },
                "created_at": 1548976445,
                "description": "",
                "id": publisher,
                "logo_id": None,
                "logo_uri": None,
                "meta": None,
                "name": "Test publisher",
                "status": "active",
            }
        ],
        "ordering": [
            {
                "category_id": category,
                "resource_id": publisher,
                "order": 1,
            }
        ],
    }


def default_invalid_publisher_fixtures():
    res = default_publisher_fixtures()
    res["publishers"][0]["status"] = "pending"
    return res
