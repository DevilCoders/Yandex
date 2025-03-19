from cloud.marketplace.functests.yc_marketplace_functests.fixtures.test_build.test_blueprint_build_task import \
    get_fixture as test_blueprint_build_task


def get_fixture():
    return {
        "test_blueprint_build_task_success": test_blueprint_build_task(),
        "test_blueprint_build_task_failed": test_blueprint_build_task(),
    }
