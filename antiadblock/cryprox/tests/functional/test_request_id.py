import pytest
import requests

from antiadblock.cryprox.cryprox.config.system import NGINX_SERVICE_ID_HEADER


@pytest.mark.parametrize('request_id, expected_output_id',
                         [('test.local_cryproxtest-test.test_1234fg5t445', 'test.local_cryproxtest-test.test_1234fg5t445'),
                          ('None', 'None'),
                          ('123123', '123123'),
                          ('', 'None')])
def test_request_id(request_id, expected_output_id, cryprox_worker_url):
    if request_id == '':
        test_headers = {}
    else:
        test_headers = {"X-AAB-RequestId": request_id}

    response = requests.get(cryprox_worker_url, headers=test_headers)
    assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'unknown'
    assert 403 == response.status_code
    assert response.text == 'Forbidden\n'
