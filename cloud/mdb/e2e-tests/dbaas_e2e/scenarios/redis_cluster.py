"""
DBaaS E2E tests scenarios for redis
"""
import redis
from retrying import retry

from ..utils import geo_name, scenario, wait_tcp_conn


@retry(wait_exponential_multiplier=100, wait_exponential_max=1000, stop_max_attempt_number=60)
def check(config, all_hosts):
    """
    Check cluster configuration
    """
    masters = []
    for host in all_hosts:
        wait_tcp_conn(host, 6380, timeout=config.precheck_conn_timeout)
        info = redis.StrictRedis(
            host=host, port=6380, db=0, password=config.dbpassword, ssl=True, ssl_ca_certs=config.conn_ca_path
        ).info()
        if info['role'] == 'master':
            masters.append(host)
            connected = info['connected_slaves']
            assert connected == len(all_hosts) - 1, 'Found {found} replicas, expected {expected}'.format(
                found=connected, expected=(len(all_hosts) - 1)
            )
    assert len(masters) == 1, 'Found {cnt} masters {masters}'.format(cnt=len(masters), masters=masters)


@scenario
class RedisClusterCreate:
    """
    Scenario for redis creation
    """

    CLUSTER_TYPE = 'redis'

    @staticmethod
    def get_options(config):
        """
        Returns options for redis cluster creation
        """
        return {
            'configSpec': {
                'version': '6.2',
                'redisConfig_6_2': {
                    'password': config.dbpassword,
                },
                'resources': {
                    'resourcePresetId': config.redis_flavor,
                    'diskSize': 21474836480,
                },
            },
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
            'tlsEnabled': True,
        }

    @staticmethod
    def post_check(config, hosts, **_):
        """
        Post-creation checks
        """
        all_hosts = [x['name'] for x in hosts['hosts']]
        check(config, all_hosts)
