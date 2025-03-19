from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family_version import \
    test_product_family_version_create_with_sku
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family_version import \
    test_product_family_version_list
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family_version import \
    test_product_family_version_publish
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family_version import test_version_get
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures


def get_fixture():
    return {
        "test_product_create": default_publisher_fixtures(),
        "test_get_non_public_product": default_publisher_fixtures(),
        "test_get_product_slug": default_publisher_fixtures(),
        "test_get_product_slug_update": default_publisher_fixtures(),
        "test_get_public_product": default_publisher_fixtures(),
        "test_own_product_list": default_publisher_fixtures(),
        "test_public_product_list": default_publisher_fixtures(),
        "test_product_update": default_publisher_fixtures(),
        "test_product_listing_by_non_unique_field": default_publisher_fixtures(),
        "test_put_product_on_place_in_category": default_publisher_fixtures(),
        "test_product_update_does_not_affect_ordering": default_publisher_fixtures(),
        "test_product_family_version_list": test_product_family_version_list.get_fixture(),
        "test_product_family_version_publish": test_product_family_version_publish.get_fixture(),
        "test_version_get": test_version_get.get_fixtures(),
        "test_product_family_version_create_with_sku": test_product_family_version_create_with_sku.get_fixture(),
    }
