from dataclasses import dataclass
from endpoints import Endpoints, WellKnownEndpoints


@dataclass
class Profile:
    name: str
    endpoints: Endpoints
    solomon_instance_usage_url: str
    solomon_cluster_usage_url: str
    prod_disk_dashbord: str
    profile_static_keys: dict


@dataclass
class UserKey:
    key = {
        'id': '',
        'acc_id': '',
        'priv_key': ''
    }


class WellKnownSolomonUrls:
    prod_instance_usage = 'https://monitoring.yandex-team.ru/projects/yandexcloud/dashboards/moncsrb1imkm459ifvmh?p.cluster=cloud_prod_compute&p.service=compute&p.instance_id={}&range=1d&refresh=60'
    prod_cluster_usage = 'https://solomon.cloud.yandex-team.ru/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system&cluster=mdb_{}&b=1d&e=&l.host={}'

    prod_disk_dashbord = 'https://monitoring.yandex-team.ru/projects/nbs/dashboards/monro1kvjdhr1b61t03m?p.cluster=yandexcloud_prod_%2A&p.service=client_volume&p.volume={}&range=1d&refresh=60'

    preprod_instance_usage = 'https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_preprod_compute&service=compute&instance_id={}&dashboard=cloud-prod-instance-health&b=7d&e='
    preprod_cluster_usage = 'https://solomon.cloud-preprod.yandex-team.ru/?project=yandexcloud&service=yandexcloud_dbaas&dashboard=cloud-mdb-instance-system&cluster=mdb_{}&b=1d&e=&l.host={}'

    preprod_disk_dashbord = 'https://monitoring.yandex-team.ru/projects/nbs/dashboards/monro1kvjdhr1b61t03m?p.cluster=yandexcloud_preprod&p.service=client_volume&p.volume={}&range=1d&refresh=60'


class WellKnownProfiles:
    prod = Profile(
        'prod', WellKnownEndpoints.prod,
        WellKnownSolomonUrls.prod_instance_usage, WellKnownSolomonUrls.prod_cluster_usage,
        WellKnownSolomonUrls.prod_disk_dashbord, UserKey.key
    )
    preprod = Profile(
        'preprod', WellKnownEndpoints.preprod,
        WellKnownSolomonUrls.preprod_instance_usage, WellKnownSolomonUrls.preprod_cluster_usage,
        WellKnownSolomonUrls.preprod_disk_dashbord, UserKey.key
    )

    __all__ = [prod, preprod]
