"""
Steps related to secrets.
"""

import logging

import requests
from behave import given

from tests.helpers import secrets
from tests.helpers.workarounds import retry


@given('Secrets api is up and running')
@retry(wait_fixed=1000, stop_max_attempt_number=30)
def secrets_ready(context):
    """
    Wait until secrets api is ready to accept incoming requests.
    """
    base_url = secrets.get_base_url(context)
    ping_url = '{0}/v1/ping'.format(base_url)
    logging.info('Pinging secrets api %s', ping_url)
    response = requests.get(ping_url, verify=False)
    response.raise_for_status()
    logging.info('Secrets api is ok')
