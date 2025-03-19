from yc_common.clients.billing import CreateSkuRequest
from yc_common.misc import timestamp

from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.models.billing.billing_account import UsageStatus
from cloud.marketplace.common.yc_marketplace_common.models.billing.pricing import PricingVersion
from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import PublisherAccountGetRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueReportRequest
from cloud.marketplace.common.yc_marketplace_common.utils.metadata_token import get_instance_metadata_token


class Billing:
    @staticmethod
    def list_person():
        pass

    @staticmethod
    def add_person():
        pass

    @staticmethod
    def list_contract():
        pass

    @staticmethod
    def add_contract():
        pass

    @staticmethod
    def check_usage_permissions(endpoint, cloud_id):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        billing_account = billing_client.resolve_billing_account_by_cloud(cloud_id, timestamp())
        return billing_account.usage_status != UsageStatus.TRIAL

    @staticmethod
    def get_sku(endpoint, sku_id):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        sku = billing_client.get_sku(sku_id)
        return sku

    @staticmethod
    def list_skus(endpoint, filter_query):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        sku_list = billing_client.list_skus(filter_query)
        return sku_list

    @staticmethod
    def create_sku(endpoint, request: CreateSkuRequest):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        sku = billing_client.create_sku(request)
        return sku

    @staticmethod
    def delete_sku(endpoint, sku_id):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        billing_client.delete_sku(sku_id)

    @staticmethod
    def add_pricing_version(endpoint, sku_id, request: PricingVersion):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        sku = billing_client.add_pricing_version(sku_id, request)
        return sku

    @staticmethod
    def get_revenue_reports_meta(endpoint, publisher_account_id):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        return billing_client.get_revenue_reports_meta(publisher_account_id)

    @staticmethod
    def get_revenue_reports(endpoint, publisher_account_id, request: RevenueReportRequest):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        return billing_client.get_revenue_reports(publisher_account_id, request)

    @staticmethod
    def get_publisher(endpoint, publisher_account_id):
        iam_token = get_instance_metadata_token()
        billing_client = BillingPrivateClient(endpoint, iam_token=iam_token)
        return billing_client.get_publisher_account(publisher_account_id, PublisherAccountGetRequest({
            "view": PublisherAccountGetRequest.ViewType.FULL
        }))
