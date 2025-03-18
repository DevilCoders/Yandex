import logging

from juggler_sdk import JugglerApi, CheckFilter

from library.python.monitoring.solo.handlers.juggler.base.handler import JugglerBaseHandler

logger = logging.getLogger(__name__)


class JugglerHandler(JugglerBaseHandler):
    def __init__(self, juggler_token, juggler_mark, juggler_endpoint, downtime_changed=True):
        super(JugglerHandler, self).__init__(juggler_mark, juggler_endpoint)
        self.api = JugglerApi(api_url=self.endpoint, mark=self.juggler_mark, dry_run=False,
                              force=True, oauth_token=juggler_token,
                              downtime_changed=downtime_changed)

    def get(self, resource):
        if resource.local_state:
            response = self._get_api_call(self.api.get_one_check, service=resource.local_state.service, host=resource.local_state.host)()
            self._fill_resource_provider_id(resource)
        else:
            response = self._get_api_call(self.api.get_one_check, service=resource.provider_id["service"], host=resource.provider_id["host"])()
        resource.provider_state = response

    def create(self, resource):
        self._get_api_call(self.api.upsert_check, resource.local_state)()

    def modify(self, resource):
        self._get_api_call(self.api.upsert_check, resource.local_state)()

    def delete(self, resource):
        filter_for_check = [
            CheckFilter(
                host=resource.provider_id["host"],
                service=resource.provider_id["service"],
                namespace=resource.provider_id.get("namespace"),
            )
        ]
        response = self._get_api_call(self.api.remove_checks, filter_for_check)()
        if not response.results[0].success or len(response.results[0].removed) == 0:
            raise RuntimeError(response.results[0].message)
