import traceback
import sys

from tornado.httpclient import HTTPError
from tornado.gen import coroutine, Return
from tenacity import retry, stop_after_attempt, wait_exponential

try:
    MAX_WAIT = sys.maxint / 2
except AttributeError:
    MAX_WAIT = 1073741823


class FetchRequestException(Exception):
    pass


class FetchRequestFatalException(FetchRequestException):
    pass


class FetchRequestRetryableException(FetchRequestException):
    pass


@coroutine
def http_retry(client, request, raise_error=True, tries=-1, delay=0, max_delay=None, backoff=1, **kwargs):
    max_delay = max_delay or MAX_WAIT

    @retry(stop=stop_after_attempt(tries), wait=wait_exponential(multiplier=backoff, min=delay, max=max_delay))
    @coroutine
    def fetch_request(client, request):
        try:
            response = yield client.fetch(request, raise_error=raise_error, **kwargs)
            if response.error and 500 <= response.code <= 599:
                raise FetchRequestRetryableException
            raise Return(response)
        except HTTPError:
            raise FetchRequestRetryableException

    try:
        response = yield fetch_request(client, request)
        raise Return(response)
    except FetchRequestRetryableException:
        raise FetchRequestFatalException('All tries to fetch resource have failed! Last exception was ({})'
                                         .format(traceback.format_exc()))
