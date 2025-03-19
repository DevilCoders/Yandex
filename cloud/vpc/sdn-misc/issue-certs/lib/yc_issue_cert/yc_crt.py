from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from datetime import datetime
import logging
import itertools
import operator
import os
import pem
import requests
from typing import List, Dict, Optional, Iterable
import urllib.parse as urlparse

from yc_issue_cert.cluster import HostGroup
from yc_issue_cert.config import YcCrtConfig

YANDEX_CA_COMMON_NAME = "YandexInternalCA"
YANDEX_CA_URL = "https://crls.yandex.net/YandexInternalRootCA.crt"


log = logging.getLogger("yc-crt")


class YcCrtClient:
    def __init__(self, yc_crt_config: YcCrtConfig, profile: str):
        self.config = yc_crt_config
        try:
            self.profile = yc_crt_config.profiles[profile]
        except KeyError:
            raise RuntimeError("Invalid YC CRT profile {!r} is specified".format(profile))

        token = os.environ.get("YC_CRT_TOKEN")
        if not token:
            raise RuntimeError("Please specify YC_CRT_TOKEN= environment variable")

        self.url = urlparse.urljoin(self.profile.endpoint, "/api/certificate")
        self.auth_headers = {"Authorization": "OAuth {}".format(token)}

    def find(self, host) -> Optional[Dict]:
        r = requests.get(self.url, headers=self.auth_headers, params={"host": host})

        if not r.json()["count"]:
            return None

        matched_certinfo = None
        for certinfo in r.json()["certs"]:
            if certinfo["revoked"]:
                log.debug("Certificate %s is revoked", certinfo["url"])
                continue

            if len(certinfo["hosts"]) > 1:
                log.warning("Certificate %s is valid for multiple hosts: %s, skipping",
                            certinfo["url"], certinfo["hosts"])
                continue

            if host in certinfo["hosts"]:
                if matched_certinfo:
                    log.warning("More than one certificate found for host %s: %s",
                                host, certinfo["url"])
                else:
                    matched_certinfo = certinfo

        return matched_certinfo

    def issue(self, host):
        request = {
            "abc_service": self.config.abc_service,
            "type": self.profile.ca_type,
            "hosts": host,
            "desired_ttl_days": self.config.desired_ttl_days,
        }
        r = requests.post(self.url, headers=self.auth_headers, json=request)
        if r.status_code >= 400:
            log.error("YC CRT returns %s: %r", r.status_code, r.json())
            raise RuntimeError("Error issuing certificate {!r}!".format(host))

        certinfo = r.json()
        log.info("Issued certificate %s", certinfo["url"])
        return certinfo

    def revoke(self, certinfo):
        requests.delete(certinfo["url"], headers=self.auth_headers)
        log.info("Revoked certificate %s", certinfo["url"])
        return None

    def download(self, certinfo):
        r = requests.get(certinfo["download2"], headers=self.auth_headers)
        return {**certinfo, "certificate": bytes(r.text, encoding="ascii")}


