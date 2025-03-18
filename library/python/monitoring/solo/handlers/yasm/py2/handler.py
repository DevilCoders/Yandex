import json
import logging

import requests
import requests.exceptions
import retry

from library.python.monitoring.solo.handlers.yasm.base.handler import YasmBaseHandler, OBJECT_TYPE_TO_CLASS, YasmAction

logger = logging.getLogger(__name__)


class YasmHandler(YasmBaseHandler):
    def __init__(self):
        super(YasmHandler, self).__init__()
        self.session = requests.Session()
        self.session.headers.update({
            "Content-Type": "application/json"
        })

    @retry.retry(exceptions=requests.exceptions.RequestException, tries=5, backoff=2, delay=0.3, logger=logger)
    def _execute_request(
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

        def parse_response_body(_response):
            try:
                response_body_json = json.loads(response.text)
                return response_body_json
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
            parsed_response = parse_response_body(response)
            if parsed_response:
                if "response" in parsed_response:
                    return parsed_response["response"]
                if "result" in parsed_response:
                    return parsed_response["result"]

            return parsed_response

    def get(self, resource):
        response = self._execute_request(
            request_method="GET",
            request_details=dict(
                url=self._get_url(resource)
            ),
            pass_404=True
        )
        if resource.local_state:
            self._fill_resource_provider_id(resource)
            if response:
                base_class = OBJECT_TYPE_TO_CLASS[resource.provider_id["object_type"]]
                resource.provider_state = base_class(response)
            else:
                resource.provider_state = None
        else:
            resource.provider_state = response

    def create(self, resource):
        post_data = self.prepare_data_for_upsert(resource)
        self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.CREATE),
                data=json.dumps(post_data)
            ),
            pass_404=False
        )

    def modify(self, resource):
        post_data = self.prepare_data_for_upsert(resource)
        self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.UPDATE),
                data=json.dumps(post_data)
            ),
            pass_404=False
        )

    def delete(self, resource):
        self._execute_request(
            request_method="POST",
            request_details=dict(
                url=self._get_url(resource, YasmAction.DELETE)
            ),
            pass_404=False,
            pass_response_body_value_error=True
        )

    def finish(self):
        self.session.close()
