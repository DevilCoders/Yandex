from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher import test_publisher_create
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher import test_publisher_create_duplication
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher import test_publisher_get
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher import test_publisher_listing


def get_fixture():
    return {
        "test_publisher_get": test_publisher_get.get_fixture(),
        "test_publisher_listing": test_publisher_listing.get_fixture(),
        "test_publisher_create": test_publisher_create.get_fixture(),
        "test_publisher_create_duplication": test_publisher_create_duplication.get_fixture(),
    }
