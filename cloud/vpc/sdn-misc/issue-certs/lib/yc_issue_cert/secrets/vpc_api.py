from yc_issue_cert.secrets import SecretFactory, ServicePkcs8PemCertificate


class _VpcApiSecretMixin:
    USER = "yc-vpc-config-plane"
    GROUP = "yc-vpc-config-plane"


@SecretFactory.register("vpc-api-server-pem")
class VpcApiCertificate(_VpcApiSecretMixin, ServicePkcs8PemCertificate):
    PATH = "/etc/yc/vpc-config-plane/server.pem"
