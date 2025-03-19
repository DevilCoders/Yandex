from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import default_publisher_fixtures
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family.test_product_family_update_inherit_logo import \
    get_fixture as test_product_family_update_inherit_logo
from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family\
    .test_product_family_related_saas_product import get_fixture as test_product_family_related_saas_product


def get_fixture():
    return {
        "test_product_family_create": default_publisher_fixtures(),
        "test_product_family_update": default_publisher_fixtures(),
        "test_product_family_update_inherit_logo": test_product_family_update_inherit_logo(),
        "test_product_family_update_desc_only": default_publisher_fixtures(),
        "test_product_family_deprecate": default_publisher_fixtures(),
        "test_own_product_family_list": default_publisher_fixtures(),
        "test_public_product_family_list": default_publisher_fixtures(),
        "test_product_family_list_filtered": default_publisher_fixtures(),
        "test_deprecation_flow": default_publisher_fixtures(),
        "test_create_licence_rules": default_publisher_fixtures(),
        "test_update_licence_rules": test_product_family_update_inherit_logo(),
        "test_product_family_version_create_with_sku": default_publisher_fixtures(),
        "test_product_family_related_saas_product": test_product_family_related_saas_product(),
        "test_product_family_related_saas_product_update": test_product_family_related_saas_product(),
        "test_product_family_related_saas_product_update_error": test_product_family_related_saas_product(),
    }
