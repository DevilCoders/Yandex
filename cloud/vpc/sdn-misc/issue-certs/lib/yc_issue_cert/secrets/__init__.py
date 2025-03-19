
import base64
from datetime import datetime
from collections import namedtuple
import hashlib
import os
import logging

from typing import List, Dict, Optional, Union, Tuple, Iterable

from yc_issue_cert.config import SecretConfig, SecretGroupConfig
from yc_issue_cert.cluster import Cluster
from yc_issue_cert.utils import InternalError, run
from yc_issue_cert.yc_crt import Certificate


log = logging.getLogger("secrets")


class BaseRole:
    CLOUDVM = "cloudvm"
    OCT_HEAD = "oct-head"

    LB_NODE = "loadbalancer-node"
    LB_CTRL = "loadbalancer-ctrl"
    HC_NODE = "healthcheck-node"
    HC_CTRL = "healthcheck-ctrl"

    OCT_HEAD_ROLES = [OCT_HEAD]
    YLB_ROLES = [LB_NODE, LB_CTRL, HC_NODE, HC_CTRL]


class SecretData:
    USER, GROUP = None, None
    SERVICES = []
    DEPENDS = []
    BASE_ROLES = None
    PATH = None
    NEED_FINGERPRINTS = False
    NEED_CERTIFICATE_COMMON_NAME = False

    def __init__(self, name: Union[str, Tuple], srcpath: str, dstpath: str,
                 bootstrap_filter: Dict, expire: Optional[datetime] = None,
                 compare_by_expire=False, ss_hostgroups: List[Tuple[str, str]] = None,
                 fingerprints: Optional[Dict[str, str]] = None,
                 certificate_common_name: Optional[str] = None,
                 scope: Optional[str] = None):
        self.name = name
        if not isinstance(name, str):
            self.name = "-".join(filter(bool, self.name))

        self.srcpath = srcpath
        self.dstpath = dstpath
        self.scope = scope
        self.bootstrap_filter = bootstrap_filter

        self.expire = expire
        self.compare_by_expire = compare_by_expire
        self.ss_hostgroups = ss_hostgroups or []

        self.fingerprints = None
        if self.NEED_FINGERPRINTS:
            self.fingerprints = fingerprints
        self.certificate_common_name = None
        if self.NEED_CERTIFICATE_COMMON_NAME:
            self.certificate_common_name = certificate_common_name

        if os.path.basename(srcpath) != self.filename:
            raise InternalError(("Source basename {} should match"
                                 " the one in Secret Service ({})").format(srcpath, dstpath))

    @property
    def filename(self):
        # For whatever reason, SS uses source path passed into
        # yc-secret-cli secret add as file name internally
        return os.path.basename(self.srcpath)

    def get_sha256_b64(self) -> str:
        with open(self.srcpath, "rb") as secret_file:
            secret_text = secret_file.read()

        digest = hashlib.sha256()
        digest.update(secret_text)
        return str(base64.encodebytes(digest.digest()).strip(),
                   encoding="ascii")

    @classmethod
    def _save_file(cls, pem_text, dstdir, fnamefmt, *args, **kwargs) -> str:
        path = os.path.join(dstdir, fnamefmt.format(*args, **kwargs))
        if pem_text is None:
            if os.path.exists(path):
                os.remove(path)
            return path

        if isinstance(pem_text, str):
            pem_text = bytes(pem_text, encoding="ascii")

        with open(path, "wb") as pem_file:
            pem_file.write(pem_text)

        return path

    @classmethod
    def _save_host_cert(cls, certificate: Certificate, dstdir: str):
        return cls._save_file(certificate.get_host_cert(),
                              dstdir, "{}.crt", certificate.host_sanitized)

    @classmethod
    def _save_ycloud_ca_cert(cls, certificate: Certificate, dstdir: str):
        return cls._save_file(certificate.get_ycloud_ca_cert(),
                              dstdir, "ycloud.crt")

    @classmethod
    def _save_combined_host_cert(cls, certificate: Certificate, dstdir: str):
        return cls._save_file(certificate.get_host_cert() +
                              certificate.get_ycloud_ca_cert(),
                              dstdir, "{}.crt", certificate.host_sanitized)

    @classmethod
    def _save_private_key(cls, certificate: Certificate, dstdir: str):
        return cls._save_file(certificate.get_private_key(),
                              dstdir, "{}.key", certificate.host_sanitized)

    @classmethod
    def _save_pem(cls, certificate: Certificate, dstdir: str):
        return cls._save_file(certificate.get_host_cert() +
                              certificate.get_ycloud_ca_cert() +
                              certificate.get_private_key(),
                              dstdir, "{}.pem", certificate.host_sanitized)

    @classmethod
    def _save_pkcs12(cls, certificate: Certificate, dstdir: str, passphrase: str = ""):
        cert_path = cls._save_host_cert(certificate, dstdir)
        key_path = cls._save_private_key(certificate, dstdir)
        p12_path = cls._save_file(None, dstdir, "{}.p12", certificate.host)

        run("openssl", "pkcs12", export=True, _in=cert_path, inkey=key_path,
            name=certificate.host, out=p12_path, passout="pass:{}".format(passphrase))

        return p12_path

    @classmethod
    def _create_bootstrap_filter(cls, factory: "SecretFactory", config: SecretConfig,
                                 host: Optional[str] = None):
        """Generate bootstrap filter for base_role for which certificate
        is destined. If base role passed in --base-role is different (usually,
        a cloudvm), add it to filter too. zone_id is for multi-az stands.
        """
        if config.scope == "HOST":
            if not host:
                raise RuntimeError("Secret {!s} has scope HOST, but factory didn't supply host.".format(config.name))

            if host and not factory.cluster.config.is_cloudvm:
                return {"hosts": [host]}

        role_filter = cls._create_bootstrap_filter_roles(factory, config, factory.secret_group.base_roles)
        if factory.cluster.config.zone_id:
            role_filter["zones"] = [factory.cluster.config.zone_id]

        return role_filter

    @classmethod
    def _create_bootstrap_filter_roles(cls, factory: "SecretFactory", config: SecretConfig,
                                       base_roles: List[str]):
        role_filter = {"roles": base_roles[:]}
        if factory.cluster.config.is_cloudvm:
            role_filter["roles"].insert(0, BaseRole.CLOUDVM)

        return role_filter

    @classmethod
    def validate(cls, factory, config: SecretConfig):
        pass

    @classmethod
    def create(cls, ctx, config: SecretConfig) -> List["SecretData"]:
        raise NotImplementedError

    @classmethod
    def _create(cls, ctx, config: SecretConfig, name: str, srcpath: str, **kwargs) -> "SecretData":
        factory = ctx.factory
        if "dstpath" not in kwargs:
            kwargs["dstpath"] = cls.PATH
        if "bootstrap_filter" not in kwargs:
            if config.scope != "ROLE":
                raise InternalError("Cannot fill default bootstrap_filter for secret {!r}".format(name))

            kwargs["bootstrap_filter"] = cls._create_bootstrap_filter(ctx.factory, config)
        if "ss_hostgroups" not in kwargs:
            base_roles = factory.secret_group.base_roles
            if not base_roles:
                raise InternalError("Cannot fill default ss_hostgroups for secret {!r}".format(name))

            kwargs["ss_hostgroups"] = list(factory.cluster.iter_ss_hostgroups(base_roles))

        certificate = kwargs.pop("certificate", None)
        if certificate:
            log.debug("Certificate %s until: %s, path %s", name, certificate.not_until, srcpath)
            kwargs.setdefault("expire", certificate.not_until)
            kwargs.setdefault("fingerprints", certificate.fingerprints)
            certificate_hosts = certificate.certinfo.get("hosts", [])
            if len(certificate_hosts) > 0:
                kwargs.setdefault("certificate_common_name", certificate_hosts[0])

        return cls(name, srcpath, **kwargs)


