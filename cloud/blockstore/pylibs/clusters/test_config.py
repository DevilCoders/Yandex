from dataclasses import dataclass
from typing import Dict


@dataclass
class FolderDesc:
    folder_id: str
    zone_id: str
    subnet_name: str
    subnet_id: str
    image_name: str
    filesystem_id: str


@dataclass
class ClusterTestConfig:
    name: str
    solomon_cluster: str
    ipc_type_to_folder_desc: Dict[str, FolderDesc]

    def get_solomon_cluster(self, host: str) -> str:
        if self.name != 'prod':
            return self.solomon_cluster
        return self.solomon_cluster + '_' + host[:3]


def get_cluster_test_config_hw_nbs_stable_lab(zone_id: str):  # for hw-nbs-stable-lab there is only one zone
    return ClusterTestConfig(
        name='hw-nbs-stable-lab',
        solomon_cluster='cloud_hw-nbs-stable-lab',
        ipc_type_to_folder_desc={
            'grpc': FolderDesc(
                folder_id='yc.dogfood.serviceFolder',
                zone_id='ru-central1-a',
                subnet_name='cloudtestnets-ru-central1-a',
                subnet_id=None,
                image_name='ubuntu1604-stable',
                filesystem_id=None,
            ),
            # TODO(arigachnyy): add vhost folder for hw-nbs-stable-lab
        }
    )


def get_cluster_test_config_testing(zone_id: str) -> ClusterTestConfig:
    return ClusterTestConfig(
        name='testing',
        solomon_cluster='yandexcloud_testing',
        ipc_type_to_folder_desc={
            'grpc': FolderDesc(
                folder_id='nbs.tests.folder',
                zone_id=zone_id,
                subnet_name=f'cloudvmnets-{zone_id}',
                subnet_id=None,
                image_name='ubuntu1604-stable',
                filesystem_id=None,
            ),
            'vhost': FolderDesc(
                folder_id='nbs.tests-vhost.folder',
                zone_id=zone_id,
                subnet_name=f'cloudvmnets-{zone_id}',
                subnet_id=None,
                image_name='ubuntu1604-stable',
                filesystem_id=None,
            ),
        }
    )


def get_cluster_test_config_preprod(zone_id: str) -> ClusterTestConfig:
    vhost_filesystem_id = {
        'ru-central1-a': 'a7lrr587m6ej4bulec52',
        'ru-central1-b': 'c8rj9tmcie2cn7oclmqt',
        'ru-central1-c': 'd9hk8do0grdskhvanvop',
    }
    return ClusterTestConfig(
        name='preprod',
        solomon_cluster='yandexcloud_preprod',
        ipc_type_to_folder_desc={
            'grpc': FolderDesc(
                folder_id='aoeo9gm0oegspcd5psm1',  # tests
                zone_id=zone_id,
                subnet_name=f'cloudvmnets-{zone_id}',
                subnet_id=None,
                image_name='ubuntu1604-stable',
                filesystem_id=None,
            ),
            'vhost': FolderDesc(
                folder_id='aoeal318uf4kpa3arqsm',  # tests-vhost
                zone_id=zone_id,
                subnet_name=f'cloudvmnets-{zone_id}',
                subnet_id=None,
                image_name='ubuntu1604-stable',
                filesystem_id=vhost_filesystem_id[zone_id],
            ),
        }
    )


def get_cluster_test_config_prod(zone_id: str) -> ClusterTestConfig:
    grpc_subnet_id = {
        'ru-central1-a': 'e9bksmn8m0e27ag7jlfs',
        'ru-central1-b': 'e2l0jjdpmo90r6grc41p',
        'ru-central1-c': 'b0cs654dlj1mjr7os91d',
    }
    vhost_subnet_id = {
        'ru-central1-a': 'e9bl6mpg4hl82bpefcom',
        'ru-central1-b': 'e2l30dn9917qu8v1vcqp',
        'ru-central1-c': 'b0c1gpe5c8v7sbg6hp3a',
    }
    vhost_filesystem_id = {
        'ru-central1-a': 'fhmi297209nsd6ic013f',
        'ru-central1-b': 'epdf077emtihqt0uin25',
        'ru-central1-c': 'ef37p0ssqteitl00h1jc',
    }
    return ClusterTestConfig(  # sas
        name='prod',
        solomon_cluster='yandexcloud_prod',
        ipc_type_to_folder_desc={
            'grpc': FolderDesc(
                folder_id='b1g6ljuaclej00ahe38h',  # tests
                zone_id=zone_id,
                subnet_name=None,
                subnet_id=grpc_subnet_id[zone_id],
                image_name='ubuntu1604-stable',
                filesystem_id=None,
            ),
            'vhost': FolderDesc(
                folder_id='b1g2sds1grbivr54a79q',  # tests-vhost
                zone_id=zone_id,
                subnet_name=None,
                subnet_id=vhost_subnet_id[zone_id],
                image_name='ubuntu1604-stable',
                filesystem_id=vhost_filesystem_id[zone_id],
            ),
        }
    )


_CLUSTERS_TEST_CONFIGS = {
    'hw-nbs-stable-lab': get_cluster_test_config_hw_nbs_stable_lab,
    'testing': get_cluster_test_config_testing,
    'preprod': get_cluster_test_config_preprod,
    'prod': get_cluster_test_config_prod
}


def get_cluster_test_config(name: str, zone_id: str) -> ClusterTestConfig:
    return _CLUSTERS_TEST_CONFIGS[name](zone_id)
