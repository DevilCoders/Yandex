
import os.path
import shutil
import subprocess
import textwrap
from typing import List

from yc_issue_cert.config import SecretConfig
from yc_issue_cert.secrets import SecretData, SecretDataHostCertsMixin, BaseRole, SecretFactory, \
    iter_hosts, ServicePemCertificate
from yc_issue_cert.utils import run, InternalError
from yc_issue_cert.yc_crt import Certificate


KEYTOOL_PASSWORD = "cassandra"


class _ContrailSecretData(SecretData):
    USER, GROUP = "contrail", "contrail"
    SERVICES = ["contrail-api", "contrail-schema", "contrail-discovery"]
    BASE_ROLES = BaseRole.OCT_HEAD_ROLES


class _CassandraSecretData(SecretData):
    USER, GROUP = "cassandra", "cassandra"
    SERVICES = ["cassandra"]
    BASE_ROLES = BaseRole.OCT_HEAD_ROLES


class _RabbitMqSecretData(SecretData):
    USER, GROUP = "root", "rabbitmq"  # default rights on /etc/rabbitmq/ dir
    SERVICES = ["rabbitmq"]
    BASE_ROLES = BaseRole.OCT_HEAD_ROLES


class CombinedCaMixin:
    SECRET_NAME = "ca"

    @classmethod
    def create(cls, ctx: SecretFactory.Context, config: SecretConfig) -> List[SecretData]:
        certificate = next(iter(ctx.certificates))
        ca_path = cls._save_file(certificate.get_ycloud_ca_cert() +
                                 certificate.get_yaint_ca_cert(),
                                 ctx.dstdir, "ca.crt")
        return [cls._create(ctx, config, cls.SECRET_NAME, ca_path)]


class GeneratedPasswordFileMixin:
    SECRET_FILENAME = "contrail-some-name.conf"
    SECRET_NAME = "some-name"
    OPTION_NAME = "password-file"
    PASSWORD_FILE_FORMAT = textwrap.dedent("""\
        {password}
    """)

    @classmethod
    def validate(cls, factory: SecretFactory, config: SecretConfig):
        if cls.OPTION_NAME not in factory.options:
            raise RuntimeError("{} option is not specified!".format(cls.OPTION_NAME))

    @classmethod
    def create(cls, ctx: SecretFactory.Context, config: SecretConfig) -> List[SecretData]:
        password_file_path = ctx.factory.options[cls.OPTION_NAME]
        with open(password_file_path, "r") as password_file:
            password = password_file.read().strip()

        scope = ctx.factory.cluster.config.scope
        content = cls.PASSWORD_FILE_FORMAT.format(password=password)
        password_conf_path = cls._save_file(content, ctx.dstdir, cls.SECRET_FILENAME)
        return [cls._create(ctx, config, cls.SECRET_NAME, password_conf_path,
                            scope=scope)]


