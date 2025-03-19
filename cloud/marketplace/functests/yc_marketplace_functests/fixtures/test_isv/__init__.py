from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_isv import test_isv_create
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_isv import test_isv_create_duplication
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_isv import test_isv_get
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_isv import test_isv_listing


def get_fixture():
    return {
        "test_isv_get": test_isv_get.get_fixture(),
        "test_isv_listing": test_isv_listing.get_fixture(),
        "test_isv_create": test_isv_create.get_fixture(),
        "test_isv_create_duplication": test_isv_create_duplication.get_fixture(),
    }
