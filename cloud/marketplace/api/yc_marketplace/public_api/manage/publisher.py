"""Publisher API"""
from yc_common import config
from yc_common.clients.models.operations import OperationV1Beta1

from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authz_by_object
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib import Billing
from cloud.marketplace.common.yc_marketplace_common.lib.publisher import Publisher
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueAggregateReport
from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueMetaResponse
from cloud.marketplace.common.yc_marketplace_common.models.billing.revenue_report import RevenueReportRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherFullResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherList
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherOperation
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherRevenueReportRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherUpdateRequest


@api_handler(
    "GET",
    "/manage/publishers/<publisher_id>",
    context_params=["publisher_id"],
    response_model=PublisherResponse,
)
@i18n_traverse()
def get_publisher(publisher_id, request_context) -> PublisherResponse:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_PUBLISHER,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    publisher = Publisher.rpc_get(publisher_id, auth=auth_func)

    return publisher.to_api(True)


@api_handler(
    "GET",
    "/manage/publishers/<publisher_id>/fullView",
    context_params=["publisher_id"],
    response_model=PublisherResponse,
)
@i18n_traverse()
def get_publisher_full_view(publisher_id, request_context) -> PublisherFullResponse:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_PUBLISHER,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    publisher = Publisher.rpc_get_full(publisher_id, auth=auth_func)

    return publisher.to_api(True)


@api_handler(
    "GET",
    "/manage/publishers",
    params_model=BaseMktManageListingRequest,
    response_model=PublisherList,
)
@authz(action_name=ActionNames.LIST_PUBLISHERS)
@i18n_traverse()
def list_publishers(request: BaseMktManageListingRequest, request_context) -> PublisherList:
    return Publisher.rpc_list(request, billing_account_id=request.billing_account_id, filter_query=request.filter,
                              order_by=request.order_by).to_api(True)


@api_handler(
    "POST",
    "/manage/publishers",
    model=PublisherRequest,
    response_model=OperationV1Beta1,
)
@authz(action_name=ActionNames.CREATE_PUBLISHER)
@i18n_traverse()
def create_publisher(request, request_context) -> PublisherOperation:
    return Publisher.rpc_create(request, request_context).to_api(False)


@api_handler(
    "PATCH",
    "/manage/publishers/<publisher_id>",
    model=PublisherUpdateRequest,
    response_model=OperationV1Beta1,
    context_params=["publisher_id"],
    query_variables=True,
)
@i18n_traverse()
def update_publisher(request, request_context) -> PublisherOperation:
    # auth_func = api_authz_by_object(
    #     service_name=ServiceNames.PUBLIC,
    #     action_name=ActionNames.UPDATE_PUBLISHER,
    #     request_context=request_context,
    #     auth_fields={"ba_id": "billing_account_id"},
    # )
    # TODO: After Scale make it work
    raise NotImplementedError()
    # return Publisher.rpc_update(request, request_context, auth=auth_func).to_api(False)


@api_handler(
    "GET",
    "/manage/publishers/<publisher_id>/revenueReportsMeta",
    context_params=["publisher_id"],
    response_model=RevenueMetaResponse
)
@i18n_traverse()
def get_revenue_reports_meta(publisher_id, request_context) -> RevenueMetaResponse:
    billing_url = config.get_value("endpoints.billing.url")
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_PUBLISHER,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )
    publisher = Publisher.rpc_get(publisher_id, auth=auth_func)
    return Billing.get_revenue_reports_meta(billing_url, publisher_account_id=publisher.billing_publisher_account_id)


@api_handler(
    "POST",
    "/manage/publishers/<publisher_id>:getRevenueReports",
    context_params=["publisher_id"],
    model=PublisherRevenueReportRequest,
    response_model=RevenueAggregateReport,
    query_variables=True
)
@i18n_traverse()
def get_revenue_reports(request, request_context) -> RevenueAggregateReport:
    billing_url = config.get_value("endpoints.billing.url")
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_PUBLISHER,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )
    publisher = Publisher.rpc_get(request.publisher_id, auth=auth_func)
    billing_request = RevenueReportRequest.new(start_date=request.start_date,
                                               end_date=request.end_date,
                                               sku_ids=request.sku_ids)
    return Billing.get_revenue_reports(billing_url, publisher_account_id=publisher.billing_publisher_account_id,
                                       request=billing_request)
