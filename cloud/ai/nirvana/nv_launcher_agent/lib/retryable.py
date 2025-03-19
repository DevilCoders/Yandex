import requests
from retry import retry


class UnexpectedStatusCodeException(Exception):
    pass

def retryable(f):
    @retry((requests.exceptions.Timeout, requests.exceptions.ConnectionError, UnexpectedStatusCodeException), tries=100, max_delay=7200, delay=1, backoff=1.2)
    def retryable_decorator(*args, **kwargs):
        status = f(*args, **kwargs)

        if not (200 <= status < 300):
            raise UnexpectedStatusCodeException("Bad return status code")

        return status

    return retryable_decorator
