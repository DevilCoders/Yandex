
from yc_issue_cert.secrets import SecretData, SecretDataHostCertsMixin, SecretFactory
from yc_issue_cert.utils import run
from yc_issue_cert.yc_crt import Certificate


@SecretFactory.register("monops-pem")
class MonopsPem(SecretDataHostCertsMixin, SecretData):
    PATH = "/etc/yc/monops/web.pem"

    @classmethod
    def create_one(cls, dstdir: str, certificate: Certificate) -> str:
        # Convert private key to PKCS8 (cloud-java compatible)
        key_path = cls._save_private_key(certificate, dstdir)
        pkcs8_path = cls._save_file(None, dstdir, "{}.p8", certificate.host)
        run("openssl", "pkcs8", topk8=True, inform="PEM", _in=key_path, out=pkcs8_path,
            nocrypt=True)

        with open(pkcs8_path, "rb") as pkcs8_file:
            pkcs8_key = pkcs8_file.read()

        return cls._save_file(certificate.get_host_cert() +
                              certificate.get_ycloud_ca_cert() +
                              pkcs8_key,
                              dstdir, "{}.pem", certificate.host)
