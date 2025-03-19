
from cryptography import x509
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
import datetime
import uuid

import pytest
from unittest.mock import patch

from yc_issue_cert.cluster import Cluster
from yc_issue_cert.config import ClusterConfig
from yc_issue_cert.yc_crt import Certificate


@pytest.fixture(scope="session", autouse=True)
def disable_side_effects():
    import subprocess       # noqa
    import requests         # noqa

    disabled = Exception("Side effects are disabled in unit tests")

    with patch("subprocess.run", side_effect=disabled), \
            patch("subprocess.Popen", side_effect=disabled), \
            patch("subprocess.check_call", side_effect=disabled), \
            patch("subprocess.check_output", side_effect=disabled), \
            patch("requests.adapters.HTTPAdapter.send", side_effect=disabled):
        yield


@pytest.fixture
def cluster():
    config = ClusterConfig({
        "secret_profile": "testing",
        "bootstrap_stand": "dev",
        "hosts": {
            "oct-head": [
                "oct-sas1.hw99.cloud-lab.yandex.net",
                "oct-sas2.hw99.cloud-lab.yandex.net",
                "oct-sas3.hw99.cloud-lab.yandex.net",
            ],
            "head": [
                "head-sas1.hw99.cloud-lab.yandex.net",
            ]
        }
    })
    return Cluster("ru-lab1-z@hw99-lab", config, None, None)


@pytest.fixture(scope="session")
def raw_certificate():
    """Issues one self-signed certificate for oct-head1.cloud-lab.yandex.net
    and returns own cert, CA cert and private key as bytes, just like
    YC CRT does it (though it doesn't merge it)

    From: https://gist.github.com/major/8ac9f98ae8b07f46b208"""
    ca_name = "yc_issue_cert Test CA"
    ca_private_key, ca_certificate = _certificate(ca_name, ca_name)
    private_key, certificate = _certificate("oct-head1.test", ca_name, ca_private_key)

    return (_serialize_cert(certificate),
            _serialize_cert(ca_certificate),
            _serialize_private_key(private_key))


@pytest.fixture(scope="session")
def raw_expired_certificate():
    """Issues one expired self-signed certificate for oct-head1.cloud-lab.yandex.net
    and returns own cert, CA cert and private key as bytes, just like
    YC CRT does it (though it doesn't merge it)

    From: https://gist.github.com/major/8ac9f98ae8b07f46b208"""
    ca_name = "yc_issue_cert Test CA"
    not_valid_before = datetime.datetime.today() - datetime.timedelta(30, 0, 0)
    not_valid_after = datetime.datetime.today() - datetime.timedelta(1, 0, 0)
    ca_private_key, ca_certificate = _certificate(ca_name, ca_name)
    private_key, certificate = _certificate("oct-head1.test", ca_name, ca_private_key,
                                            not_valid_before=not_valid_before,
                                            not_valid_after=not_valid_after)

    return (_serialize_cert(certificate),
            _serialize_cert(ca_certificate),
            _serialize_private_key(private_key))


@pytest.fixture(scope="session")
def certificate(raw_certificate):
    return Certificate({
        "url": "http://localhost/certificate/100",
        "certificate": b"".join(raw_certificate)
    })


@pytest.fixture
def ca_cert_path(tmpdir, raw_certificate):
    path = str(tmpdir / "ca.crt")
    _, ca_raw_certificate, _ = raw_certificate
    with open(path, "wb") as ca_file:
        ca_file.write(ca_raw_certificate)

    return path


@pytest.fixture
def api_cert_path(tmpdir, raw_certificate):
    path = str(tmpdir / "oct-head1.crt")
    api_raw_certificate, _, _ = raw_certificate
    with open(path, "wb") as api_file:
        api_file.write(api_raw_certificate)

    return path


def _certificate(common_name, issuer_name, sign_key=None, not_valid_before=None, not_valid_after=None):
    if not_valid_before is None:
        not_valid_before = datetime.datetime.today() - datetime.timedelta(1, 0, 0)
    if not_valid_after is None:
        not_valid_after = datetime.datetime.today() + datetime.timedelta(30, 0, 0)
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
        backend=default_backend()
    )
    public_key = private_key.public_key()

    builder = (
        x509.CertificateBuilder()
        .subject_name(x509.Name([
            x509.NameAttribute(NameOID.COMMON_NAME, common_name),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Yandex Cloud"),
            x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, "YC VPC VN"),
        ]))
        .issuer_name(x509.Name([
            x509.NameAttribute(NameOID.COMMON_NAME, issuer_name),
        ]))
        .not_valid_before(not_valid_before)
        .not_valid_after(not_valid_after)
        .serial_number(int(uuid.uuid4()))
        .public_key(public_key))

    sign_key = sign_key or private_key
    certificate = builder.sign(
        private_key=sign_key, algorithm=hashes.SHA256(),
        backend=default_backend()
    )

    return private_key, certificate


def _serialize_cert(certificate):
    return certificate.public_bytes(
        encoding=serialization.Encoding.PEM,
    )


def _serialize_private_key(private_key):
    return private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.BestAvailableEncryption(b"yc_issue_cert-test-enc")
    )
