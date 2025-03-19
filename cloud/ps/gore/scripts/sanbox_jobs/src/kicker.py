import logging
import requests

from requests.exceptions import BaseHTTPError

from cloud.ps.gore.scripts.sanbox_jobs.src.retry import requests_retry_session
from cloud.ps.gore.scripts.sanbox_jobs.config import (
    SCHEME,
    GORE_URL,
    GORE_API_ROOT,
    GORE_EXPORT,
    GORE_IMPORT,
    SB_TOKEN
)

_headers = {
    'Content-Type': 'application/json',
    'Authorization': 'OAuth %s' % SB_TOKEN
    }


def renew_data(service):
    status = 200
    url = "%s://%s/%s/%s/%s" % (SCHEME, GORE_URL, GORE_API_ROOT, GORE_IMPORT, service)
    logging.info("Renewing data for %s" % service)
    with requests.Session() as s:
        s.headers.update(_headers)
        try:
            r = requests_retry_session(session=s).post(url)
        except BaseHTTPError as e:
            logging.error("Network error: %s" % e)
            status = 500
            return status

        if r.status_code != 200:
            logging.error("Got status %s" % r.status_code)
            status = r.status_code
            return status

        text = r.text
        logging.info("%s" % text)

    return status


def renew_apis(service):
    status = 200
    url = "%s://%s/%s/%s/%s" % (SCHEME, GORE_URL, GORE_API_ROOT, GORE_EXPORT, service)
    logging.info("Renewing APIs for %s" % service)
    with requests.Session() as s:
        s.headers.update(_headers)
        try:
            r = requests_retry_session(session=s).post(url)
        except BaseHTTPError as e:
            status = 500
            logging.error("Network error: %s" % e)
            return (service, status)

        if r.status_code != 200:
            logging.error("Got status %s" % r.status_code)
            status = r.status_code
            return (service, status)

        text = r.text
        logging.info("%s" % text)

    return (service, status)
