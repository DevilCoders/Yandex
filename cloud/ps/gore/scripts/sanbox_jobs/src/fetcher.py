import requests
import logging

from requests.exceptions import BaseHTTPError

from cloud.ps.gore.scripts.sanbox_jobs.src.retry import requests_retry_session
from cloud.ps.gore.scripts.sanbox_jobs.config import (
    SCHEME,
    GORE_URL,
    GORE_API_ROOT,
    GORE_SERVICES
)


def fetch_services():
    url = "%s://%s/%s/%s" % (SCHEME, GORE_URL, GORE_API_ROOT, GORE_SERVICES)
    logging.info("Fetching service list")
    with requests.Session() as s:
        try:
            r = requests_retry_session(session=s).get(url)
        except BaseHTTPError as e:
            logging.error("Network error: %s" % e)
            return

        if r.status_code != 200:
            logging.error("Got status %s" % r.status_code)
            return

        json = r.json()
        logging.info("%s" % json)
        return ([x["service"] for x in json])