class Certificate:
    def __init__(self, certinfo: Dict, ca_name: str):
        self.certinfo = certinfo
        self.ca_name = ca_name
        self.certdata = pem.parse(certinfo["certificate"])

        self.__host_cert = None

    @property
    def host(self):
        return self.certinfo["hosts"][0]

    @property
    def host_sanitized(self):
        return self.host.replace("*", "_")

    @property
    def shortname_sanitized(self):
        return self.host_sanitized.split(".")[0]

    @property
    def not_until(self) -> datetime:
        return self._host_cert.not_valid_after

    @property
    def fingerprints(self) -> Dict[str, str]:
        return {
            "sha1": self._fingerprint(hashes.SHA1()),
            "sha256": self._fingerprint(hashes.SHA256())
        }

    def _fingerprint(self, algorithm) -> str:
        fingerprint = self._host_cert.fingerprint(algorithm)
        return "".join("{:02X}".format(byte) for byte in fingerprint)

    @property
    def _host_cert(self):
        if self.__host_cert is None:
            self.__host_cert = x509.load_pem_x509_certificate(self.get_host_cert(),
                                                              default_backend())

        return self.__host_cert

    def _get_cert(self, index, expected_name) -> bytes:
        pem_obj = self.certdata[index]
        if not isinstance(pem_obj, pem.Certificate):
            log.debug("Unexpected pem object of type %s: `%s` while expected `%s`", type(pem_obj), pem_obj,
                      pem.Certificate)
            raise RuntimeError("Error in certificate for {!r}: section #{:d} is not a certificate.".format(
                self.host, index))

        raw_cert = bytes(str(pem_obj), encoding="ascii")
        cert = x509.load_pem_x509_certificate(raw_cert, default_backend())
        common_names = cert.subject.get_attributes_for_oid(NameOID.COMMON_NAME)
        if len(common_names) != 1:
            raise RuntimeError("Error in certificate for {!r}: cannot get common name.".format(
                self.host))

        common_name = common_names[0].value
        if common_name != expected_name:
            raise RuntimeError(("Unexpected PEM section #{:d}:"
                                " expected {!r}, got {!r}").format(index, expected_name, common_name))

        return raw_cert

    def get_host_cert(self) -> bytes:
        return self._get_cert(0, self.host)

    def get_ycloud_ca_cert(self) -> bytes:
        # Old certificates have empty section #2 and intermediate CA
        # in section #1. New certificates have intermediate CA in #2
        # and internal root CA in #1
        try:
            return self._get_cert(1, self.ca_name)
        except RuntimeError:
            return self._get_cert(2, self.ca_name)

    def get_yaint_ca_cert(self) -> bytes:
        try:
            self._get_cert(1, YANDEX_CA_COMMON_NAME)
        except RuntimeError:
            r = requests.get(YANDEX_CA_URL)
            return bytes(r.text, encoding="ascii")

    def get_private_key(self) -> bytes:
        pem_obj = self.certdata[-1]
        if not isinstance(pem_obj, pem.RSAPrivateKey):
            raise RuntimeError("Error in certificate for {!r}: last section is not a private key.".format(
                self.host))

        return bytes(str(pem_obj), encoding="ascii")

    def get_pkcs8_private_key(self) -> bytes:
        pem_pk = self.get_private_key()
        pk = serialization.load_pem_private_key(pem_pk, None, default_backend())
        return pk.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        )


def get_or_create_certs(yc_crt_config: YcCrtConfig, hosts: Iterable[HostGroup],
                        profile: str, reissue=False, revoke=False, revoke_expired=False, all_hosts_cert=False) -> List[Certificate]:
    yc_crt = YcCrtClient(yc_crt_config, profile)
    if all_hosts_cert:
        host_key = operator.attrgetter("group")
        hosts = [(",".join(host.host for host in group_hosts), group) for group, group_hosts
                 in itertools.groupby(sorted(hosts, key=host_key), host_key)]

    certificates = []
    now = datetime.today()
    for host, _ in hosts:
        certinfo = yc_crt.find(host)

        issue_new = True
        while certinfo and revoke_expired \
           and Certificate(yc_crt.download(certinfo), yc_crt.profile.ca_name).not_until < now:
            yc_crt.revoke(certinfo)
            certinfo = yc_crt.find(host)

        if certinfo and not reissue:
            log.info("Reusing certificate %s (%r)", certinfo["url"], certinfo)
            issue_new = False
            if revoke:
                certinfo = yc_crt.revoke(certinfo)
                continue
        elif certinfo and revoke:
            certinfo = yc_crt.revoke(certinfo)

        if issue_new:
            certinfo = yc_crt.issue(host)

        certinfo = yc_crt.download(certinfo)
        certificates.append(Certificate(certinfo, yc_crt.profile.ca_name))

    return certificates
