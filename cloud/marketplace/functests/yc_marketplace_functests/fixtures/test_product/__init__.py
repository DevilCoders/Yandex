from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_invalid_publisher_fixtures


def get_fixture():
    return {
        "test_product_ordering": default_publisher_fixtures(),
        "test_product_update_does_not_affect_ordering": default_publisher_fixtures(),
        "test_get_non_public_product": default_publisher_fixtures(),
        "test_get_product_error": default_publisher_fixtures(),
        "test_get_product_slug": default_publisher_fixtures(),
        "test_get_product_slug_update": default_publisher_fixtures(),
        "test_get_public_product": default_publisher_fixtures(),
        "test_own_product_list": default_publisher_fixtures(),
        "test_product_create": default_publisher_fixtures(),
        "test_product_create_invalid_publisher": default_invalid_publisher_fixtures(),
        "test_product_listing_by_non_unique_field": default_publisher_fixtures(),
        "test_product_update": default_publisher_fixtures(),
        "test_public_product_list": default_publisher_fixtures(),
        "test_public_product_list_families_alphabetically": default_publisher_fixtures(),
        "test_public_product_list_families_natsort": default_publisher_fixtures(),
        "test_public_product_batch": default_publisher_fixtures(),
        "test_put_product_on_place_in_category": default_publisher_fixtures(),
    }
