"""
DBaaS E2E tests scenarios for clickhouse
"""

from clickhouse_driver import Client

from ..utils import geo_name, scenario, wait_tcp_conn


@scenario
class ClickhouseClusterCreate:
    """
    Scenario for clickhouse creation
    """

    CLUSTER_TYPE = 'clickhouse'

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        return {
            'configSpec': {
                'clickhouse': {
                    'resources': {
                        'resourcePresetId': config.flavor,
                        'diskTypeId': config.disk_type,
                        'diskSize': 10737418240,
                    },
                },
                'zookeeper': {
                    'resources': {
                        'resourcePresetId': config.flavor,
                        'diskTypeId': config.disk_type,
                        'diskSize': 10737418240,
                    },
                },
            },
            'databaseSpecs': [
                {
                    'name': config.dbname,
                }
            ],
            'userSpecs': [
                {
                    'name': config.dbuser,
                    'password': config.dbpassword,
                }
            ],
            'hostSpecs': [
                {
                    'type': 'CLICKHOUSE',
                    'assignPublicIp': config.assign_public_ip,
                    'zoneId': geo_name(config, 'man'),
                },
                {
                    'type': 'CLICKHOUSE',
                    'assignPublicIp': config.assign_public_ip,
                    'zoneId': geo_name(config, 'vla'),
                },
            ],
        }

    @staticmethod
    def post_check(config, hosts, **_):
        """
        Post-creation check
        """
        ch_hosts = [x['name'] for x in hosts['hosts'] if x['type'] == 'CLICKHOUSE']

        if isinstance(config.conn_ca_path, str):
            verify = True
            ca_certs = config.conn_ca_path
        else:
            verify = config.conn_ca_path
            ca_certs = None

        clients = {}

        for host in ch_hosts:
            wait_tcp_conn(host, 9440, timeout=config.precheck_conn_timeout)
            clients[host] = Client(
                host,
                database=config.dbname,
                user=config.dbuser,
                password=config.dbpassword,
                port=9440,
                secure=True,
                verify=verify,
                ca_certs=ca_certs,
            )

        for host in ch_hosts:
            clients[host].execute(
                f"""
                CREATE TABLE `{config.dbname}`.test (id UInt64, date Date)
                ENGINE = ReplicatedMergeTree(
                    '/clickhouse/tables/{{shard}}/test',
                    '{{replica}}',
                    date,
                    id,
                    8192)
                """
            )

        for host in ch_hosts:
            result = clients[host].execute(
                f"""
                SELECT total_replicas, active_replicas
                FROM system.replicas
                WHERE (engine = 'ReplicatedMergeTree')
                    AND (database = '{config.dbname}')
                    AND (table = 'test')
                """
            )

            assert len(result) == 1, 'Table test not found on clickhouse {host}'.format(host=host)
            total_replicas, active_replicas = result[0]
            assert total_replicas == len(ch_hosts), 'Expected {expect} replicas, got {total}'.format(
                expect=len(ch_hosts), total=total_replicas
            )
            assert active_replicas == len(ch_hosts), 'Found only {total} active replicas, expect {expect}'.format(
                expect=len(ch_hosts), total=active_replicas
            )
