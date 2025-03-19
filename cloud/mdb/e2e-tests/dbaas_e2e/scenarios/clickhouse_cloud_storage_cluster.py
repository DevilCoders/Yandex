"""
DBaaS E2E tests scenarios for clickhouse
"""

from clickhouse_driver import Client

from ..utils import geo_name, scenario, wait_tcp_conn


@scenario
class ClickhouseCloudStorageClusterCreate:
    """
    Scenario for clickhouse creation
    """

    CLUSTER_TYPE = 'clickhouse'

    CLUSTER_NAME_SUFFIX = '_cloud_storage'

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
                'cloudStorage': {
                    'enabled': True,
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
                }
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

        for host in ch_hosts:
            wait_tcp_conn(host, 9440, timeout=config.precheck_conn_timeout)
            client = Client(
                host,
                database=config.dbname,
                user=config.dbuser,
                password=config.dbpassword,
                port=9440,
                secure=True,
                verify=verify,
                ca_certs=ca_certs,
            )

            client.execute(
                f"""
                CREATE TABLE `{config.dbname}`.test (
                    dt Date,
                    id Int64
                ) ENGINE=MergeTree()
                PARTITION BY dt
                ORDER BY (dt, id)
                """
            )

            client.execute(
                f"""
                INSERT INTO `{config.dbname}`.test (dt, id)
                VALUES ('2020-01-03', 1), ('2020-01-04', 2), ('2020-01-05', 3)
                """
            )

            client.execute(
                f"""
                ALTER TABLE `{config.dbname}`.test MOVE PARTITION '2020-01-04' TO DISK 'object_storage'
                """
            )

            result = client.execute(
                f"""
                SELECT count(*) FROM system.parts where database = '{config.dbname}' and table = 'test' and disk_name = 'object_storage'
                """
            )
            assert result[0] == (1,), 'Only 1 part should be stored on "object_storage" disk, got {parts}'.format(
                parts=result[0]
            )

            result = client.execute(
                f"""
                SELECT sum(id) FROM `{config.dbname}`.test
                """
            )
            assert result[0] == (6,), 'Wrong data check, got {sum}, expected: 6'.format(sum=result[0])

            client.execute(
                f"""
                DROP TABLE `{config.dbname}`.test
                """
            )
