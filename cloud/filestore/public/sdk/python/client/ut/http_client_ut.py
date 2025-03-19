import logging
import os
import pytest
import urllib3

try:
    from http.server import HTTPServer, BaseHTTPRequestHandler
except ImportError:
    # python2
    from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler

from yatest.common import network

import cloud.filestore.public.sdk.python.protos as protos

from cloud.filestore.public.sdk.python.client.http_client import HttpEndpointClient
from cloud.filestore.public.sdk.python.client.error import ClientError
from cloud.filestore.public.sdk.python.client.error_codes import EResult

from cloud.filestore.public.sdk.python.client.ut.client_methods \
    import endpoint_methods


logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger("test_logger")


def _test_every_method(sync, timeout):
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

    port = int(os.getenv("NFS_HTTP_PROXY_PORT"))

    host = "localhost"
    server_mock = None
    if timeout is not None:
        with network.PortManager() as pm:
            port = pm.get_port()

        class NoOp(BaseHTTPRequestHandler):

            def do_GET(self):
                pass

            def do_POST(self):
                pass

        server_mock = HTTPServer((host, port), NoOp)  # noqa

    addr = "{}://{}:{}".format("http", host, port)

    http_client = HttpEndpointClient(
        addr,
        log=logger,
        timeout=timeout,
        connect_timeout=timeout)

    for endpoint_method in endpoint_methods:
        method_name = endpoint_method[0]
        if not sync:
            method_name += "_async"
        request_name = "T%sRequest" % endpoint_method[1]
        method = getattr(http_client, method_name)
        request_class = getattr(protos, request_name)
        request = request_class()

        te = None

        try:
            if sync:
                method(request)
            else:
                method(request).result()
        except ClientError as e:
            if e.code == EResult.E_TIMEOUT.value:
                te = e
            else:
                logger.error(e)
                pytest.fail(str(e))
        except Exception as e:
            logger.error(e)
            pytest.fail(str(e))

        if te is not None and timeout is None:
            pytest.fail(str(te))

        if te is None and timeout is not None:
            pytest.fail("expected timeout, got success")


@pytest.mark.parametrize("sync", ['sync', 'async'])
@pytest.mark.parametrize("timeout", [1, None])
def test_every_method(sync, timeout):
    _test_every_method(sync == 'sync', timeout=timeout)
