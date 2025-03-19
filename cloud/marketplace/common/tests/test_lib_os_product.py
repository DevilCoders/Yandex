import pytest

from yc_common.ids import generate_id
from cloud.marketplace.common.yc_marketplace_common import lib


@pytest.mark.parametrize(
    ["all_free", "check_usage_permissions_result", "result"],
    [
        [True, None, True],
        [True, True, True],
        [False, False, False],
    ],
)
def test_rpc_check_usage_permissions(monkeypatch, all_free, check_usage_permissions_result, result):
    monkeypatch.setattr(lib.OsProductFamilyVersion, "rpc_check_all_free", lambda *a, **k: all_free)
    monkeypatch.setattr(lib.Billing,
                        "check_usage_permissions",
                        lambda *a, **k: check_usage_permissions_result)

    permission = lib.OsProduct.rpc_check_usage_permissions("testBillingEndpoint", generate_id("foo"),
                                                           [generate_id("foo")])

    assert permission.get("permission") is result
