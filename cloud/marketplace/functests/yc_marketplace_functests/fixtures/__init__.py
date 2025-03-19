import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_build as test_build
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_category as test_category
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_health as test_health
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_isv as test_isv
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product as test_product
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family as test_product_family
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_product_family_version as \
    test_product_family_version
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher as test_publisher
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_sku_draft as test_sku_draft
import cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_var as test_var
from cloud.marketplace.functests.yc_marketplace_functests.fixtures import test_saas_product as test_saas_product
from cloud.marketplace.functests.yc_marketplace_functests.fixtures import test_simple_product as test_simple_product


def get_fixtures():
    return {
        "test_build": test_build.get_fixture(),
        "test_category": test_category.get_fixture(),
        "test_health": test_health.get_fixture(),
        "test_saas_product": test_saas_product.get_fixture(),
        "test_simple_product": test_simple_product.get_fixture(),
        "test_product": test_product.get_fixture(),
        "test_product_family": test_product_family.get_fixture(),
        "test_product_family_version": test_product_family_version.get_fixture(),
        "test_publisher": test_publisher.get_fixture(),
        "test_var": test_var.get_fixture(),
        "test_isv": test_isv.get_fixture(),
        "test_sku_draft": test_sku_draft.get_fixture(),
    }
