"""Test validation"""

from ipaddress import IPv4Address, IPv6Address
import pytest
import schematics.exceptions as schematics_exceptions

from yc_common.models import ModelValidationError
from yc_common.clients.models.grpc.common import QuasiIpAddress
from yc_common.clients.models.target_groups import HttpOptionsPublic, HealthCheckPublic


def test_http_options_validation():
    with pytest.raises(schematics_exceptions.DataError):
        HttpOptionsPublic({"port": 8080, "path": "path_without_slash"}).validate()

    with pytest.raises(schematics_exceptions.DataError):
        HttpOptionsPublic({"port": 8080, "path": 'path = "?a=b"'}).validate()

    with pytest.raises(schematics_exceptions.DataError):
        HttpOptionsPublic({"port": 8080, "path": 'path = "/?a=b"'}).validate()

    with pytest.raises(schematics_exceptions.DataError):
        HttpOptionsPublic({"port": 8080, "path": 'path = "?a=b"'}).validate()

    with pytest.raises(schematics_exceptions.DataError):
        HttpOptionsPublic({"port": 8080, "path": ""}).validate()

    HttpOptionsPublic({"port": 8080, "path": "/"}).validate()
    HttpOptionsPublic({"port": 8080, "path": "/?a=b"}).validate()
    HttpOptionsPublic({"port": 8080, "path": "/path"}).validate()
    HttpOptionsPublic({"port": 8080, "path": "/path?"}).validate()
    HttpOptionsPublic({"port": 8080, "path": "/path?key=value"}).validate()


@pytest.mark.parametrize("params,raises", [
    [{}, True],
    [{"name": "test"}, True],
    [{"name": "test", "tcp_options": {"port": 80}}, False],
])
def test_health_check_validation(params, raises):
    if raises:
        with pytest.raises(ModelValidationError):
            HealthCheckPublic.from_api(params)
    else:
        HealthCheckPublic.from_api(params)


def test_http_options_encoding():
    dirty_query = "/test_path?xxx= =yyy"
    options = HttpOptionsPublic({"port": 8080, "path": dirty_query})
    print(options.path)
    options.validate()
    assert "+%3D" in options.path

    request_splitting_case = "/test_path? HTTP/1.1\r\ntest=\r\n\r\nGET / HTTP/1.1\r\nHost:test.com\r\n\r\nGET /"
    options = HttpOptionsPublic({"port": 8080, "path": request_splitting_case})
    options.validate()
    assert "%" in options.path


@pytest.mark.parametrize("ipv4,vrf,quasi", [
    [IPv4Address("127.0.0.1"), 1, IPv6Address("fc00::100:0:7f00:1")],
    [IPv4Address("127.0.0.1"), 2, IPv6Address("fc00::200:0:7f00:1")],
    [IPv4Address("127.0.0.1"), 1000, IPv6Address("fc00::e803:0:7f00:1")],
])
def test_quasip(ipv4, vrf, quasi):
    result = QuasiIpAddress.to_quasi_ipv6(ipv4, vrf)
    assert result == quasi

    _ipv4, _vrf = QuasiIpAddress.from_quasi_ipv6(quasi)
    assert _ipv4 == ipv4
    assert _vrf == vrf
