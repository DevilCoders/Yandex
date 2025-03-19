import pytest
from yc_common.ids import generate_id

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.billing.billing_account import BillingAccount
from cloud.marketplace.common.yc_marketplace_common.models.billing.billing_account import UsageStatus


@pytest.mark.parametrize(
    ["usage_status", "result"],
    [
        [UsageStatus.TRIAL, False],
        [UsageStatus.PAID, True],
    ],
)
def test_check_usage_permissions(monkeypatch, mocker, usage_status, result):
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.billing.get_instance_metadata_token")
    billing_account_mock = BillingAccount({"usage_status": usage_status})
    monkeypatch.setattr(lib.billing.BillingPrivateClient,
                        "resolve_billing_account_by_cloud",
                        lambda *a, **k: billing_account_mock)

    permission = lib.Billing.check_usage_permissions("testBillingEndpoint", generate_id("foo"))

    assert permission is result