class SecretFactory:
    SECRET_TYPES = {}

    Context = namedtuple("SecretFactoryContext", ["factory", "dstdir", "certificates"])

    @classmethod
    def register(cls, name):
        def decorator(secret_klass):
            cls.SECRET_TYPES[name] = secret_klass
            return secret_klass
        return decorator

    def __init__(self, cluster: Cluster, secret_group: SecretGroupConfig,
                 options: Dict[str, str], skip_secret_types: List[str],
                 only_secret_types: Optional[List[str]]):
        self.cluster = cluster
        self.secret_group = secret_group
        self.options = options
        self.skip_secret_types = skip_secret_types
        self.only_secret_types = only_secret_types

    def validate(self):
        for config in self._iter_secret_group_secrets():
            secret_klass = self.SECRET_TYPES[config.type]
            secret_klass.validate(self, config)

            if secret_klass.BASE_ROLES and self.secret_group.base_roles and \
                    secret_klass.BASE_ROLES != self.secret_group.base_roles:
                raise RuntimeError(("Cannot use secret {!r} for current secret group:"
                                    " it can only be used for base roles {!r},"
                                    " {!r} are in config.").format(config.type, secret_klass.BASE_ROLES,
                                                                   self.secret_group.base_roles))

    def create(self, dstdir: str, certificates: List[Certificate]) -> List[SecretData]:
        ctx = SecretFactory.Context(self, dstdir, certificates)

        secret_data = []
        for config in self._iter_secret_group_secrets():
            secret_klass = self.SECRET_TYPES[config.type]
            secret_data.extend(secret_klass.create(ctx, config))

        return secret_data

    def _iter_secret_group_secrets(self):
        for config in self.secret_group.secrets:
            if config.type in self.skip_secret_types:
                continue
            if self.only_secret_types is not None and config.type not in self.only_secret_types:
                continue

            yield config


