"""
DBaaS E2E tests scenarios for mysql
"""
import time
from contextlib import closing

import pymysql
from retrying import retry

from ..utils import geo_name, scenario, wait_tcp_conn


@retry(wait_exponential_multiplier=100, wait_exponential_max=1000, stop_max_attempt_number=30)
def _connect(config, host):
    return pymysql.connect(
        host=host,
        port=3306,
        user=config.dbuser,
        passwd=config.dbpassword,
        db=config.dbname,
        ssl={
            'ca': config.conn_ca_path if isinstance(config.conn_ca_path, str) else None,
        },
    )


@scenario
class MysqlClusterCreate:
    """
    Scenario for mysql creation
    """

    CLUSTER_TYPE = 'mysql'

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        return {
            'configSpec': {
                'mysqlConfig_5_7': {},
                'resources': {
                    'resourcePresetId': config.flavor,
                    'diskTypeId': config.disk_type,
                    'diskSize': 10737418240,
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
                    'permissions': [
                        {
                            'databaseName': config.dbname,
                            'roles': ['ALL_PRIVILEGES'],
                        }
                    ],
                }
            ],
            'hostSpecs': [
                {
                    'zoneId': geo_name(config, 'man'),
                },
                {
                    'zoneId': geo_name(config, 'vla'),
                },
                {
                    'zoneId': geo_name(config, 'sas'),
                },
            ],
        }

    @staticmethod
    def post_check(config, hosts, **_):
        """
        Post-creation check
        """
        master, replicas = MysqlClusterCreate._test_connection(config, hosts)
        MysqlClusterCreate._test_statuses(config, master, replicas)
        MysqlClusterCreate._test_data_replication(config, master, replicas)

    @staticmethod
    def _test_connection(config, hosts):
        master = None
        masters = list()
        replicas = list()
        all_hosts = [x['name'] for x in hosts['hosts']]
        replicas_cnt = len(all_hosts) - 1
        for host in all_hosts:
            wait_tcp_conn(host, 3306, timeout=config.precheck_conn_timeout)
            # pylint: disable=E1101
            with closing(_connect(config, host)) as conn:
                cur = conn.cursor(pymysql.cursors.DictCursor)
                cur.execute('SHOW SLAVE STATUS')
                is_slave = cur.fetchone() is not None
                if is_slave:
                    replicas.append(host)
                else:
                    masters.append(host)

        assert len(replicas) == replicas_cnt, 'Expected {exp} replicas, but found {found}: {replicas}'.format(
            exp=replicas_cnt, found=len(replicas), replicas=replicas
        )
        assert len(masters) == 1, 'Expected exactly one master, but found {found}: {masters}'.format(
            found=len(masters), masters=masters
        )
        master = masters[0]
        return master, replicas

    @staticmethod
    def _test_statuses(config, master, replicas):
        # pylint: disable=E1101
        with closing(_connect(config, master)) as conn:
            cur = conn.cursor(pymysql.cursors.DictCursor)
            cur.execute('SELECT @@read_only AS ro')
            res = cur.fetchone()
            assert int(res['ro']) == 0, 'Suddenly master host {master} is read-only'.format(master=master)

        for host in replicas:
            # pylint: disable=E1101
            with closing(_connect(config, host)) as conn:
                cur = conn.cursor(pymysql.cursors.DictCursor)
                cur.execute('SELECT @@read_only AS ro')
                res = cur.fetchone()
                assert int(res['ro']) == 1, 'Suddenly slave host {host} is NOT read-only'.format(host=host)
                cur.execute('SHOW SLAVE STATUS')
                res = cur.fetchone()
                assert res is not None, 'Slave {host} has empty SLAVE STATUS'.format(host=host)
                assert res['Master_Host'] == master, 'Slave {host} connected to \'{mh}\' instead of {master}'.format(
                    host=host, master=master, mh=res['Master_Host']
                )
                assert res['Slave_IO_Running'] == 'Yes', 'Slave {host} IO thread is not running: {err}'.format(
                    host=host, err=res['Last_IO_Error']
                )
                assert res['Slave_SQL_Running'] == 'Yes', 'Slave {host} SQL thread is not running: {err}'.format(
                    host=host, err=res['Last_SQL_Error']
                )

    @staticmethod
    def _test_data_replication(config, master, replicas):
        # pylint: disable=E1101
        with closing(_connect(config, master)) as conn:
            cur = conn.cursor(pymysql.cursors.DictCursor)
            cur.execute('CREATE TABLE test_table(id INT)')
            cur.execute('INSERT INTO test_table VALUES (42)')
            conn.commit()

        deadline = time.time() + config.replication_timeout

        replicas_ok = set()
        last_err = None
        while len(replicas_ok) < len(replicas) and time.time() < deadline:
            for host in set(replicas) - replicas_ok:
                # pylint: disable=E1101
                with closing(_connect(config, host)) as conn:
                    cur = conn.cursor(pymysql.cursors.DictCursor)
                    try:
                        cur.execute('SELECT id FROM test_table')
                        the_answer = cur.fetchone()['id']
                        assert the_answer == 42
                    except Exception as err:
                        last_err = err
                        time.sleep(1)
                        break
                    else:
                        replicas_ok.add(host)

        assert len(replicas_ok) == len(replicas), 'Data was not replicated to {hosts}, got err: {err}'.format(
            hosts=set(replicas) - replicas_ok, err=last_err
        )
