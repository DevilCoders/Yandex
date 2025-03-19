from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_var import test_var_create
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_var import test_var_create_duplication
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_var import test_var_get
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_var import test_var_listing


def get_fixture():
    return {
        "test_var_get": test_var_get.get_fixture(),
        "test_var_listing": test_var_listing.get_fixture(),
        "test_var_create": test_var_create.get_fixture(),
        "test_var_create_duplication": test_var_create_duplication.get_fixture(),

    }