class KeytoolDependencyMixin:
    @classmethod
    def validate(cls, factory: SecretFactory, config: SecretConfig):
        try:
            subprocess.check_call(["keytool"], stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            raise RuntimeError("keytool is not installed. Please install JDK.")


@SecretFactory.register("contrail-api-pem")
class ContrailApiPem(ServicePemCertificate, _ContrailSecretData):
    USE_SCOPE = True
    PATH = "/etc/contrail/ssl/api.pem"
    NAME_SUFFIX = "contrail-api"


@SecretFactory.register("contrail-api-v6-pem")
class ContrailApiV6Pem(ServicePemCertificate, _ContrailSecretData):
    USE_SCOPE = True
    PATH = "/etc/contrail/ssl/api-v6.pem"
    NAME_SUFFIX = "contrail-api"
    SERVICES = ["contrail-api"]
    BASE_ROLES = ["ipv6@" + role for role in _ContrailSecretData.BASE_ROLES]


@SecretFactory.register("oct-rabbitmq-key")
class RabbitMqKey(ServicePemCertificate, _RabbitMqSecretData):
    USE_SCOPE = True
    PATH = "/etc/rabbitmq/ssl/server.pem"
    NAME_SUFFIX = "rabbitmq"


@SecretFactory.register("oct-rabbitmq-ca")
class RabbitMqCa(CombinedCaMixin, _RabbitMqSecretData):
    PATH = "/etc/rabbitmq/ssl/cacerts.crt"
    SECRET_NAME = "rabbitmq-ca"


@SecretFactory.register("oct-rabbitmq-cookie")
class RabbitMqCookie(GeneratedPasswordFileMixin, _RabbitMqSecretData):
    PATH = "/var/lib/rabbitmq/.erlang.cookie"
    USER, GROUP = "rabbitmq", "rabbitmq"
    SECRET_FILENAME = ".erlang.cookie"
    SECRET_NAME = "rabbitmq-cookie"
    OPTION_NAME = "rabbitmq-cookie-file"
    PASSWORD_FILE_FORMAT = "{password}"


@SecretFactory.register("oct-rabbitmq-secrets")
class ContrailRabbitMqSecrets(GeneratedPasswordFileMixin, _ContrailSecretData):
    PATH = "/etc/contrail/contrail-rabbitmq-secrets.conf"
    SECRET_FILENAME = "contrail-rabbitmq-secrets.conf"
    SECRET_NAME = "rabbitmq-secrets"
    OPTION_NAME = "rabbitmq-password-file"
    PASSWORD_FILE_FORMAT = textwrap.dedent("""\
        [DEFAULTS]
        rabbit_user=securecontrail
        rabbit_password={password}
    """)


@SecretFactory.register("oct-cloud-ca")
class CloudCa(CombinedCaMixin, _ContrailSecretData):
    PATH = "/etc/contrail/cloud-ca.crt"
    SECRET_NAME = "cloud-ca"


@SecretFactory.register("oct-cassandra-truststore")
class CassandraTrustStore(KeytoolDependencyMixin, _CassandraSecretData):
    PATH = "/etc/cassandra/conf/.truststore"
    OPTION_NAME = "cassandra-existing-truststore"

    @classmethod
    def validate(cls, factory: SecretFactory, config: SecretConfig):
        existing_truststore_path = factory.options.get(cls.OPTION_NAME)
        if existing_truststore_path is not None and not os.path.exists(existing_truststore_path):
            raise RuntimeError("Truststore in {} is not found!".format(existing_truststore_path))

    @classmethod
    def create(cls, ctx: SecretFactory.Context, config: SecretConfig) -> List[SecretData]:
        existing_truststore_path = ctx.factory.options.get(cls.OPTION_NAME)
        truststore_path = os.path.join(ctx.dstdir, "cassandra.truststore")
        if existing_truststore_path is not None:
            shutil.copy(existing_truststore_path, truststore_path)
        elif os.path.exists(truststore_path):
            os.remove(truststore_path)

        for certificate in ctx.certificates:
            cert_path = cls._save_host_cert(certificate, ctx.dstdir)
            alias = certificate.host + "_till_" + certificate.not_until.strftime("%Y.%m.%d")
            run("keytool", _import=True, noprompt=True, trustcacerts=True,
                alias=alias, file=cert_path, keystore=truststore_path,
                storepass=KEYTOOL_PASSWORD)

        # NOTE: we use expire on keystores to monitor certificate expiration
        # we use expire here to determine when new certificate was added and
        # certificate must be bumped in Secret Service, hence max() here
        expire = max(certificate.not_until for certificate in ctx.certificates)
        scope = ctx.factory.cluster.config.scope
        name = ("cassandra", scope, "keystore")
        return [cls._create(ctx, config, name, truststore_path, expire=expire,
                            compare_by_expire=True, scope=scope)]


@SecretFactory.register("oct-cassandra-keystore")
class CassandraKeyStore(SecretDataHostCertsMixin, KeytoolDependencyMixin, _CassandraSecretData):
    PATH = "/etc/cassandra/conf/.keystore"
    NAME_PREFIX = "cassandra"
    NAME_SUFFIX = "truststore"
    COMPARE_BY_EXPIRE = True

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        # keytool doesn't support pem and requires pkcs12. unfortunately,
        # cryptography can't serialize it, so use openssl for it
        p12_path = cls._save_pkcs12(certificate, dstdir, KEYTOOL_PASSWORD)
        keystore_path = cls._save_file(None, dstdir, "{}.keystore", certificate.host)

        run("keytool", importkeystore=True, srckeystore=p12_path, srcstoretype="PKCS12",
            srcstorepass=KEYTOOL_PASSWORD, deststorepass=KEYTOOL_PASSWORD,
            destkeystore=keystore_path)

        return keystore_path


@SecretFactory.register("oct-database-secrets")
class ContrailDatabaseSecrets(GeneratedPasswordFileMixin, _ContrailSecretData):
    PATH = "/etc/contrail/contrail-database-secrets.conf"
    SECRET_FILENAME = "contrail-database-secrets.conf"
    SECRET_NAME = "database-secrets"
    OPTION_NAME = "cassandra-password-file"
    PASSWORD_FILE_FORMAT = textwrap.dedent("""\
        [CASSANDRA]
        cassandra_user=cassandra
        cassandra_password={password}
    """)


class _ContrailClientCertificate(SecretData):
    BASE_ROLES_DEV = []
    NEED_FINGERPRINTS = True
    NEED_CERTIFICATE_COMMON_NAME = True

    @classmethod
    def create(cls, ctx: SecretFactory.Context, config: SecretConfig) -> List[SecretData]:
        hosts_by_subgroup = dict(iter_hosts(ctx.factory.cluster, ctx.factory.secret_group))
        secrets = []
        for certificate in ctx.certificates:
            subgroup = hosts_by_subgroup[certificate.host]
            if subgroup != cls.SUBGROUP:
                continue

            if secrets:
                raise InternalError("More than one certificate for oct clients is not supported")

            base_roles = cls.BASE_ROLES
            if ctx.factory.cluster.config.bootstrap_stand == "dev":
                base_roles += cls.BASE_ROLES_DEV

            bootstrap_filter = cls._create_bootstrap_filter_roles(ctx.factory, config, base_roles)
            ss_hostgroups = list(ctx.factory.cluster.iter_ss_hostgroups(base_roles))
            compare_by_expire = False
            if cls.PATH.endswith(".p12"):
                cert_path = cls._save_pkcs12(certificate, ctx.dstdir)
                compare_by_expire = True
            else:
                cert_path = cls._save_pem(certificate, ctx.dstdir)

            secrets.append(cls._create(ctx, config, ("oct-client", cls.SUBGROUP), cert_path,
                                       bootstrap_filter=bootstrap_filter, ss_hostgroups=ss_hostgroups,
                                       certificate=certificate, compare_by_expire=compare_by_expire))

        return secrets


@SecretFactory.register("oct-client-head")
class ComputeApiClientCertificate(_ContrailClientCertificate):
    SUBGROUP = "head"
    PATH = "/etc/yc/compute/head-oct-client.pem"
    USER = "yc-compute"
    GROUP = "yc-compute"
    BASE_ROLES = ["head"]


@SecretFactory.register("oct-client-vpc-api")
class VpcApiClientCertificate(_ContrailClientCertificate):
    SUBGROUP = "vpc-api"
    PATH = "/etc/yc/vpc-config-plane/vpc-api-oct-client.p12"
    COMPARE_BY_EXPIRE = True
    USER = "yc-vpc-config-plane"
    GROUP = "yc-vpc-config-plane"
    BASE_ROLES = ["vpc-api"]
    BASE_ROLES_DEV = ["head"]


@SecretFactory.register("oct-client-e2e")
class E2eClientCertificate(_ContrailClientCertificate):
    SUBGROUP = "e2e"
    PATH = "/etc/yc/e2e-tests/e2e-oct-client.pem"
    USER = "root"
    GROUP = "root"
    BASE_ROLES = ["compute"]
    BASE_ROLES_DEV = ["compute-node-vm"]


@SecretFactory.register("oct-client-provision")
class ProvisionClientCertificate(_ContrailClientCertificate):
    SUBGROUP = "provision"
    PATH = "/etc/contrail/provision-oct-client.pem"
    USER = "root"
    GROUP = "root"
    BASE_ROLES = ["compute", "cgw", "cgw-dc", "cgw-ipv4", "cgw-ipv6", "cgw-nat",
                  "loadbalancer-node"] + BaseRole.OCT_HEAD_ROLES
    BASE_ROLES_DEV = ["compute-node-vm"]


@SecretFactory.register("oct-client-discovery")
class DiscoveryClientCertificate(_ContrailClientCertificate):
    SUBGROUP = "discovery"
    PATH = "/etc/contrail/discovery-oct-client.pem"
    USER = "root"
    GROUP = "root"
    BASE_ROLES = ["compute"] + BaseRole.OCT_HEAD_ROLES
    BASE_ROLES_DEV = ["compute-node-vm"]
