from yc_issue_cert.secrets import SecretDataHostCertsMixin, SecretData, SecretFactory
from yc_issue_cert.yc_crt import Certificate


@SecretFactory.register("compute-node-wildcard-pem")
class ComputeNodeWildcardCertificate(SecretDataHostCertsMixin, SecretData):
    USE_SCOPE = False
    PATH = "/etc/nginx/ssl/compute-node.api.pem"
    USER = "yc-compute-node"
    GROUP = "yc-compute-node"
    NAME_SUFFIX = "pem"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        # NOTE: compute-head contains internal certificate as CA,
        # so we should put it here for head ssl verify
        return cls._save_file(certificate.get_host_cert() +
                              certificate.get_ycloud_ca_cert() +
                              certificate.get_yaint_ca_cert() +
                              certificate.get_private_key(),
                              dstdir, "{}.pem", certificate.host_sanitized)
