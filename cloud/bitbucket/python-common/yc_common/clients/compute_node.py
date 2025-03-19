"""Compute Node public client."""

from yc_common import constants

from .api import ApiClient
from .common import ServiceHealth


# Attention: This is a special public client to check service health
#            DON'T PUT ANY PRIVATE METHOD HERE!
class ComputeNodeClient:
    def __init__(self, node_name):
        self._client = ApiClient("http://{host}:8000/v1".format(host=node_name),
                                 timeout=constants.SERVICE_REQUEST_TIMEOUT)

    def get_health(self):
        return self._client.get("/health", model=ServiceHealth)
