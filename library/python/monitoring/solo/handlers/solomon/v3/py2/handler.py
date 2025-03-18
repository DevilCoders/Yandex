import json
import logging

import requests
import requests.exceptions
import retry
from google.protobuf.json_format import Parse, MessageToJson

from library.python.monitoring.solo.handlers.solomon.v3.base.handler import SolomonV3BaseHandler, OBJECT_TYPE_TO_CLASS, OBJECT_TYPES_CREATED_WITH_ID
from library.python.monitoring.solo.objects.solomon.v3 import Project, Cluster, Service, Shard

logger = logging.getLogger(__name__)


class SolomonV3Handler(SolomonV3BaseHandler):
    def __init__(self, token, endpoint):
        super(SolomonV3Handler, self).__init__(endpoint)
        self.session = requests.Session()
        self.session.headers.update({
            "Content-Type": "application/json",
            "Authorization": "OAuth {0}".format(token)
        })

    @retry.retry(exceptions=requests.exceptions.RequestException, tries=5, backoff=2, delay=0.3, logger=logger)
    def _execute_request(
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

        def parse_response_body(_response):
            try:
                if expected_response_type is not None and response.status_code < 400:
                    return Parse(_response.text, expected_response_type())
                else:
                    return json.loads(_response.text)
            except ValueError as error:
                if pass_response_body_value_error:
                    return None
                raise RuntimeError("Unexpected error on parsing response_body from http response: {0}, response_body: {1}, error: {2}".format(
                    _response, _response.text, error))
            except Exception as error:
                raise RuntimeError("Unexpected error on reading response_body from http response: {0}, error: {1}".format(
                    _response, error))

        response = getattr(self.session, request_method.lower())(**request_details)
        if response.status_code >= 500:
            response.raise_for_status()
        elif response.status_code >= 400:
            if response.status_code == 404 and pass_404:
                return None
            response_body = parse_response_body(response)
            raise RuntimeError("Error on http request, response: {0}, response body: {1}".format(
                response, response_body))
        else:
            return parse_response_body(response)

    def get(self, resource):

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

        response = self._execute_request(
            request_method="GET",
            request_details=dict(
                url=self._get_url(resource),
            ),
            expected_response_type=OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]],
            pass_404=True
        )
        resource.provider_state = response

    def create(self, resource):
        response = self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource)[:self._get_url(resource).rfind("/")],
                data=MessageToJson(self._get_create_request(resource)),
            ),
            expected_response_type=OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]],
            pass_404=False
        )
        resource.provider_id["id"] = response.id

    def modify(self, resource):
        # TODO(lazuka23) - version modification?
        self._execute_request(
            request_method="PATCH",
            request_details=dict(
                url=self._get_url(resource),
                data=MessageToJson(self._get_update_request(resource))
            ),
            pass_404=False
        )

    def delete(self, resource):
        if resource.provider_id["object_type"] in {
            Project.DESCRIPTOR.name,
            Cluster.DESCRIPTOR.name,
            Service.DESCRIPTOR.name,
            Shard.DESCRIPTOR.name
        }:
            logger.warning("Can't delete \"{0}\" via solo - you'll have to delete it in UI".format(resource.local_id))
            return
        self._execute_request(
            request_method="DELETE",
            request_details=dict(
                url=self._get_url(resource),
                data=MessageToJson(self._get_delete_request(resource))
            ),
            pass_404=False,
            pass_response_body_value_error=True
        )

    def finish(self):
        self.session.close()
