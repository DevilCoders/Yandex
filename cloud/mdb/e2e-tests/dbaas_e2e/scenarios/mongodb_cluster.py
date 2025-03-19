"""
DBaaS E2E tests scenarios for mongodb
"""

import ssl
from urllib.parse import quote_plus

from pymongo import MongoClient
from retrying import retry

from ..utils import geo_name, scenario, wait_tcp_conn


@retry(wait_exponential_multiplier=100, wait_exponential_max=1000, stop_max_attempt_number=30)
def get_conn(config, host):
    """
    Wait for tcp connection and open a mongodb client conn with host
    """
    wait_tcp_conn(host, 27018, timeout=config.precheck_conn_timeout)
    conn_string_tmpl = 'mongodb://{user}:{password}@{host}:27018/{dbname}'
    conn_string = conn_string_tmpl.format(
        user=quote_plus(config.dbuser), password=quote_plus(config.dbpassword), host=host, dbname=config.dbname
    )
    return MongoClient(
        conn_string,
        connect=True,
        ssl=True,
        ssl_cert_reqs=ssl.CERT_NONE,
        ssl_ca_certs=config.conn_ca_path,
    )


@scenario
class MongoDBClusterCreate:
    """
    Scenario for mongodb creation
    """

    CLUSTER_TYPE = 'mongodb'

    @staticmethod
    def get_options(config):
        """
        Returns options for cluster creation
        """
        return {
            'configSpec': {
                'mongodbSpec_4_4': {
                    'mongod': {
                        'resources': {
                            'resourcePresetId': config.flavor,
                            'diskTypeId': config.disk_type,
                            'diskSize': 10737418240,
                        },
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
                    'permissions': [
                        {
                            'databaseName': config.dbname,
                            'roles': ['readWrite'],
                        }
                    ],
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
        masters = list()
        replicas = list()
        all_hosts = [x['name'] for x in hosts['hosts']]
        for host in all_hosts:
            client = get_conn(config, host)
            if client.is_primary:
                masters.append(host)
            else:
                replicas.append(host)
        assert len(masters) == 1, 'Found {cnt} masters {masters}'.format(cnt=len(masters), masters=masters)
        assert len(replicas) == len(all_hosts) - 1, 'Expected {exp} replicas, found {found}'.format(
            exp=(len(all_hosts) - 1), found=len(replicas)
        )
