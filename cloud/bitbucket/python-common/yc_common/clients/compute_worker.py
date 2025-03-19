"""Compute Worker client"""

from yc_common import logging

from .api import ApiClient

log = logging.get_logger(__name__)


class ComputeWorkerClient:
    def __init__(self, host, port):
        self.__client = ApiClient("http://{host}:{port}/v1".format(host=host, port=port))

    def on_new_operation(self, operation_id, order_id, resumed=False):
        self.__client.post("/on-new-operation", {
            "id": operation_id,
            "order_id": order_id,
            "resumed": resumed,
        })

    def on_operation_cancelled(self, operation_id, order_id):
        self.__client.post("/on-operation-cancelled", {
            "id": operation_id,
            "order_id": order_id,
        })

    # Test mode API

    def simulate_lockup(self):
        self.__client.post("/simulate-lockup", {})
