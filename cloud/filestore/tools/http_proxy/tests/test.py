import os
import requests
import urllib3
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

import ydb.tests.library.common.yatest_common as yatest_common

from cloud.filestore.tests.python.lib.http_proxy import create_nfs_http_proxy


def post(s, scheme, port, path):
    return s.post('{}://localhost:{}/{}'.format(scheme, port, path), verify=False)


def test_ping():
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

    port_manager = yatest_common.PortManager()
    port = port_manager.get_port()
    nfs_port = os.getenv("NFS_VHOST_PORT")

    with create_nfs_http_proxy(port, nfs_port), requests.Session() as s:
        s.mount('https://', HTTPAdapter(max_retries=Retry(total=10, backoff_factor=0.5)))
        s.mount('http://', HTTPAdapter(max_retries=Retry(total=10, backoff_factor=0.5)))

        r = s.post('http://localhost:{}/ping'.format(port))
        assert r.status_code == 200
        assert r.text == "{}"
