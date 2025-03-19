"""Billing service client"""
from typing import List

from yc_common import config
from yc_common.clients.api import ApiClient
from yc_common.clients.models import base
from yc_common.clients.models.billing import BillingOperation
from yc_common.clients.models.billing.billing_account import BillingAccount
from yc_common.clients.models.billing.billing_account import BillingAccountFeatureRequest
from yc_common.clients.models.billing.billing_account import BillingAccountFullView
from yc_common.clients.models.billing.cloud import BindCloudAccountRequest
from yc_common.clients.models.billing.cloud import CloudBatchRequest
from yc_common.clients.models.billing.cloud import CloudBatchResponseEntry
from yc_common.clients.models.billing.grant_policy import GrantPolicyCreateRequest
from yc_common.clients.models.billing.grant_policy import GrantPolicyList
from yc_common.clients.models.billing.grant_policy import GrantPolicyPublicView
from yc_common.clients.models.billing.monetary_grant_offer import BindMonetaryGrantOfferRequest
from yc_common.clients.models.billing.monetary_grant_offer import MonetaryGrantOfferPublicView
from yc_common.clients.models.billing.person import PrivatePersonsList
from yc_common.clients.models.billing.pricing import PricingVersion
from yc_common.clients.models.billing.publisher_account import PublisherAccount
from yc_common.clients.models.billing.publisher_account import PublisherAccountCreateRequest
from yc_common.clients.models.billing.publisher_account import PublisherAccountGetRequest
from yc_common.clients.models.billing.publisher_account import PublisherAccountUpdateRequest
from yc_common.clients.models.billing.revenue_report import RevenueAggregateReport
from yc_common.clients.models.billing.revenue_report import RevenueMetaResponse
from yc_common.clients.models.billing.revenue_report import RevenueReportRequest
from yc_common.clients.models.billing.sku import CreateSkuRequest
from yc_common.clients.models.billing.sku import SkuLinkView
from yc_common.clients.models.billing.sku import SkuList
from yc_common.clients.models.billing.sku import SkuProductLinkRequest
from yc_common.clients.models.billing.sku import SkuPublicView
from yc_common.misc import drop_none
from yc_common.models import ListType
from yc_common.models import Model
from yc_common.models import ModelType


