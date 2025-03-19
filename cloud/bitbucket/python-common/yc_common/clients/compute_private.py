"""Compute private API client"""

from yc_common import config, constants
from yc_common.misc import drop_none
from yc_common.models import Model, ModelType, StringType, ListType, IntType

from .api import ApiClient
from .common import ServiceHealth


# Attention: This is a special public client for CI
#            DON'T PUT ANY PRIVATE METHOD HERE!
class ComputePrivateClient:
    def __init__(self, url):
        self.__client = ApiClient(url + "/v1", timeout=constants.SERVICE_REQUEST_TIMEOUT)

    def get_schedulers(self, with_allocations_count=None):
        class Scheduler(Model):
            id = StringType(required=True)
            name = StringType()
            allocations = IntType(required=with_allocations_count)

        class Response(Model):
            schedulers = ListType(ModelType(Scheduler), required=True)

        return self.__client.get("/schedulers", params=drop_none({
            "with_allocations_count": with_allocations_count
        }), model=Response).schedulers

    def get_health(self):
        return self.__client.get("/health", model=ServiceHealth)


class ComputePrivateEndpointConfig(Model):
    url = StringType(required=True)


def get_compute_private_client() -> ComputePrivateClient:
    return ComputePrivateClient(config.get_value("endpoints.compute_private", model=ComputePrivateEndpointConfig).url)
