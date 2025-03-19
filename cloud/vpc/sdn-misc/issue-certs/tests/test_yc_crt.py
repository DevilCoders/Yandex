
import os
import json

import responses
import pytest
from unittest.mock import patch

from yc_issue_cert.cluster import HostGroup
from yc_issue_cert.config import YcCrtConfig, YcCrtProfile
from yc_issue_cert.yc_crt import get_or_create_certs

CERTINFO_REVOKED = {
    "url": "http://localhost/api/certificate/1",
    "revoked": "today",
    "hosts": ["oct-head1.test"],
    "download2": "http://localhost/api/certificate/1/cert.pem",
}
CERTINFO1 = {
    "url": "http://localhost/api/certificate/1",
    "revoked": None,
    "hosts": ["oct-head1.test"],
    "download2": "http://localhost/api/certificate/1/cert.pem",
}
CERTINFO2 = {
    "url": "http://localhost/api/certificate/1",
    "revoked": None,
    "hosts": ["oct-head1.test"],
    "download2": "http://localhost/api/certificate/1/cert.pem",
}


@pytest.fixture(scope="module", autouse=True)
def yc_crt_patch():
    os.environ["YC_CRT_TOKEN"] = "X"


@pytest.fixture
def yc_crt_config():
    profile = YcCrtProfile(dict(endpoint="http://localhost", ca_type="ca",
                                ca_name="yc_issue_cert Test CA"))
    return YcCrtConfig(dict(profiles=dict(test=profile),
                            desired_ttl_days=30, abc_service=999999999))


@pytest.fixture
def fake_cluster(cluster):
    with patch.object(cluster, "iter_hosts", return_value=[HostGroup("oct-head1.test", "oct-head")]):
        yield cluster


@pytest.mark.parametrize("results", [
    [CERTINFO2],
    [CERTINFO_REVOKED, CERTINFO2],
])
@responses.activate
def test_yc_crt_reuse1(results, yc_crt_config, fake_cluster, raw_certificate):
    responses.add(responses.GET, "http://localhost/api/certificate?host=oct-head1.test",
                  json={"count": len(results), "certs": results})
    responses.add(responses.GET, CERTINFO2["download2"], body=b"".join(raw_certificate))

    certs = get_or_create_certs(yc_crt_config, fake_cluster.iter_hosts(["oct-head"]), "test")
    assert len(certs) == 1

    cert0 = certs[0]
    assert cert0.host == "oct-head1.test"
    assert cert0.shortname_sanitized == "oct-head1"

    assert cert0.get_host_cert() == raw_certificate[0]
    assert cert0.get_ycloud_ca_cert() == raw_certificate[1]
    assert cert0.get_private_key() == raw_certificate[2]


@responses.activate
def test_yc_crt_reuse3(yc_crt_config, cluster, raw_certificate):
    for idx in range(1, 4):
        host = "oct-sas{}.hw99.cloud-lab.yandex.net".format(idx)
        dl_url = "http://localhost/api/certificate/{}/cert.pem".format(idx)
        certinfo = {
            "url": "http://localhost/api/certificate/{}".format(idx),
            "revoked": None,
            "hosts": [host],
            "download2": dl_url,
        }

        responses.add(responses.GET, "http://localhost/api/certificate?host={}".format(host),
                      json={"count": 1, "certs": [certinfo]})
        responses.add(responses.GET, dl_url, body=b"".join(raw_certificate))

    certs = get_or_create_certs(yc_crt_config, cluster.iter_hosts(["oct-head"]), "test")
    assert len(certs) == 3

    cert0 = certs[0]
    assert cert0.host == "oct-sas1.hw99.cloud-lab.yandex.net"
    assert cert0.shortname_sanitized == "oct-sas1"


@pytest.mark.parametrize("results, reissue, revoke, all_hosts_cert", [
    ([],          False, False, False),
    ([CERTINFO1], True, False, False),
    ([CERTINFO1], True, False, False),
    ([],          False, False, True),
])
@responses.activate
def test_yc_crt_issue(results, reissue, revoke, all_hosts_cert, yc_crt_config, fake_cluster, raw_certificate):
    def issue_cert(request):
        nonlocal issued_cert
        issued_cert = True

        return 200, {}, json.dumps(CERTINFO2)

    responses.add(responses.GET, "http://localhost/api/certificate?host=oct-head1.test",
                  json={"count": len(results), "certs": results})
    responses.add(responses.GET, CERTINFO2["download2"], body=b"".join(raw_certificate))
    responses.add_callback(responses.POST, "http://localhost/api/certificate", callback=issue_cert,
                           content_type="application/json")
    if revoke:
        responses.add(responses.DELETE, CERTINFO1["url"], json={})

    issued_cert = False
    certs = get_or_create_certs(yc_crt_config, fake_cluster.iter_hosts(["oct-head"]), "test",
                                reissue=reissue, revoke=revoke, all_hosts_cert=all_hosts_cert)

    assert issued_cert

    cert0 = certs[0]
    assert cert0.host == "oct-head1.test"
    assert cert0.shortname_sanitized == "oct-head1"


@pytest.mark.parametrize("revoke_expired", [
    False,
    True
])
@responses.activate
def test_revoke_expired(revoke_expired, yc_crt_config, fake_cluster, raw_expired_certificate):
    def issue_cert(request):
        nonlocal issued_cert
        issued_cert = True

        return 200, {}, json.dumps(CERTINFO2)

    def revoke_cert(request):
        nonlocal revoked_cert
        revoked_cert = True

        return 200, {}, ''

    responses.add(responses.GET, "http://localhost/api/certificate?host=oct-head1.test",
                            json={"count": 1, "certs": [CERTINFO1]})
    if revoke_expired:
        # after revoke need to modify count of certificates
        responses.add(responses.GET, "http://localhost/api/certificate?host=oct-head1.test",
                                json={"count": 0, "certs": []})

        responses.add_callback(responses.DELETE, CERTINFO1["url"], callback=revoke_cert)

        responses.add_callback(responses.POST, "http://localhost/api/certificate", callback=issue_cert,
                                         content_type="application/json")
    responses.add(responses.GET, CERTINFO1["download2"], body=b"".join(raw_expired_certificate))

    issued_cert = False
    revoked_cert = False
    certs = get_or_create_certs(yc_crt_config, fake_cluster.iter_hosts(["oct-head"]), "test",
                                revoke_expired=revoke_expired)
    if revoke_expired:
        assert revoked_cert
        assert issued_cert
    else:
        assert not issued_cert
        assert not revoked_cert

    cert0 = certs[0]
    assert cert0.host == "oct-head1.test"
    assert cert0.shortname_sanitized == "oct-head1"
