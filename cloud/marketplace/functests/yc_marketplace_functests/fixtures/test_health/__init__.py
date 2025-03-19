from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_health import test_health_monrun


def get_fixture():
    return {
        "test_health_monrun": test_health_monrun.get_fixture(),
    }
