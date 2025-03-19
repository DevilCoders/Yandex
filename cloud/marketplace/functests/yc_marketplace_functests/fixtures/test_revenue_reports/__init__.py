from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_publisher import test_publisher_get


def get_fixture():
    return {
        "test_get_revenue_reports_meta": test_publisher_get.get_fixture(),
        "test_get_revenue_reports": test_publisher_get.get_fixture(),
    }
