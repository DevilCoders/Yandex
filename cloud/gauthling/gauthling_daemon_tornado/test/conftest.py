from concurrent import futures

import grpc
import pytest
import requests

from gauthling_daemon_tornado import GauthlingClient
from gauthling_daemon_mock import ControlServer, GauthlingMockServicer, AccessServiceMockServicer, gauthling_pb2_grpc, access_service_pb2_grpc

from yatest.common import network


@pytest.fixture(scope="session", autouse=True)
def gauthling_daemon_mock(request):
    host = "localhost"
    pm = network.PortManager()
    gauthling_port = pm.get_port()
    control_port = pm.get_port()

    # Start gauthling mock server
    gauthling = GauthlingMockServicer()
    access_service = AccessServiceMockServicer()
    gauthling_server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    gauthling_pb2_grpc.add_GauthlingServiceServicer_to_server(gauthling, gauthling_server)
    access_service_pb2_grpc.add_AccessServiceServicer_to_server(access_service, gauthling_server)
    gauthling_server.add_insecure_port("{0}:{1}".format(host, gauthling_port))
    gauthling_server.start()

    # Start control server
    control_server_executor = futures.ThreadPoolExecutor(max_workers=1)
    control_server = ControlServer(gauthling, access_service, "GauthlingMockControl")
    control_server_executor.submit(control_server.run, host=host, port=control_port)

    # Add teardowns
    control_server_shutdown_url = "http://{0}:{1}/shutdown".format(host, control_port)
    request.addfinalizer(lambda: _teardown(pm, gauthling_server, control_server_shutdown_url))
    return host, gauthling_port, control_port


def _teardown(port_manager, gauthling_server, control_server_shutdown_url):
    gauthling_server.stop(grace=True)
    requests.post(control_server_shutdown_url).raise_for_status()
    port_manager.release()


@pytest.fixture(scope="module")
def client(gauthling_daemon_mock):
    host, port, _ = gauthling_daemon_mock
    return GauthlingClient("{0}:{1}".format(host, port))
