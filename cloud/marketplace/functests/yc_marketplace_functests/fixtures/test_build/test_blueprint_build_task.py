from yc_common.misc import timestamp

from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintStatus
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures
from cloud.marketplace.functests.yc_marketplace_functests.utils import PUBLISHER_ACCOUNT_ID


def get_fixture():
    blueprint_id = generate_id()

    return {
        "blueprints": [
            {
                "id": blueprint_id,
                "name": "testBlueprint",
                "publisher_account_id": PUBLISHER_ACCOUNT_ID,
                "created_at": timestamp(),
                "updated_at": timestamp(),
                "status": BlueprintStatus.ACTIVE,
                "build_recipe_links": ["s3://blueprints/test.zip", ],
                "test_suites_links": ["https://storage.yandexcloud.net/yc-marketplace-image-tests/generic-linux.zip",
                                      "https://storage.yandexcloud.net/yc-marketplace-image-tests/second-test.zip"],
                "test_instance_config": {
                    "cpus": 1,
                    "memory": 2147483648,
                },
                "commit_hash": "abchash",
            }
        ],
        **(default_publisher_fixtures()),
    }
