from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_category import test_default_category_list
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_category import test_get_category
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_category import test_list_category


def get_fixture():
    return {
        "test_get_category": test_get_category.get_fixture(),
        "test_list_category": test_list_category.get_fixture(),
        "test_default_category_list": test_default_category_list.get_fixture(),
    }
