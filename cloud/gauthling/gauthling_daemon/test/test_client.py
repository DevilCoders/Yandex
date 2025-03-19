import grpc
import pytest

from gauthling_daemon import GauthlingClient


# NOTE: These tests should be run against mock server


def test_ping(client):
    assert client.ping()


def test_auth(client):
    response = client.auth(b"token")
    assert response is not None


def test_auth_with_sign(client):
    response = client.auth_with_sign(b"token", "signed_string", "signature", "datestamp", "service")
    assert response is not None


def test_auth_with_amazon_sign(client):
    response = client.auth_with_amazon_sign(b"token", "signed_string", "signature", "datestamp", "service", "region")
    assert response is not None


def test_authz(client):
    response = client.authz(b"token", {})
    assert response is not None


def test_error():
    client_w_invalid_endpoint = GauthlingClient("unknown:4284", timeout=0.1)
    with pytest.raises(grpc.RpcError):
        client_w_invalid_endpoint.ping()
