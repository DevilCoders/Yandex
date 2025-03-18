import asyncio

import juggler_sdk

from library.python.monitoring.solo.handlers.juggler.base.handler import JugglerBaseHandler


class JugglerHandler(JugglerBaseHandler):
    def __init__(self, juggler_token, juggler_mark, juggler_endpoint, downtime_changed=True):
        # TODO downtime_changed в секундах
        super(JugglerHandler, self).__init__(juggler_mark, juggler_endpoint)
        self.loop = asyncio.get_event_loop()
        self.api = juggler_sdk.JugglerApi(api_url=self.endpoint, mark=self.juggler_mark, dry_run=False,
                                          force=True, oauth_token=juggler_token,
                                          downtime_changed=downtime_changed)

    async def get(self, resource):
        if resource.local_state:
            response = await self.loop.run_in_executor(None, self._get_api_call(
                self.api.get_one_check, service=resource.local_state.service, host=resource.local_state.host))
            self._fill_resource_provider_id(resource)
        else:
            response = await self.loop.run_in_executor(None, self._get_api_call(
                self.api.get_one_check, service=resource.provider_id["service"], host=resource.provider_id["host"]))
        resource.provider_state = response

    async def create(self, resource):
        await self.loop.run_in_executor(None, self._get_api_call(self.api.upsert_check, resource.local_state))

    async def modify(self, resource):
        await self.loop.run_in_executor(None, self._get_api_call(self.api.upsert_check, resource.local_state))

    async def delete(self, resource):
        filter_for_check = [
            juggler_sdk.CheckFilter(
                host=resource.provider_state.host,
                service=resource.provider_state.service,
                namespace=resource.provider_state.namespace,
            )
        ]
        response = await self.loop.run_in_executor(None, self._get_api_call(self.api.remove_checks, filter_for_check))
        if not response.results[0].success or len(response.results[0].removed) == 0:
            raise RuntimeError(response.results[0].message)
