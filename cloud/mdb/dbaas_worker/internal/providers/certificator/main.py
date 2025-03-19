from .certificator import CertificatorApi as YandexCertificatorClient
from .mdb_secrets import MDBSecretsApi
from .noop import Noop
from .base import CrtClient, AvailableCerts, IssueResult


class CertificatorApi(CrtClient):
    def __init__(self, config, task, queue):
        desired_type = config.cert_api.api
        if desired_type == AvailableCerts.certificator:
            self.client: CrtClient = YandexCertificatorClient(config, task, queue)
        elif desired_type == AvailableCerts.mdb_secrets:
            self.client: CrtClient = MDBSecretsApi(config, task, queue)
        elif desired_type == AvailableCerts.noop:
            self.client: CrtClient = Noop()
        else:
            raise RuntimeError(f'unknown cert type {desired_type}')

    def issue(self, hostname, alt_names, force_tls_certs=False) -> IssueResult:
        return self.client.issue(hostname, alt_names, force_tls_certs)

    def revoke(self, hostname) -> None:
        return self.client.revoke(hostname)
