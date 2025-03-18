import asyncio
import json
import logging

import aiohttp
import aiohttp_retry
from library.python.monitoring.solo.handlers.yasm.base.handler import OBJECT_TYPE_TO_CLASS, YasmAction, YasmBaseHandler

logger = logging.getLogger(__name__)

RETRY_OPTIONS = dict(
    attempts=5,
    start_timeout=0.3,
    factor=2,
    exceptions=[aiohttp.ClientError],  # retry "all" errors (but not 4xx codes)
    statuses=None,  # aiohttp_retry lib retries 5xx by default
)


class YasmHandler(YasmBaseHandler):
    def __init__(self):
        super(YasmHandler, self).__init__()
        self.client = aiohttp_retry.RetryClient()
        self.client._client.headers.update({
            "Content-Type": "application/json"
        })

    async def _execute_request(
        self,
        request_method,
        request_details,
        pass_404=False,
        pass_response_body_value_error=False
    ):
        # https://solomon.yandex-team.ru/docs/api-ref/rest#error
        # CONTRACT: retry all 5xx/connection errors, do not retry 4xx
        # in case of 4xx/zero retries left - raise Exception
        # in case of successful request - return response body

        async def parse_response_body(_response):
            try:
                response_body = await _response.read()
                response_body_json = json.loads(response_body)
                return response_body_json
            except ValueError as error:
                if pass_response_body_value_error:
                    return None
                raise RuntimeError("Unexpected error on parsing response_body from http response: {0}, response_body: {1}, error: {2}".format(
                    _response, response_body, error))
            except Exception as error:
                raise RuntimeError("Unexpected error on reading response_body from http response: {0}, error: {1}".format(
                    _response, error))

        request_details["retry_options"] = aiohttp_retry.ExponentialRetry(**RETRY_OPTIONS)
        async with getattr(self.client, request_method.lower())(
            raise_for_status=False,
            **request_details
        ) as response:

            if response.status >= 400:
                if response.status == 404 and pass_404:
                    return None
                response_body = await parse_response_body(response)
                raise RuntimeError("Error on http request, response: {0}, response body: {1}".format(
                    response, response_body))
            else:
                parsed_response = await parse_response_body(response)
                if parsed_response:
                    if "response" in parsed_response:
                        return parsed_response["response"]
                    if "result" in parsed_response:
                        return parsed_response["result"]

                return parsed_response

    async def get(self, resource):
        response = await self._execute_request(
            request_method="GET",
            request_details=dict(
                url=self._get_url(resource)
            ),
            pass_404=True
        )
        if resource.local_state:
            # fill provider_id based on local_state
            self._fill_resource_provider_id(resource)
            if response:
                # parse response using local_state class constructor
                base_class = OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]]
                resource.provider_state = base_class(response)
            else:
                # no local state and no provider state - obsolete untracked object, no need to fill provider_state
                resource.provider_state = None
        else:
            # no local state - untracked object, fill provider_state with response data without any processing
            resource.provider_state = response

    async def create(self, resource):
        post_data = json.dumps(self.prepare_data_for_upsert(resource))

        await self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.CREATE),
                data=post_data
            ),
            pass_404=False
        )

    async def modify(self, resource):
        post_data = json.dumps(self.prepare_data_for_upsert(resource))

        await self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.UPDATE),
                data=post_data
            ),
            pass_404=False
        )

    async def delete(self, resource):
        await self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.DELETE)
            ),
            pass_404=False,
            pass_response_body_value_error=True
        )

    def finish(self):
        asyncio.get_event_loop().run_until_complete(self.client.close())
