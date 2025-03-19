"""Instance Metadata client."""

import cgi

import requests

from yc_common import logging
from yc_common.clients.api import ApiClient, YcClientInternalError
from yc_common.exceptions import Error

log = logging.get_logger(__name__)


class InstanceMetadataClient(ApiClient):
    def __init__(self):
        super().__init__("http://169.254.169.254/latest/meta-data")

    def get_metadata(self, path):
        return super().get("/" + path, retry_temporary_errors=True)

    def _parse_response(self, response, model=None):
        content_type, type_options = cgi.parse_header(response.headers.get("Content-Type", ""))

        if response.status_code != requests.codes.ok:
            raise (YcClientInternalError if 500 <= response.status_code < 600 else Error)(
                "Instance Metadata API returned an error with {} status code for {}.",
                response.status_code, response.url)

        if content_type != "text/plain":
            self._on_invalid_response(response, Error("Invalid Content-Type: {!r}.", content_type))

        return response.text, response
