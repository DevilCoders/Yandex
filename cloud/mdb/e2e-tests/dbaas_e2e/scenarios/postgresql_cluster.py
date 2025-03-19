"""
DBaaS E2E tests scenarios for postgresql
"""
import psycopg2

from ..utils import geo_name, scenario, wait_tcp_conn

TRANSLATE_TBL = str.maketrans(dict.fromkeys('-.', '_'))


def _connect(config, host):
    conn_str_tmpl = 'host={host} port=6432 dbname={dbname}' ' user={user} password={pwd}'
    return psycopg2.connect(
        conn_str_tmpl.format(host=host, dbname=config.dbname, user=config.dbuser, pwd=config.dbpassword)
    )


@scenario
class PostgresqlClusterCreate:
    """
    Scenario for postgresql creation
    """

    CLUSTER_TYPE = 'postgresql'

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        return {
            'configSpec': {
                'version': '12',
                'postgresqlConfig_12': {},
                'resources': {
                    'resourcePresetId': config.flavor,
                    'diskTypeId': config.disk_type,
                    'diskSize': 10737418240,
                },
            },
            'databaseSpecs': [
                {
                    'name': config.dbname,
                    'owner': config.dbuser,
                    'extensions': [
                        {
                            'name': 'pgcrypto',
                        }
                    ],
                }
            ],
            'userSpecs': [
                {
                    'name': config.dbuser,
                    'password': config.dbpassword,
                    'connLimit': 10,
                }
            ],
            'hostSpecs': [
                {
                    'assignPublicIp': config.assign_public_ip,
                    'zoneId': geo_name(config, 'man'),
                },
                {
                    'assignPublicIp': config.assign_public_ip,
                    'zoneId': geo_name(config, 'vla'),
                },
                {
                    'assignPublicIp': config.assign_public_ip,
                    'zoneId': geo_name(config, 'sas'),
                },
            ],
        }

    @staticmethod
    def post_check(config, hosts, **_):
        """
        Post-creation check
        """
        master = None
        replicas = list()
        stat_replication = None
        all_hosts = [x['name'] for x in hosts['hosts']]
        replicas_cnt = len(all_hosts) - 1
        for host in all_hosts:
            wait_tcp_conn(host, 6432, timeout=config.precheck_conn_timeout)
            with _connect(config, host) as con:
                with con.cursor() as cur:
                    cur.execute('SELECT NOT pg_is_in_recovery()')
                    (is_master,) = cur.fetchone()
                    if is_master:
                        assert (
                            master is None
                        ), 'Split brain: {host} is master, but master' ' already found in host {master}'.format(
                            host=host, master=master
                        )
                        # NOTE: here we select `application_name` column
                        # instead of `client_hostname` because our custom
                        # user has no superuser privileges and he will
                        # see NULL instead of actual value.
                        cur.execute('SELECT application_name FROM pg_stat_replication')
                        stat_replication = [row[0] for row in cur.fetchall()]
                        assert len(stat_replication) == replicas_cnt, (
                            'Unexpected number of replicas connected to '
                            ' master: {found}, expected {exp}, replicas:'
                            ' {replicas}'.format(
                                found=len(stat_replication), exp=replicas_cnt, replicas=stat_replication
                            )
                        )
                        master = host
                    else:
                        replicas.append(host)
        assert len(replicas) == replicas_cnt, 'Expected {exp} replicas, but found {found}: {replicas}'.format(
            exp=replicas_cnt, found=len(replicas), replicas=replicas
        )
        for fqdn in replicas:
            assert (
                fqdn.translate(
                    TRANSLATE_TBL,
                )
                in stat_replication
            ), 'Replica {host} not found in master ' 'pg_stat_replication: {stat}'.format(
                host=fqdn, stat=stat_replication
            )
