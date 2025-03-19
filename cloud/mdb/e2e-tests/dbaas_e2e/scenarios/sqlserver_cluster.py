"""
DBaaS E2E tests scenarios for SQLServer
"""

import subprocess

from yandex.cloud.priv.mdb.sqlserver.v1 import cluster_service_pb2 as ss_spec
from yandex.cloud.priv.mdb.sqlserver.v1.cluster_service_pb2_grpc import ClusterServiceStub

from ..utils import geo_name, scenario, wait_tcp_conn


@scenario
class SQLServerClusterCreate:
    """
    Scenario for SQLServer creation
    """

    CLUSTER_TYPE = 'sqlserver'
    API_CLIENT = 'internal-grpc'

    @staticmethod
    def get_api_client_kwargs():
        return {
            'grpc_defs': {
                'cluster_service': {
                    'stub': ClusterServiceStub,
                    'requests': {
                        'create': ss_spec.CreateClusterRequest,
                        'delete': ss_spec.DeleteClusterRequest,
                        'list_clusters': ss_spec.ListClustersRequest,
                        'list_hosts': ss_spec.ListClusterHostsRequest,
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
            'environment': config.environment,
            'folder_id': config.folder_id,
            'network_id': config.network_id,
            'name': 'sqlserver_e2e',
            'config_spec': {
                'version': '2016sp2ent',
                'sqlserver_config_2016sp2ent': {},
                'secondary_connections': 'SECONDARY_CONNECTIONS_READ_ONLY',
                'resources': {
                    'resource_preset_id': config.sqlserver_flavor,
                    'disk_size': 10737418240,
                    'disk_type_id': config.disk_type,
                },
            },
            'database_specs': [
                {
                    'name': config.dbname,
                }
            ],
            'user_specs': [
                {
                    'name': config.dbuser,
                    'password': config.dbpassword,
                    'permissions': [
                        {
                            'database_name': config.dbname,
                            'roles': ['DB_OWNER'],
                        }
                    ],
                }
            ],
            'host_specs': [
                {
                    'zone_id': geo_name(config, 'man'),
                },
                {
                    'zone_id': geo_name(config, 'vla'),
                },
                {
                    'zone_id': geo_name(config, 'sas'),
                },
            ],
        }

    @staticmethod
    def post_check(config, hosts, **_):
        """
        Post-creation check
        """
        all_hosts = [x['name'] for x in hosts['hosts']]
        for host in all_hosts:
            wait_tcp_conn(host, 1433, timeout=config.precheck_conn_timeout)
            ret = subprocess.call(
                [
                    config.odbc_check_script,
                    '--host',
                    host,
                    '--user',
                    config.dbuser,
                    '--password',
                    config.dbpassword,
                    '--database',
                    config.dbname,
                    '--query',
                    'SELECT GETDATE()',
                ],
                timeout=10,
            )
            assert ret == 0, "ODBC query failed"
