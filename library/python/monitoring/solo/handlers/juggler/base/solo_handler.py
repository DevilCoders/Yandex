import logging

from library.python.monitoring.solo.util.diff import get_jsonobject_diff

logger = logging.getLogger(__name__)


class SoloBaseHandler:
    def __init__(self, endpoint):
        self.endpoint = endpoint

    def diff(self, resource):
        return get_jsonobject_diff(resource.local_state, resource.provider_state)

    def finish(self):
        pass

    def _build_request_filter(self, resource):
        request_filter = {
            "address": resource.local_state["address"],
        }

        if resource.provider_id is not None:
            request_filter["dashboard_id"] = resource.provider_id["dashboard_id"]

        return request_filter

    def _fill_resource_provider_id(self, resource):
        resource.provider_id = {
            "address": resource.provider_state["address"],
            "dashboard_id": resource.provider_state["dashboard_id"],
            "handler_type": self.__class__.__name__,
        }

    def process_get(self, resource, json_response):
        if len(json_response["items"]) == 0:
            return

        resource.provider_state = json_response["items"][0]
        self._fill_resource_provider_id(resource)

    def process_create(self, status, resource, json_response):
        if status >= 400:
            raise RuntimeError("Error on http request, response body: {0}".format(json_response))
        else:
            dashboard_id = json_response["dashboard_id"]
            resource.local_state.dashboard_id = dashboard_id
            resource.provider_id = {
                "dashboard_id": dashboard_id,
                "handler_type": self.__class__.__name__,
            }
