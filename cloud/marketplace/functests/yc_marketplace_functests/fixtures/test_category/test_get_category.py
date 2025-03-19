from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id


def get_fixture():
    return {
        "category": [{
            "id": generate_id(),
            "name": "category",
            "type": Category.Type.PUBLIC,
            "score": 1,
        }],
    }
