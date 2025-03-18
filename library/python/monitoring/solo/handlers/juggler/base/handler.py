import functools
import logging

import retry.api

from library.python.monitoring.solo.util.diff import get_juggler_diff

JUGGLER_API_URL = "http://juggler-api.search.yandex.net"
JUGGLER_MARK_PREFIX = "a_mark_"

logger = logging.getLogger(__name__)


class RetryableJugglerException(Exception):
    pass


class JugglerBaseHandler(object):
    def __init__(self, juggler_mark, endpoint):
        self.juggler_mark = juggler_mark
        self.endpoint = endpoint

    def diff(self, resource):
        return get_juggler_diff(self.juggler_mark, resource.local_state, resource.provider_state)

    def finish(self):
        pass

    def _fill_resource_provider_id(self, resource):
        resource.provider_id = {
            "service": resource.local_state.service,
            "host": resource.local_state.host,
            "namespace": resource.local_state.namespace,
            "handler_type": self.__class__.__name__,
        }

    def _get_api_call(self, f, *args, **kwargs):

        def _api_call():
            try:
                return f(*args, **kwargs)
            except Exception as err:
                # retry all http/tcp level errors, do not retry 4xx codes
                err_msg = str(err)
                if "HTTP API call failed" in err_msg and "HTTPError: HTTP 5" in err_msg:
                    raise RetryableJugglerException(err_msg)
                elif "Unable to read" in err_msg:
                    raise RetryableJugglerException(err_msg)
                else:
                    raise

        return functools.partial(retry.api.retry_call, _api_call, None, None, exceptions=(RetryableJugglerException,),
                                 tries=5, delay=0.3, backoff=2, logger=logger)
