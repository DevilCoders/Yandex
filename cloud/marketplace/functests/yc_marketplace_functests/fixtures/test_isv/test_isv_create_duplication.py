from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def get_fixture():
    category1 = generate_id()
    category2 = generate_id()
    isv0 = generate_id()

    return {
        "category": [
            {
                "id": category1,
                "name": "category1",
                "parent_id": None,
                "type": Category.Type.ISV,
                "score": 1,
            },
            {
                "id": category2,
                "name": "category2",
                "parent_id": None,
                "type": Category.Type.ISV,
                "score": 1,
            },
        ],
        "isv": [
            {
                "billing_account_id": BA_ID,
                "contact_info": {
                    "address": None,
                    "phone": None,
                    "uri": "Test uri",
                },
                "created_at": 1548976445,
                "description": "",
                "id": isv0,
                "logo_id": generate_id(),
                "logo_uri": None,
                "meta": None,
                "name": "Test isv",
                "status": "active",
            },
        ],
        "ordering": [
            {
                "category_id": category1,
                "resource_id": isv0,
                "order": 1,
            }, {
                "category_id": category2,
                "resource_id": isv0,
                "order": 2,
            },
        ],
    }
