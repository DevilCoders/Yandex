import threading
import requests


from yc_common import config
from yc_common import exceptions
from yc_common import logging
from yc_common import models
from yc_common.misc import timestamp
from yc_common.models import BooleanType
from yc_common.models import IntType
from yc_common.models import StringType
from yc_common.clients import api


log = logging.get_logger('yc_marketplace')


class InstanceMetadataEndpointConfig(models.Model):
    enabled = BooleanType(default=True)
    url = StringType(default="http://169.254.169.254")
    master_token = StringType()


class InstanceMetadataToken(models.Model):
    expires_in = IntType()
    access_token = StringType()
    token_type = StringType()


class InstanceMetadataClient(api.ApiClient):
    def __init__(self, url, enabled=False, master_token=None):
        self.__enabled = enabled
        self.__master_token = master_token
        super().__init__(url + "/computeMetadata/v1")

    def token(self):
        if not self.__enabled:
            return InstanceMetadataToken.new(access_token=self.__master_token, expires_in=24 * 60 * 60)

        return self._query("instance/service-accounts/default/token", model=InstanceMetadataToken)

    def _query(self, path, model=None) -> InstanceMetadataToken:
        headers = {"Metadata-Flavor": "Google"}
        return super().get("/" + path, retry_temporary_errors=True, extra_headers=headers, model=model, mask_fields=["access_token"])

    def _parse_response(self, response, model=None):
        if response.status_code != requests.codes.ok:
            raise (api.YcClientInternalError if 500 <= response.status_code < 600 else exceptions.Error)(
                "Instance Metadata API returned an error with {} status code for {}.",
                response.status_code, response.url)

        # Note: Current metadata service sometime returns wrong content-type (text/plain instead of application/json)
        result = raw_result = response.text
        if model is not None:
            try:
                result = raw_result = response.json()
            except exceptions.Error as e:
                self._on_invalid_response(response, e)
            try:
                result = model.from_api(result, ignore_unknown=True)
            except models.ModelValidationError as e:
                self._on_invalid_response(response, e)

        return result, raw_result


_TOKEN_DATA = None
_TOKEN_EXPIRATION = 0
_TOKEN_LOCK = threading.Lock()
_CLOCK_SKEW = 1 * 60

_default_config = InstanceMetadataEndpointConfig()


def get_instance_metadata_client():
    cfg = config.get_value("endpoints.instance_metadata", model=InstanceMetadataEndpointConfig, default=_default_config)
    return InstanceMetadataClient(cfg.url, enabled=cfg.enabled, master_token=cfg.master_token)


def get_instance_metadata_token():
    global _TOKEN_DATA
    global _TOKEN_EXPIRATION
    global _TOKEN_LOCK

    now = timestamp()
    if _TOKEN_EXPIRATION < now:
        with _TOKEN_LOCK:
            if _TOKEN_EXPIRATION < now:
                log.info("Updating token from instance metadata...")
                token = get_instance_metadata_client().token()
                _TOKEN_DATA = token.access_token
                _TOKEN_EXPIRATION = now + token.expires_in - _CLOCK_SKEW

    return _TOKEN_DATA
