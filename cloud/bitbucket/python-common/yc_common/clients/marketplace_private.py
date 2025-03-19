from typing import List

from schematics.types import BooleanType, StringType

from yc_common import config
from yc_common.clients.api import ApiClient
from yc_common.clients.models import BasePublicModel
from yc_common.constants import ServiceNames
from yc_common.models import Model


class CheckUsagePermissionsResponse(BasePublicModel):
    permission = BooleanType(required=True)  # type: bool


class MarketplacePrivateClient(object):
    def __init__(self, api_url, credentials=None, retry_temporary_errors=None, iam_token=None):
        self.__client = ApiClient(api_url,
                                  service_name=ServiceNames.MARKETPLACE_INTERNAL,
                                  credentials=credentials,
                                  iam_token=iam_token)
        self.__retry_temporary_errors = retry_temporary_errors

    def check_usage_permissions(self, cloud_id, product_ids: List[str]) -> bool:
        data = {
            "cloudId": cloud_id,
            "productIds": product_ids
        }
        return self.__client.post("/private/osProducts:checkUsagePermissions", request=data,
                                  retry_temporary_errors=self.__retry_temporary_errors,
                                  model=CheckUsagePermissionsResponse).permission


class MarketplacePrivateEndpointConfig(Model):
    url = StringType(required=True)


def get_compute_client(iam_token) -> MarketplacePrivateClient:
    return MarketplacePrivateClient(config.get_value("endpoints.marketplace_private",
                                                     model=MarketplacePrivateEndpointConfig).url,
                                    iam_token=iam_token)
