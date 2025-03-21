import functools

from cloud.mdb.clickhouse.tools.monrun_checks.clickhouse_client import ClickhouseClient, ClickhousePort


class ClickhouseInfo(object):
    @classmethod
    @functools.lru_cache(maxsize=1)
    def get_versions_count(cls):
        """
        Count different clickhouse versions in cluster.
        """
        ch_client = ClickhouseClient()
        #  I belive that all hosts in cluster have the same port set, so check current for security port
        remoteCommand = 'remoteSecure' if ch_client.check_port(ClickhousePort.tcps) else 'remote'
        replicas = cls.get_replicas()
        query = '''
            SELECT uniq(version()) FROM {remoteCommand}('{replicas}', system.one)
            '''.format(
            remoteCommand=remoteCommand, replicas=','.join(replicas)
        )
        return int(ch_client.execute(query)[0][0])

    @classmethod
    @functools.lru_cache(maxsize=1)
    def get_replicas(cls):
        """
        Get hostnames of replicas.
        """
        cluster = cls.get_cluster()
        query = '''
            SELECT host_name
            FROM system.clusters
            WHERE cluster = '{cluster}'
            AND shard_num = (SELECT shard_num FROM system.clusters
                            WHERE host_name = hostName() AND cluster = '{cluster}')
            '''.format(
            cluster=cluster
        )
        return [row[0] for row in ClickhouseClient().execute(query)]

    @classmethod
    @functools.lru_cache(maxsize=1)
    def get_cluster(cls):
        """
        Get cluster identifier.
        """
        query = 'SELECT substitution FROM system.macros WHERE macro = \'cluster\''
        return ClickhouseClient().execute(query)[0][0]
