"""
Simple internal-api config mock
"""

from httmock import urlmatch

from .utils import handle_http_action, http_return
from cloud.mdb.dbaas_worker.internal.tasks.hadoop.utils import MASTER_ROLE_TYPE, DATA_ROLE_TYPE, COMPUTE_ROLE_TYPE


def internal_api(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='internal-api.test', method='get', path='/mdb/.*/1.0/clusters/.*/backups.*')
    def backups(url, _):
        cid = url.path.split('/')[5]
        return http_return(
            body=state['internal-api-config']['backups'][cid],
        )

    @urlmatch(netloc='internal-api.test', method='get', path='/api/v1.0/config_unmanaged/.+')
    def config_unmanaged(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'internal-api-get-config-{fqdn}')
        if ret:
            return ret

        return http_return(
            body={
                'data': {
                    'service_account_id': 'test-service-account',
                    'image': 'hadoop-image',
                    'subcluster_main_id': 'subcid-test-m',
                    'topology': {
                        'network_id': 'test-network-id',
                        'zone_id': 'test-zone-id',
                        'subclusters': {
                            'subcid-test-m': {
                                'role': MASTER_ROLE_TYPE,
                                'subcid': 'subcid-test-m',
                                'subnet_id': 'test-subnet_id',
                                # fqdn format matters here
                                'hosts': ['rc1b-dataproc-m-dlsca2qeisuy57ca.mdb.cloud-preprod.yandex.net'],
                            },
                            'subcid-test-d': {
                                'role': DATA_ROLE_TYPE,
                                'subcid': 'subcid-test-d',
                            },
                            'subcid-test-d2': {
                                'role': DATA_ROLE_TYPE,
                                'subcid': 'subcid-test-d2',
                            },
                            'subcid-test-c': {
                                'role': COMPUTE_ROLE_TYPE,
                                'name': 'subcid-test-c',
                                'subnet_id': 'test-compute-subnet-id-1',
                                'subcid': 'subcid-test-c',
                                'resources': {
                                    'memory': 1,
                                    'cores': 2,
                                    'disk_type_id': 2,
                                    'core_fraction': 100,
                                    'disk_size': 2,
                                    'platform_id': 2,
                                },
                                'instance_group_config': {
                                    'scale_policy': {},
                                    'instance_template': {
                                        'scheduling_policy': {},
                                    },
                                },
                            },
                            'subcid-test-c2': {
                                'role': COMPUTE_ROLE_TYPE,
                                'name': 'subcid-test-c2',
                                'subcid': 'subcid-test-c2',
                                'subnet_id': 'test-compute-subnet-id-2',
                                'resources': {
                                    'memory': 1,
                                    'cores': 2,
                                    'disk_type_id': 2,
                                    'core_fraction': 100,
                                    'disk_size': 2,
                                    'platform_id': 2,
                                },
                                'instance_group_config': {
                                    'scale_policy': {},
                                    'instance_template': {
                                        'scheduling_policy': {},
                                    },
                                },
                            },
                        },
                    },
                }
            }
        )

    return (
        config_unmanaged,
        backups,
    )
