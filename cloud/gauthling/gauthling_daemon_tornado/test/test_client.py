import grpc
import pytest

from gauthling_daemon_tornado import GauthlingClient


# NOTE: These tests should be run against mock server


@pytest.mark.gen_test
def test_ping(client):
    response = yield client.ping()
    assert response


@pytest.mark.gen_test
def test_auth_with_sign(client):
    response = yield client.auth_with_sign(b"token", "signed_string", "signature", "datestamp", "service")
    assert response is not None


@pytest.mark.gen_test
def test_auth(client):
    response = yield client.auth(b"token")
    assert response is not None


@pytest.mark.gen_test
def test_auth_with_amazon_sign(client):
    response = yield client.auth_with_amazon_sign(b"token", "signed_string", "signature", "datestamp",
                                                  "service", "region")
    assert response is not None


@pytest.mark.gen_test
def test_authz(client):
    response = yield client.authz(b"token", {})
    assert response is not None


@pytest.mark.gen_test
def test_error():
    client_w_invalid_endpoint = GauthlingClient("unknown:4284", timeout=0.1)
    with pytest.raises(grpc.RpcError):
        yield client_w_invalid_endpoint.ping()
