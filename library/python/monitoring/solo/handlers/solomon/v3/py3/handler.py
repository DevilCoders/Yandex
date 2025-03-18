import asyncio
import json
import logging

import aiohttp
import aiohttp_retry
from google.protobuf.json_format import Parse, MessageToJson

from library.python.monitoring.solo.handlers.solomon.v3.base.handler import SolomonV3BaseHandler, OBJECT_TYPE_TO_CLASS, OBJECT_TYPES_CREATED_WITH_ID
from library.python.monitoring.solo.objects.solomon.v3 import Project, Cluster, Service, Shard

logger = logging.getLogger(__name__)

RETRY_OPTIONS = dict(
    attempts=5,
    start_timeout=0.3,
    factor=2,
    exceptions=[aiohttp.ClientError],  # retry "all" errors (but not 4xx codes)
    statuses=None,  # aiohttp_retry lib retries 5xx by default
)


class SolomonV3Handler(SolomonV3BaseHandler):
    def __init__(self, token, endpoint):
        super(SolomonV3Handler, self).__init__(endpoint)
        self.client = aiohttp_retry.RetryClient()
        self.client._client.headers.update({
            "Content-Type": "application/json",
            "Authorization": "OAuth {0}".format(token)
        })

    async def _execute_request(
        self,
        request_method,
        request_details,
        expected_response_type=None,
        pass_404=False,
        pass_response_body_value_error=False
    ):
        # https://cloud.yandex.ru/docs/api-design-guide/concepts/errors#http-mapping
        # CONTRACT: retry all 5xx/connection errors, do not retry 4xx
        # in case of 4xx/zero retries left - raise Exception
        # in case of successful request - return response body

        async def parse_response_body(_response):
            try:
                response_body = await _response.read()
                if expected_response_type is not None and response.status < 400:
                    return Parse(response_body, expected_response_type())
                else:
                    return json.loads(response_body)
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
                return await parse_response_body(response)

    async def get(self, resource):
        if resource.local_state:
            if not resource.provider_id:
                resource.provider_id = {
                    "project_id": resource.local_state.project_id,
                    "object_type": resource.local_state.DESCRIPTOR.name,
                    "handler_type": self.__class__.__name__
                }
            if resource.provider_id["object_type"] in OBJECT_TYPES_CREATED_WITH_ID:
                resource.provider_id["id"] = resource.local_state.id

        if not resource.provider_id.get("id"):
            return None

        response = await self._execute_request(
            request_method="GET",
            request_details=dict(
                url=self._get_url(resource)
            ),
            expected_response_type=OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]],
            pass_404=True
        )
        resource.provider_state = response

    async def create(self, resource):
        response = await self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource)[:self._get_url(resource).rfind("/")],
                data=MessageToJson(self._get_create_request(resource))
            ),
            expected_response_type=OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]],
            pass_404=False
        )
        resource.provider_id["id"] = response.id

    async def modify(self, resource):
        # TODO(lazuka23) - version modification?
        await self._execute_request(
            request_method="PATCH",
            request_details=dict(
                url=self._get_url(resource),
                data=MessageToJson(self._get_update_request(resource))
            ),
            pass_404=False
        )

    async def delete(self, resource):
        if resource.provider_id["object_type"] in {
            Project.DESCRIPTOR.name,
            Cluster.DESCRIPTOR.name,
            Service.DESCRIPTOR.name,
            Shard.DESCRIPTOR.name
        }:
            logger.warning("Can't delete \"{0}\" via solo - you'll have to delete it in UI".format(resource.local_id))
            return
        await self._execute_request(
            request_method="DELETE",
            request_details=dict(
                url=self._get_url(resource),
                data=MessageToJson(self._get_delete_request(resource))
            ),
            pass_404=False,
            pass_response_body_value_error=True
        )

    def finish(self):
        asyncio.get_event_loop().run_until_complete(self.client.close())
