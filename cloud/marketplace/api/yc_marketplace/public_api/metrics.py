from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import DryRunRequest
from cloud.marketplace.common.yc_marketplace_common.models.metrics import ServiceMetricsResponse


@api_handler_without_auth(
    "POST",
    "/simulateBillingMetrics",
    model=DryRunRequest,
    response_model=ServiceMetricsResponse,
)
def simulate_billing_metrics(request, request_context):
    return lib.Metrics().simulate(
        request,
    ).to_api(True)
