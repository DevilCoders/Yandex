from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id


def get_fixture():
    parent_id = generate_id()
    return {
        "category": [
            {
                "id": parent_id,
                "name": "parent_category",
                "parent_id": None,
                "type": Category.Type.PUBLIC,
                "score": 1,
            },
            {
                "id": generate_id(),
                "parent_id": parent_id,
                "name": "child_1",
                "type": Category.Type.PUBLIC,
                "score": 1,
            },
            {
                "id": generate_id(),
                "parent_id": parent_id,
                "name": "child_2",
                "type": Category.Type.PUBLIC,
                "score": 1,
            },
        ],
    }