class BillingPrivateClient:
    def __init__(self, billing_url, api_version="v1", iam_token=None):
        api_url = "{billing_url}/billing/{api_version}/private".format(billing_url=billing_url,
                                                                       api_version=api_version)

        self.__client = ApiClient(api_url, iam_token=iam_token)

    def get_operation(self, operation_id: str) -> BillingOperation:
        return self.__client.get("/operations/{}".format(operation_id), model=BillingOperation)

    def batch_resolve_clouds(self, request: CloudBatchRequest):
        class Result(Model):
            result = ListType(ModelType(CloudBatchResponseEntry), required=True)

        data = request.to_primitive()
        return self.__client.post("/clouds/batchResolve", data, model=Result).result

    def resolve_billing_account_by_cloud(self, cloud_id, timestamp: int) -> BillingAccount:
        path = "/clouds/{}/resolveBillingAccount".format(cloud_id)
        return self.__client.get(path, params={
            "effective_time": timestamp
        })

    def get_monetary_grant_offer(self, monetary_grant_offer_id: str) -> MonetaryGrantOfferPublicView:
        path = "/monetaryGrantOffers/{}".format(monetary_grant_offer_id)

        return self.__client.get(path, model=MonetaryGrantOfferPublicView)

    def list_monetary_grant_offers(self):
        class Result(Model):
            result = ListType(ModelType(MonetaryGrantOfferPublicView), required=True)

        path = "/monetaryGrantOffers"

        return self.__client.get(path, model=Result).result

    def bind_monetary_grant_offer(self, monetary_grant_offer_id: str,
                                  request: BindMonetaryGrantOfferRequest) -> BillingOperation:
        path = "/monetaryGrantOffers/{}:bind".format(monetary_grant_offer_id)

        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def link_sku(self, sku_id, request: SkuProductLinkRequest) -> SkuLinkView:
        path = "/skus/{}/links".format(sku_id)

        return self.__client.post(path, request.to_primitive(), model=SkuLinkView)

    def create_sku(self, request: CreateSkuRequest) -> SkuPublicView:
        path = "/skus"
        return self.__client.post(path, request.to_primitive(), model=SkuPublicView)

    def list_skus(self, filter_query: str = None) -> SkuList:
        path = "/skus"
        return self.__client.get(path, drop_none({
            "filter": filter_query,
        }), model=SkuList)

    def get_sku(self, sku_id: str) -> SkuPublicView:
        path = "/skus/{}".format(sku_id)
        return self.__client.get(path, model=SkuPublicView)

    def delete_sku(self, sku_id: str):
        path = "/skus/{}".format(sku_id)
        return self.__client.delete(path)

    def add_pricing_version(self, sku_id: str, request: PricingVersion) -> SkuPublicView:
        path = "/skus/{}/pricingVersions".format(sku_id)
        return self.__client.post(path, request.to_primitive(), model=SkuPublicView)

    def delete_pricing_version(self, sku_id: str, version_id: str) -> SkuPublicView:
        path = "/skus/{}/pricingVersions/{}".format(sku_id, version_id)
        return self.__client.delete(path, model=SkuPublicView)

    def bind_cloud(self, request: BindCloudAccountRequest) -> BillingOperation:
        path = "/clouds/bind"
        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def unbind_cloud(self, request: BindCloudAccountRequest) -> BillingOperation:
        path = "/clouds/unbind"
        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def create_publisher_account(self, request: PublisherAccountCreateRequest) -> BillingOperation:
        path = "/publisherAccounts"
        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def get_publisher_account(self, publisher_account_id, request: PublisherAccountGetRequest = None):
        path = "/publisherAccounts/{}".format(publisher_account_id)
        return self.__client.get(path, params=request.to_primitive(), model=PublisherAccount)

    def update_publisher_account(self, publisher_account_id,
                                 request: PublisherAccountUpdateRequest) -> BillingOperation:
        path = "/publisherAccounts/{}".format(publisher_account_id)
        request.publisher_account_id = publisher_account_id
        return self.__client.patch(path, request.to_primitive(), model=BillingOperation)

    def list_user_persons(self, passport_uid: str):
        path = "/persons"
        return self.__client.get(path, params={"passportUid": passport_uid}, model=PrivatePersonsList)

    def list_publisher_persons(self, publisher_account_id: str):
        path = "/publisherAccounts/{}/persons".format(publisher_account_id)
        return self.__client.get(path, model=PrivatePersonsList)

    def create_grant_policy(self, request: GrantPolicyCreateRequest):
        path = "/grantPolicies"
        return self.__client.post(path, request.to_primitive(), model=GrantPolicyPublicView)

    def get_grant_policy(self, policy_id):
        path = "/grantPolicies/{}".format(policy_id)
        return self.__client.get(path, model=GrantPolicyPublicView)

    def list_grant_policies(self, params: base.BasePublicListingRequest):
        path = "/grantPolicies"
        return self.__client.get(path, params=params, model=GrantPolicyList)

    def delete_grant_policy(self, policy_id):
        path = "/grantPolicies/{}".format(policy_id)
        return self.__client.delete(path)

    def get_billing_account_full_view(self, billing_account_id):
        path = "/billingAccounts/{}/fullView".format(billing_account_id)
        return self.__client.get(path, model=BillingAccountFullView)

    def set_grant_policies(self, billing_account_id, policy_ids: List[str]):
        path = "/billingAccounts/{}:setGrantPolicies".format(billing_account_id)
        return self.__client.post(path, {"policy_ids": policy_ids}, model=BillingAccount)

    def make_isv(self, billing_account_id):
        path = "/billingAccounts/{}:makeISV".format(billing_account_id)
        return self.__client.post(path, {}, model=BillingAccount)

    def add_feature_flag(self, billing_account_id, request: BillingAccountFeatureRequest) -> BillingOperation:
        path = "/billingAccounts/{}:addFeatureFlag".format(billing_account_id)
        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def delete_feature_flag(self, billing_account_id, request: BillingAccountFeatureRequest) -> BillingOperation:
        path = "/billingAccounts/{}:deleteFeatureFlag".format(billing_account_id)
        return self.__client.post(path, request.to_primitive(), model=BillingOperation)

    def get_revenue_reports_meta(self, publisher_account_id: str) -> RevenueMetaResponse:
        path = "/publisherAccounts/{}/revenueMeta".format(publisher_account_id)
        return self.__client.get(path, model=RevenueMetaResponse)

    def get_revenue_reports(self, publisher_account_id: str, request: RevenueReportRequest) -> RevenueAggregateReport:
        path = "/publisherAccounts/{}/skuRevenueReport".format(publisher_account_id)
        return self.__client.post(path, request.to_primitive(), model=RevenueAggregateReport)


def get_billing_private_client(api_version="v1", iam_token=None) -> BillingPrivateClient:
    billing_url = config.get_value('endpoints.billing.private_url')

    return BillingPrivateClient(
        billing_url=billing_url,
        api_version=api_version,
        iam_token=iam_token
    )
