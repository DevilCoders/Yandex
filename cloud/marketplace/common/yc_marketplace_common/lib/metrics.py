from typing import List

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import DryRunRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import LightMetricRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import LightMetricResponse
from cloud.marketplace.common.yc_marketplace_common.models.metrics import SCHEMAS
from cloud.marketplace.common.yc_marketplace_common.models.metrics import ServiceMetricsResponse
from cloud.marketplace.common.yc_marketplace_common.models.product_reference import ProductReferencePublicModel
from cloud.marketplace.common.yc_marketplace_common.models.product_type import ProductType
from cloud.marketplace.common.yc_marketplace_common.utils.errors import MetricsFolderIdError
from yc_common.misc import drop_none
from yc_common.misc import timestamp


def generate_metric(
        *, timestamp, folder_id, sku_id
):
    return LightMetricRequest({
        "folder_id": folder_id,
        "schema": SCHEMAS.MKT_IMAGE,
        "usage": {
            "start": timestamp(),
            "finish": timestamp(),
        },
        "tags": {},
        "sku_id": sku_id,
        "pricing_quantity": 1,
    })


class Metrics:
    def __init__(self, ts=None, list_versions=None, list_saas_products=None):
        if not ts:
            ts = timestamp
        self.timestamp = ts

        if not list_versions:
            list_versions = lib.OsProductFamilyVersion.get_batch
        self._list_versions = list_versions

        if not list_saas_products:
            list_saas_products = lib.SaasProduct.get_batch
        self._list_saas_products = list_saas_products

    def _simulate_from_compute_metrics(self, compute_metrics: List[LightMetricRequest]):
        """Simulates Mkt Billing metrics from compute metrics"""

        product_ids = []
        folder_ids = set()
        mkt_metrics = []
        links = {}

        metrics = (metric for metric in compute_metrics if metric.schema == SCHEMAS.COMPUTE_GENERIC)

        for metric in metrics:
            product_ids += metric.tags.get("product_ids", [])
            folder_ids.add(metric.folder_id)

        if len(folder_ids) != 1:
            raise MetricsFolderIdError()
        folder_id = folder_ids.pop()

        versions = self._list_versions(product_ids)

        related_products_refs = []
        for v in versions:
            related_products_refs += v.related_products or []

        related_products_ids = [r.product_id for r in related_products_refs]
        saas_products = self._list_saas_products(related_products_ids)

        for product in saas_products:
            for sku_id in product.sku_ids:
                mkt_metrics.append(generate_metric(
                    timestamp=self.timestamp,
                    folder_id=folder_id,
                    sku_id=sku_id,
                ))
                links[sku_id] = ProductReferencePublicModel({
                    "product_type": ProductType.SAAS,
                    "product_id": product.id,
                })

        return mkt_metrics, links

    @staticmethod
    def to_service_response(metrics: List[LightMetricRequest], links=None):
        return ServiceMetricsResponse(drop_none({
            "metrics": [LightMetricResponse(m.to_primitive()) for m in metrics],
            "links": links
        }))

    def simulate(self, request: DryRunRequest):
        mkt_metrics, links = self._simulate_from_compute_metrics(request.metrics)

        return Metrics.to_service_response(mkt_metrics, links=links)
