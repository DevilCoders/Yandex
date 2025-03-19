"""
DBaaS E2E tests scenarios for clickhouse in datacloud
"""

from clickhouse_driver import Client

from ..utils import scenario, wait_tcp_conn
from datacloud.clickhouse.v1 import cluster_service_pb2 as cs_spec
from datacloud.clickhouse.v1.cluster_service_pb2_grpc import ClusterServiceStub


@scenario
class ClickhouseDoubleCloudClusterCreate:
    """
    Scenario for clickhouse cluster creation in double cloud
    """

    CLUSTER_TYPE = 'clickhouse'
    CLUSTER_NAME_SUFFIX = '_double_cloud'
    API_CLIENT = 'doublecloud-internal-clickhouse-grpc'

    @staticmethod
    def get_api_client_kwargs():
        return {
            'grpc_defs': {
                'cluster_service': {
                    'stub': ClusterServiceStub,
                    'requests': {
                        'create': cs_spec.CreateClusterRequest,
                        'delete': cs_spec.DeleteClusterRequest,
                        'list_clusters': cs_spec.ListClustersRequest,
                        'list_hosts': cs_spec.ListClusterHostsRequest,
                        'get': cs_spec.GetClusterRequest,
                    },
                },
            },
        }

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """

        options = {
            'project_id': config.project_id,
            'cloud_type': config.cloud_type,
            'region_id': config.region_id,
            'name': config.name,
            'resources': {
                'clickhouse': {
                    'resource_preset_id': config.resource_preset_id,
                    'disk_size': config.disk_size,
                    'replica_count': config.replica_count,
                    'shard_count': config.shard_count,
                },
            },
            'access': {
                'ipv4_cidr_blocks': {
                    'values': [
                        {
                            'value': '0.0.0.0/0',
                            'description': 'access for e2e-tests',
                        }
                    ],
                },
            },
            'encryption': {
                'enabled': config.encryption,
            },
            'description': config.description,
            'version': config.version,
        }
        return options

    @staticmethod
    def post_check(config, hosts, api_client, **_):
        """
        Post-creation check
        """
        ch_hosts = [x['name'] for x in hosts['hosts']]
        cluster_id = hosts['hosts'][0]['cluster_id']
        if isinstance(config.conn_ca_path, str):
            verify = True
            ca_certs = config.conn_ca_path
        else:
            verify = config.conn_ca_path
            ca_certs = None

        clients = {}

        for host in ch_hosts:
            wait_tcp_conn(host, 9440, timeout=config.precheck_conn_timeout)
            cluster = api_client.get_cluster(cluster_id)
            password = cluster['connection_info']['password']
            clients[host] = Client(
                host,
                database=config.dbname,
                user=config.dbuser,
                password=password,
                port=9440,
                secure=True,
                verify=verify,
                ca_certs=ca_certs,
            )
