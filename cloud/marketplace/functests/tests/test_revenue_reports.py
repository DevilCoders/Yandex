import mock

from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueReportRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherRevenueReportRequest


def test_get_revenue_reports_meta(mocker, marketplace_client, db_fixture):
    publisher_id = db_fixture["publishers"][0]["id"]
    publisher = marketplace_client.get_publisher(publisher_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.billing.service_token")

    with mock.patch("yc_common.clients.billing.BillingPrivateClient") as mocked_billing_client:
        marketplace_client.get_revenue_reports_meta(publisher_id)
        mocked_billing_client.get_revenue_reports_meta.assert_called_with(publisher.billing_publisher_account_id)


def test_get_revenue_reports(mocker, marketplace_client, db_fixture):
    publisher_id = db_fixture["publishers"][0]["id"]
    publisher = marketplace_client.get_publisher(publisher_id)
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.lib.billing.service_token")
    request = PublisherRevenueReportRequest.new(
        publisher_id=publisher_id,
        start_date="2019-01-01",
        end_date="2019-01-02"
    )
    with mock.patch("yc_common.clients.billing.BillingPrivateClient") as mocked_billing_client:
        marketplace_client.get_revenue_reports(publisher_id, request)
        expected_request = RevenueReportRequest.new(
            publisher_account_id=publisher.billing_publisher_account_id,
            start_date=request.start_date,
            end_date=request.end_date,
            sku_ids=request.sku_ids
        )

        mocked_billing_client.get_revenue_reports.assert_called_with(publisher.billing_publisher_account_id,
                                                                     expected_request)
