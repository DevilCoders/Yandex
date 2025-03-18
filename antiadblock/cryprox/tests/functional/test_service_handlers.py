import requests
import pytest
from urlparse import urljoin

from antiadblock.cryprox.tests.lib.constants import service_handler_error_message
from antiadblock.cryprox.cryprox.config import system as system_config


@pytest.mark.parametrize('relative_url, expected_code, expected_text, headers', [
    ('', 403, service_handler_error_message, dict()),
    ('/proxy', 403, service_handler_error_message, dict()),  # ServiceHandler
    ('', 403, service_handler_error_message, {system_config.REQUEST_ID_HEADER_NAME: "123-123"}),
    ('/proxy', 403, service_handler_error_message, {system_config.REQUEST_ID_HEADER_NAME: "123-123"}),
    ('/ping', 200, 'Ok.\n', dict())])  # PingHandler
def test_service_handlers(relative_url, expected_code, expected_text, cryprox_service_url, headers):
    response = requests.get(urljoin(cryprox_service_url, relative_url), headers=headers)

    assert expected_code == response.status_code
    assert expected_text == response.text
