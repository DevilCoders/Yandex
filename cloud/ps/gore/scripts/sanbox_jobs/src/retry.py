import requests

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

from cloud.ps.gore.scripts.sanbox_jobs.config import (
    SCHEME
)


def requests_retry_session(
        retries=3,
        backoff_factor=0.3,
        status_forcelist=(500, 502, 504),
        session=None):
    session = session or requests.Session()
    retry = Retry(
        total=retries,
        read=retries,
        connect=retries,
        backoff_factor=backoff_factor,
        status_forcelist=status_forcelist,
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('%s://' % SCHEME, adapter)
    return session
