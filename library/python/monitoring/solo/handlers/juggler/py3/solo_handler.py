import asyncio
import json
import logging

import aiohttp
import aiohttp_retry

from library.python.monitoring.solo.handlers.juggler.base.solo_handler import SoloBaseHandler

logger = logging.getLogger(__name__)

RETRY_OPTIONS = dict(
    attempts=5,
    start_timeout=0.5,
    factor=2,
    exceptions=[aiohttp.ClientError],
    statuses=None,
)


class SoloJugglerHandler(SoloBaseHandler):
    def __init__(self, juggler_token, juggler_endpoint):
        super(SoloJugglerHandler, self).__init__(juggler_endpoint)
        self.client = aiohttp_retry.RetryClient(logger, aiohttp_retry.ExponentialRetry(**RETRY_OPTIONS))
        self.client._client.headers.update({
            "Content-Type": "application/json",
            "Authorization": f"OAuth {juggler_token}"
        })

    def finish(self):
        asyncio.get_event_loop().run_until_complete(self.client.close())

    async def get(self, resource):
        request_filter = self._build_request_filter(resource)

        async with self.client.post(
            url=f"{self.endpoint}/v2/dashboards/get_dashboards",
            data=json.dumps({
                "filters": [request_filter]
            })
        ) as response:
            json_response = await response.json()
            self.process_get(resource, json_response)

    async def create(self, resource):
        async with self.client.post(
            url=f"{self.endpoint}/v2/dashboards/set_dashboard",
            data=json.dumps(resource.local_state.to_json())
        ) as response:
            json_response = await response.json()

            self.process_create(response.status, resource, json_response)

    async def modify(self, resource):
        snapshot = {
            "dashboard_id": resource.provider_id["dashboard_id"],
            **resource.local_state.to_json()
        }

        async with self.client.post(
            url=f"{self.endpoint}/v2/dashboards/set_dashboard",
            data=json.dumps(snapshot)
        ) as response:
            json_response = await response.json()
            self.process_create(response.status, resource, json_response)

    async def delete(self, resource):
        await self.client.post(
            url=f"{self.endpoint}/v2/dashboards/remove_dashboard",
            data=json.dumps({
                "dashboard_id": resource.provider_id["dashboard_id"],
            })
        )
