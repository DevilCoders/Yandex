
from yc_issue_cert.secrets import ServiceCertificate, ServicePrivateKey, CaCertificate, \
    ServiceCertificateCombined, SecretFactory


class _VpcNodeSecretMixin:
    USER = "yc-vpc-node"
    GROUP = "yc-vpc-node"


@SecretFactory.register("vpc-node-wildcard-cert-pem")
class VpcNodeWildcardCertificate(_VpcNodeSecretMixin, ServiceCertificate):
    PATH = "/etc/yc/vpc-node/cert.pem"


@SecretFactory.register("vpc-node-wildcard-key-pem")
class VpcNodeWildcardKey(_VpcNodeSecretMixin, ServicePrivateKey):
    PATH = "/etc/yc/vpc-node/key.pem"


@SecretFactory.register("vpc-node-wildcard-ca-pem")
class VpcNodeWildcardCa(_VpcNodeSecretMixin, CaCertificate):
    PATH = "/etc/yc/vpc-node/ca.pem"


class _GrpcSnhProxySecretMixin:
    USER = "yc-contrail-grpcsnhproxy"
    GROUP = "yc-contrail-grpcsnhproxy"


@SecretFactory.register("grpcsnhproxy-cert-pem")
class GrpcSnhProxyCertificate(_GrpcSnhProxySecretMixin, ServiceCertificateCombined):
    PATH = "/etc/yc/grpcsnhproxy/cert.pem"


@SecretFactory.register("grpcsnhproxy-key-pem")
class GrpcSnhProxyPrivateKey(_GrpcSnhProxySecretMixin, ServicePrivateKey):
    PATH = "/etc/yc/grpcsnhproxy/key.pem"


class _VpcControlSecretMixin:
    USER = "yc-vpc-control"
    GROUP = "yc-vpc-control"


@SecretFactory.register("vpc-control-cert-pem")
class VpcControlCertificate(_VpcControlSecretMixin, ServiceCertificateCombined):
    PATH = "/etc/yc/vpc-control/cert.pem"


@SecretFactory.register("vpc-control-key-pem")
class VpcControlPrivateKey(_VpcControlSecretMixin, ServicePrivateKey):
    PATH = "/etc/yc/vpc-control/key.pem"
