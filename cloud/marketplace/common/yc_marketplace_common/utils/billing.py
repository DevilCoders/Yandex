from yc_common import config
from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token


def get_billing_client():
    return BillingPrivateClient(
        billing_url=config.get_value("endpoints.billing.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )
