from decimal import Decimal

from cloud.marketplace.common.yc_marketplace_common.lib import Metrics
from cloud.marketplace.common.yc_marketplace_common.models.billing.calculators import DryRunRequest
from cloud.marketplace.common.yc_marketplace_common.models.metrics import ServiceMetricsResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import ResourceSpec
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProduct
from cloud.marketplace.common.yc_marketplace_common.utils.ids import generate_id


def test_ok():
    SAAS_PRODUCT_ID = generate_id()
    BILLING_ACCOUNT_ID = generate_id()
    SAAS_SKU_ID = "a6q00000000000saas01"
    request = DryRunRequest({
        "cloudId": "aoe47jkb996097rp5rnr",
        "currency": "RUB",
        "metrics": [
            {
                "folder_id": "aoe401u4tao7jj2b069g",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 37580963840,
                    "type": "network-hdd"
                },
                "version": "v1alpha1",
                "usage": {
                    "type": "delta",
                    "unit": "seconds",
                    "start": 1582207878,
                    "quantity": 2592000,
                    "finish": 1584799878
                }
            },
            {
                "folder_id": "aoe401u4tao7jj2b069g",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 2,
                    "memory": 2147483648,
                    "platform_id": "standard-v1",
                    "product_ids": [
                        "dqnont2npkcetala9fhs"
                    ],
                    "public_fips": 0,
                    "sockets": 1
                },
                "version": "v1alpha1",
                "usage": {
                    "type": "delta",
                    "unit": "seconds",
                    "start": 1582207878,
                    "quantity": 2592000,
                    "finish": 1584799878
                }
            }
        ]
    })

    def list_products(*args, **kwargs):
        return [SaasProduct(dict(
            id=SAAS_PRODUCT_ID,
            billing_account_id=BILLING_ACCOUNT_ID,
            labels={},
            name="saas",
            description="saas",
            short_description="sass",
            vendor="vendor",
            sku_ids=[
                SAAS_SKU_ID
            ],
        ))]

    expected = ServiceMetricsResponse({
        "links": {
            SAAS_SKU_ID: {
                "productId": SAAS_PRODUCT_ID,
                "productType": "saas"
            }
        },
        "metrics": [{
            "schema": "marketplace.image.product",
            "usage": {"start": 1582212273, "finish": 1582212273},
            "tags": {},
            "skuId": SAAS_SKU_ID,
            "pricingQuantity": Decimal("1"),
            "folderId": "aoe401u4tao7jj2b069g",
            "version": "v1alpha1"
        }]
    })

    def list_versions(*args, **kwargs):
        return [OsProductFamilyVersion(dict(
            billing_account_id=generate_id(),
            os_product_family_id=generate_id(),
            pricing_options="payg",
            image_id=generate_id(),
            status=OsProductFamilyVersion.Status.ACTIVE,
            resource_spec=ResourceSpec({
                "cores": 1,
                "diskSize": 26843545600,
                "memory": 2147483648,
                "userDataFormId": "windows"
            }).to_primitive(),
            skus=[
                {
                    "checkFormula": "!(tags.core_fraction) || tags.core_fraction > `5`",
                    "id": "a6q7k40bf89tfcjgbmev"
                },
                {
                    "checkFormula": "tags.core_fraction && tags.core_fraction == `5`",
                    "id": "a6qrb12q3rftcnutduaj"
                }
            ],
            related_products=[{
                "productId": SAAS_PRODUCT_ID,
                "productType": "saas"
            }]
        ))]

    metrics = Metrics(ts=lambda: 1582212273,
                      list_versions=list_versions,
                      list_saas_products=list_products)
    result = metrics.simulate(request)

    assert expected == result
    assert len(result.to_api(public=True)) == 2