class SecretDataHostCertsMixin:
    NAME_PREFIX = None
    NAME_SUFFIX = None
    COMPARE_BY_EXPIRE = False
    USE_SCOPE = True

    @classmethod
    def get_dep_names(cls, certificates: List[Certificate]):
        return []

    @classmethod
    def create(cls, ctx: SecretFactory.Context, config: SecretConfig) -> List[SecretData]:
        secrets = []
        for certificate in ctx.certificates:
            bootstrap_filter = cls._create_bootstrap_filter(ctx.factory, config, certificate.host)
            name = (cls.NAME_PREFIX, certificate.shortname_sanitized, cls.NAME_SUFFIX)
            path = cls.create_one(ctx.dstdir, certificate)
            scope = certificate.host if cls.USE_SCOPE else None

            secret = cls._create(ctx, config, name, path, bootstrap_filter=bootstrap_filter,
                                 certificate=certificate, compare_by_expire=cls.COMPARE_BY_EXPIRE,
                                 scope=scope)
            secrets.append(secret)

        return secrets

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        raise NotImplementedError


class ServiceCertificate(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    NAME_SUFFIX = "cert"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        return cls._save_host_cert(certificate, dstdir)


class ServicePemCertificate(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    NAME_SUFFIX = "pem"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        return cls._save_pem(certificate, dstdir)


class ServicePkcs8PemCertificate(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    NAME_SUFFIX = "pem"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        pem = certificate.get_host_cert()
        pem += certificate.get_ycloud_ca_cert()
        pem += certificate.get_pkcs8_private_key()
        return cls._save_file(pem, dstdir, "{}.pem", certificate.host_sanitized)


class ServicePrivateKey(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    NAME_SUFFIX = "key"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        return cls._save_private_key(certificate, dstdir)


class CaCertificate(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    NAME_SUFFIX = "ca"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        return cls._save_ycloud_ca_cert(certificate, dstdir)


class ServiceCertificateCombined(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        return cls._save_combined_host_cert(certificate, dstdir)


def create_secret_factory(cluster: Cluster, secret_group: SecretGroupConfig,
                          options: Dict[str, str], skip_secret_types: List[str],
                          only_secret_types: Optional[List[str]]) -> SecretFactory:
    secret_factory = SecretFactory(cluster, secret_group, options, skip_secret_types, only_secret_types)
    secret_factory.validate()
    return secret_factory


def iter_hosts(cluster: Cluster, secret_group: SecretGroupConfig) -> Iterable[Tuple[str, str]]:
    if secret_group.base_roles and secret_group.client_group:
        raise RuntimeError("Secret group cannot include both base roles and clients!")

    if secret_group.base_roles:
        return cluster.iter_hosts(secret_group.base_roles)
    if secret_group.client_group:
        return cluster.iter_clients(secret_group.client_group)

    raise RuntimeError("Secret group should include base roles list or clients!")
