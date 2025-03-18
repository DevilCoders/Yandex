import json
import logging
import requests
import requests.exceptions
import retry

from library.python.monitoring.solo.handlers.juggler.base.solo_handler import SoloBaseHandler

logger = logging.getLogger(__name__)


class SoloJugglerHandler(SoloBaseHandler):
    def __init__(self, juggler_token, juggler_endpoint):
        super(SoloJugglerHandler, self).__init__(juggler_endpoint)
        self.session = requests.Session()
        self.session.headers.update({
            "Content-Type": "application/json",
            "Authorization": "OAuth {0}".format(juggler_token)
        })

    def finish(self):
        self.session.close()

    @retry.retry(exceptions=requests.exceptions.RequestException, tries=5, backoff=2, delay=0.3, logger=logger)
    def get(self, resource):
        response = self.session.post(
            url="{}/v2/dashboards/get_dashboards".format(self.endpoint),
            data=json.dumps({
                "filters": [self._build_request_filter(resource)]
            })
        )

        json_response = response.json()
        self.process_get(resource, json_response)

    @retry.retry(exceptions=requests.exceptions.RequestException, tries=5, backoff=2, delay=0.3, logger=logger)
    def create(self, resource):
        response = self.session.post(
            url="{}/v2/dashboards/set_dashboard".format(self.endpoint),
            data=json.dumps(resource.local_state.to_json())
        )

        json_response = response.json()
        self.process_create(response.status_code, resource, json_response)

    def modify(self, resource):
        snapshot = resource.local_state.to_json().copy()
        snapshot["dashboard_id"] = resource.provider_id["dashboard_id"]

        response = self.session.post(
            url="{}/v2/dashboards/set_dashboard".format(self.endpoint),
            data=json.dumps(snapshot)
        )

        json_response = response.json()
        self.process_create(response.status_code, resource, json_response)

    @retry.retry(exceptions=requests.exceptions.RequestException, tries=5, backoff=2, delay=0.3, logger=logger)
    def delete(self, resource):
        self.session.post(
            url="{}/v2/dashboards/remove_dashboard".format(self.endpoint),
            data=json.dumps({
                "dashboard_id": resource.provider_id["dashboard_id"],
            })
        )
