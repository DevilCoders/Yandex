from dataclasses import dataclass
@dataclass
class Endpoints:
    priv_iaas_api: str
    priv_identity_api: str
    priv_s3_api: str
    priv_mdb_api: str
    priv_billing_api: str
    pub_mdb_api: str
    pub_compute_api: str

    @property
    def iam_url(self):
        return {
            'PROD': 'iam.private-api.cloud.yandex.net:4283',
            'PREPROD': 'iam.private-api.cloud-preprod.yandex.net:4283'
        }

    @property
    def iam_token_url(self):
        return {
            'PROD': 'ts.private-api.cloud.yandex.net:4282',
            'PREPROD': 'ts.private-api.cloud-preprod.yandex.net:4282'
        }

    @property
    def all_folders_url(self):
        return {
            'PROD': 'rm.private-api.cloud.yandex.net:4284',
            'PREPROD': 'rm.private-api.cloud-preprod.yandex.net:4284'
        }
    @property
    def compute_api(self):
        return {
            'PROD': 'compute-api.cloud.yandex.net:9051',
            'PREPROD': 'compute-api.cloud-preprod.yandex.net:9051'
        }
    @property
    def search_folder_url(self):
        return {
            'PROD': 'rm.private-api.cloud.yandex.net:4284',
            'PREPROD': 'rm.private-api.cloud-preprod.yandex.net:4284'
        }


    @property
    def cloud_url(self):
        return {
            'PROD': 'rm.private-api.cloud.yandex.net:4284',
            'PREPROD': 'rm.private-api.cloud-preprod.yandex.net:4284'
        }

    @property
    def network_url(self):
        return {
            'PROD': 'api-adapter.private-api.ycp.cloud.yandex.net:443',
            'PREPROD': 'api-adapter.private-api.ycp.cloud-preprod.yandex.net:443'
        }

    @property
    def vm_url(self):
        return {
          'PROD':  self.priv_iaas_api + '/compute/private/v1/instances/',
            'PREPROD': 'https://iaas.private-api.cloud-preprod.yandex.net/compute/private/v1/instances/'
        }

    @property
    def s3_url(self) -> str:
        return self.priv_s3_api + '/management/stats/buckets/'

    @property
    def compute_url(self) -> str:
        return self.pub_compute_api + '/compute/v1/instances/'

    @property
    def mdb_url(self) -> str:
        return self.pub_mdb_api + '/'

    @property
    def mdb_search(self) -> str:
        return self.priv_mdb_api + '/mdb/v1/support/clusters/search'

    @property
    def billing_url(self) -> str:
        return self.priv_billing_api + '/billing/v1/private/'

    @property
    def k8s_url(self):
        return {
            'PROD': 'mk8s.private-api.ycp.cloud.yandex.net:443',
            'PREPROD': 'mk8s.private-api.ycp.cloud-preprod.yandex.net:443'
        }


class WellKnownEndpoints:
    prod = Endpoints(
        priv_iaas_api='https://iaas.private-api.cloud.yandex.net',
        priv_identity_api='https://identity.private-api.cloud.yandex.net:14336',  # ToDo: выпил in progress
        priv_s3_api='https://storage-idm.private-api.cloud.yandex.net:1443',
        priv_mdb_api='https://mdb.private-api.cloud.yandex.net',
        priv_billing_api='https://billing.private-api.cloud.yandex.net:16465',
        pub_mdb_api='https://mdb.api.cloud.yandex.net',
        pub_compute_api='https://compute.api.cloud.yandex.net',
    )
    preprod = Endpoints(
        priv_iaas_api='https://iaas.private-api.cloud-preprod.yandex.net',
        priv_identity_api='https://identity.private-api.cloud-preprod.yandex.net:14336',
        priv_s3_api='https://storage-idm.private-api.cloud-preprod.yandex.net:1443',
        priv_mdb_api='https://mdb.private-api.cloud-preprod.yandex.net',
        priv_billing_api='https://billing.private-api.cloud-preprod.yandex.net:16465',
        pub_mdb_api='https://mdb.api.cloud-preprod.yandex.net',
        pub_compute_api='https://compute.api.cloud-preprod.yandex.net',
    )
