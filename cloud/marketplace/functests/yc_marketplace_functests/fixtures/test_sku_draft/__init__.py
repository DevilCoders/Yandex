from cloud.marketplace.functests.yc_marketplace_functests.fixtures.common_data import get_fixture as common_get_fixture


def get_fixture():
    return {
        "test_sku_draft_crud": common_get_fixture(),
        "test_ba_id_filtration": common_get_fixture(),
        "test_request_validation": common_get_fixture(),
        "test_reject": common_get_fixture(),
    }
