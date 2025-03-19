from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.utils import BA_ID


def get_fixture():
    category0 = generate_id()
    category1 = generate_id()
    category2 = generate_id()
    var0 = generate_id()
    var1 = generate_id()

    return {
        "category": [
            {
                "id": category0,
                "name": "category0",
                "parent_id": None,
                "type": Category.Type.PUBLIC,
                "score": 1,
            },
            {
                "id": category1,
                "name": "category1",
                "parent_id": None,
                "type": Category.Type.VAR,
                "score": 1,
            },
            {
                "id": category2,
                "name": "category2",
                "parent_id": None,
                "type": Category.Type.VAR,
                "score": 1,
            },
            {
                "id": generate_id(),
                "name": "category3",
                "parent_id": None,
                "type": Category.Type.VAR,
                "score": 1,
            },
        ],
        "var": [
            {
                "billing_account_id": BA_ID,
                "contact_info": {
                    "address": None,
                    "phone": None,
                    "uri": "Test uri",
                },
                "created_at": 1548976445,
                "description": "",
                "id": var0,
                "logo_id": generate_id(),
                "logo_uri": None,
                "meta": None,
                "name": "Test var",
                "status": "active",
            },
            {
                "billing_account_id": BA_ID,
                "contact_info": {
                    "address": None,
                    "phone": None,
                    "uri": "Test uri",
                },
                "created_at": 1548976445,
                "description": "",
                "id": var1,
                "logo_id": generate_id(),
                "logo_uri": None,
                "meta": None,
                "name": "Test var",
                "status": "pending",
            },
        ],
        "ordering": [
            {
                "category_id": category1,
                "resource_id": var0,
                "order": 1,
            }, {
                "category_id": category2,
                "resource_id": var0,
                "order": 2,
            }, {
                "category_id": category2,
                "resource_id": var1,
                "order": 2,
            },
        ],
    }
