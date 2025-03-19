"""
DBaaS E2E tests scenarios for greenplum
"""

import psycopg2
from retrying import retry
from collections import namedtuple

from yandex.cloud.priv.mdb.greenplum.v1 import cluster_service_pb2 as gp_spec
from yandex.cloud.priv.mdb.greenplum.v1.cluster_service_pb2_grpc import ClusterServiceStub

from ..utils import geo_name, scenario, wait_tcp_conn


Creds = namedtuple('Creds', ('user', 'password'))


@scenario
class GreenplumClusterCreate:
    """
    Scenario for Greenplum creation
    """

    CLUSTER_TYPE = 'greenplum'
    API_CLIENT = 'internal-grpc'

    @staticmethod
    def get_api_client_kwargs():
        return {
            'grpc_defs': {
                'cluster_service': {
                    'stub': ClusterServiceStub,
                    'requests': {
                        'create': gp_spec.CreateClusterRequest,
                        'delete': gp_spec.DeleteClusterRequest,
                        'list_clusters': gp_spec.ListClustersRequest,
                        'list_hosts': gp_spec.ListClusterHostsRequest,
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
            'config': {
                'zone_id': geo_name(config, 'sas'),
                'assign_public_ip': config.assign_public_ip,
            },
            'master_config': {
                'resources': {
                    'resourcePresetId': config.greenplum_master_flavor,
                    'diskTypeId': config.disk_type,
                    'diskSize': 10737418240,
                },
            },
            'segment_config': {
                'resources': {
                    'resourcePresetId': config.greenplum_segment_flavor,
                    'diskTypeId': config.disk_type,
                    'diskSize': 10737418240,
                },
            },
            'environment': config.environment,
            'folder_id': config.folder_id,
            'network_id': config.network_id,
            'name': 'greenplum_e2e',
            'segment_in_host': config.greenplum_segment_in_host,
            'master_host_count': config.greenplum_master_host_count,
            'segment_host_count': config.greenplum_segment_host_count,
            'user_name': config.dbuser,
            'user_password': config.dbpassword,
        }

    @staticmethod
    def post_check(config, hosts, api_client, cluster_id, **_):
        """
        Post-creation check
        """
        create_config = GreenplumClusterCreate.get_options(config)
        expected_total = (create_config['segment_in_host'] * create_config['segment_host_count']) * 2 + create_config[
            'master_host_count'
        ]

        master_hosts = [h['name'] for h in hosts['hosts']]
        master = sorted(master_hosts)[0]
        for host in master_hosts:
            wait_tcp_conn(host, 5432, timeout=config.precheck_conn_timeout)
        with _connect(config, master) as con:
            with con.cursor() as cur:
                cur.execute("SELECT count(*) AS total FROM pg_catalog.gp_segment_configuration WHERE status='u'")
                total = cur.fetchone()[0]
        assert total == expected_total, 'Expected cluster segment count is {} but found {}'.format(
            expected_total, total
        )


@retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=3)
def _connect(config, host):
    conn_str_tmpl = 'host={host} port=5432 dbname=postgres' ' user={user} password={pwd}'
    return psycopg2.connect(conn_str_tmpl.format(host=host, user=config.dbuser, pwd=config.dbpassword))
