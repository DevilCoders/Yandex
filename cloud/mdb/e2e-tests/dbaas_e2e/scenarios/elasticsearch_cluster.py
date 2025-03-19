"""
DBaaS E2E tests scenarios for ElasticSearch
"""

import requests
from retrying import retry
from collections import namedtuple

from yandex.cloud.priv.mdb.elasticsearch.v1 import cluster_service_pb2 as es_spec
from yandex.cloud.priv.mdb.elasticsearch.v1.cluster_service_pb2_grpc import ClusterServiceStub

from ..utils import geo_name, scenario, wait_tcp_conn


Creds = namedtuple('Creds', ('user', 'password'))

ADMIN_PASSWORD = 'S$cret,tshhhh!'


@scenario
class ElasticsearchClusterCreate:
    """
    Scenario for ElasticSearch creation
    """

    CLUSTER_TYPE = 'elasticsearch'
    API_CLIENT = 'internal-grpc'

    @staticmethod
    def get_api_client_kwargs():
        return {
            'grpc_defs': {
                'cluster_service': {
                    'stub': ClusterServiceStub,
                    'requests': {
                        'create': es_spec.CreateClusterRequest,
                        'delete': es_spec.DeleteClusterRequest,
                        'list_clusters': es_spec.ListClustersRequest,
                        'list_hosts': es_spec.ListClusterHostsRequest,
                    },
                },
            },
        }

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        return {
            'host_specs': [
                {
                    'zone_id': geo_name(config, 'vla'),
                    'type': 'DATA_NODE',
                    'assign_public_ip': config.assign_public_ip,
                },
                {
                    'zone_id': geo_name(config, 'man'),
                    'type': 'DATA_NODE',
                    'assign_public_ip': config.assign_public_ip,
                },
                {
                    'zone_id': geo_name(config, 'sas'),
                    'type': 'DATA_NODE',
                    'assign_public_ip': config.assign_public_ip,
                },
            ],
            'environment': config.environment,
            'folder_id': config.folder_id,
            'network_id': config.network_id,
            'name': 'elasticsearch_e2e',
            'config_spec': {
                'admin_password': ADMIN_PASSWORD,
                'elasticsearch_spec': {
                    'data_node': {
                        'resources': {
                            'resource_preset_id': config.elasticsearch_flavor,
                            'disk_size': 10737418240,
                            'disk_type_id': config.disk_type,
                        },
                    },
                },
            },
        }

    @staticmethod
    def post_check(config, hosts, cluster_id, **_):
        """
        Post-creation check
        """
        all_hosts = [h['name'] for h in hosts['hosts']]

        num_hosts = len(all_hosts)
        assert num_hosts > 0, 'Expected number of hosts > 0 ({})'.format(cluster_id)

        for host in all_hosts:
            wait_tcp_conn(host, 9200, config.precheck_conn_timeout)

        host = all_hosts[0]
        health = _get_cluster_health(host, num_hosts, auth=Creds(user='admin', password=ADMIN_PASSWORD))

        assert health['status'] == 'green', 'Expected cluster status is green, found {} ({})'.format(
            health['status'], cluster_id
        )
        assert health['number_of_nodes'] == num_hosts, 'Expected number of nodes is {}, found {} ({})'.format(
            num_hosts, health['number_of_nodes'], cluster_id
        )


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=3)
def _make_request(url, params=None, **kwargs):
    response = requests.get(url, timeout=61, params=params, **kwargs)
    response.raise_for_status()
    return response.json()


def _get_cluster_health(host, num_hosts, auth):
    url = f'https://{host}:9200/_cluster/health'
    params = {'wait_for_status': 'green', 'wait_for_nodes': num_hosts, 'timeout': '60s'}
    return _make_request(url, params=params, auth=auth)
