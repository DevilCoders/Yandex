"""Test QuasiIpAddress"""

from ipaddress import IPv4Address, IPv6Address
import pytest

from yc_common.clients.models.grpc.common import QuasiIpAddress
from schematics.exceptions import DataError


def test_ctor_empty_invalid():
    qip = QuasiIpAddress()
    with pytest.raises(DataError):
        qip.validate()


def test_ctor_only_ipv4_invalid():
    qip = QuasiIpAddress({"ip": "10.0.1.1"})
    with pytest.raises(DataError):
        qip.validate()


def test_ctor_only_ipv6_valid():
    qip = QuasiIpAddress({"ip": "fe80::1"})
    qip.validate()


def test_ctor_only_vrf_invalid():
    qip = QuasiIpAddress({"vrf": 111})
    with pytest.raises(DataError):
        qip.validate()


def test_ctor_ipv4_and_vrf_valid():
    qip = QuasiIpAddress({"ip": "10.0.1.1", "vrf": 111})
    qip.validate()


def test_ctor_bad_ip_with_vrf():
    with pytest.raises(DataError):
        qip = QuasiIpAddress({"ip": "abcd", "vrf": 111})
        qip.validate()


def test_ctor_bad_ip_without_vrf():
    with pytest.raises(DataError):
        qip = QuasiIpAddress({"ip": "abcd"})
        qip.validate()


def test_ipv4_and_vrf_to_quasipv6():
    qip = QuasiIpAddress({"ip": "10.0.1.1", "vrf": 111})
    assert qip.ipv6 == "fc00::6f00:0:a00:101"
    assert qip.to_grpc() == "fc00::6f00:0:a00:101"


def test_ipv4_and_vrf_0_to_quasipv6():
    qip = QuasiIpAddress({"ip": "10.0.1.1", "vrf": 0})
    assert qip.ipv6 == "fc00::a00:101"
    assert qip.to_grpc() == "fc00::a00:101"


def test_ipv4_and_vrf_max_to_quasipv6():
    qip = QuasiIpAddress({"ip": "10.0.1.1", "vrf": 0xffffffff})
    assert qip.ipv6 == "fc00::ffff:ffff:a00:101"
    assert qip.to_grpc() == "fc00::ffff:ffff:a00:101"


def test_quasipv6_to_ipv4_and_vrf():
    qip = QuasiIpAddress.from_grpc("fc00::6f00:0:a00:101")
    assert qip.ip == IPv4Address("10.0.1.1")
    assert qip.vrf == 111


def test_ipv6_to_quasipv6():
    qip = QuasiIpAddress({"ip": "2a02::1"})
    assert qip.ipv6 == "2a02::1"
    assert qip.to_grpc() == "2a02::1"


def test_quasipv6_to_ipv6():
    qip = QuasiIpAddress.from_grpc("2a02::1")
    assert qip.ip == IPv6Address("2a02::1")
    assert qip.vrf is None
