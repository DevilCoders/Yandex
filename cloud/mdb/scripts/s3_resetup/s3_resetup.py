"""
Resetup s3 host in dbm
"""

import argparse
import base64
import json
import logging
import os
import socket
import time
import urllib.parse
from configparser import RawConfigParser
from contextlib import closing

import requests
from retrying import retry


class S3Resetup:
    def __init__(self, config_path):
        config = RawConfigParser()
        config.read(config_path)
        level = getattr(logging, config.get('main', 'log_level').upper())
        logging.basicConfig(level=level, format='%(asctime)s %(levelname)s:\t%(message)s')
        self.logger = logging.getLogger('s3resetup')
        self.conductor_url = config.get('conductor', 'url')
        self.conductor_headers = {
            'Authorization': f'OAuth {config.get("conductor", "token")}',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.dbm_url = config.get('dbm', 'url')
        self.dbm_headers = {
            'Authorization': f'OAuth {config.get("dbm", "token")}',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.deploy_url = config.get('deploy', 'url')
        self.deploy_headers = {
            'Authorization': f'OAuth {config.get("deploy", "token")}',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.session = requests.Session()
        adapter = requests.adapters.HTTPAdapter(pool_connections=3, pool_maxsize=3)
        parsed_conductor = urllib.parse.urlparse(self.conductor_url)
        self.session.mount(f'{parsed_conductor.scheme}://{parsed_conductor.netloc}', adapter)
        parsed_dbm = urllib.parse.urlparse(self.dbm_url)
        self.session.mount(f'{parsed_dbm.scheme}://{parsed_dbm.netloc}', adapter)
        parsed_deploy = urllib.parse.urlparse(self.deploy_url)
        self.session.mount(f'{parsed_deploy.scheme}://{parsed_deploy.netloc}', adapter)

    @retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
    def try_ssh_port(self, fqdn):
        """
        Open and close connection with fqdn on port 22
        """
        self.logger.info('Trying to open connection to %s:22', fqdn)
        with closing(socket.socket(socket.AF_INET6)) as sock:
            sock.settimeout(1)
            sock.connect((fqdn, 22))

    def check_availability(self, fqdn):
        """
        Check if fqdn is available
        """
        try:
            self.try_ssh_port(fqdn)
            self.logger.info('%s seems available', fqdn)
            return True
        except Exception:
            self.logger.info('%s seems unavailable', fqdn)
            return False

    @retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
    def wait_shipment(self, shipment_id):
        """
        Wait for shipment to finish (raises on shipment error)
        """
        while True:
            result = self.session.get(
                urllib.parse.urljoin(self.deploy_url, f'v1/shipments/{shipment_id}'), headers=self.deploy_headers
            )
            result.raise_for_status()
            shipment_status = result.json()['status']

            if shipment_status == 'inprogress':
                time.sleep(1)
            elif shipment_status == 'done':
                return True
            else:
                return False

    @retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
    def find_primary(self, fqdn):
        """
        Find primary for shard
        """
        self.logger.info('Getting conductor group for %s', fqdn)
        host_info = self.session.get(
            urllib.parse.urljoin(self.conductor_url, f'api/v1/hosts/{fqdn}?format=json'), headers=self.conductor_headers
        )
        host_info.raise_for_status()
        group_name = host_info.json()['value']['group']['web_path'].split('/')[-1]
        self.logger.info('Getting hosts in group %s', group_name)
        hosts_list = self.session.get(
            urllib.parse.urljoin(self.conductor_url, f'api/v1/groups/{group_name}/hosts?format=json'),
            headers=self.conductor_headers,
        )
        hosts_list.raise_for_status()
        other_hosts = [x['value']['fqdn'] for x in hosts_list.json()['value'] if x['value']['fqdn'] != fqdn]

        shipment_data = {
            'commands': [
                {
                    'type': 'mdb_postgresql.is_replica',
                    'arguments': [],
                    'timeout': 600,
                }
            ],
            'fqdns': other_hosts,
            'parallel': len(other_hosts),
            'stopOnErrorCount': len(other_hosts),
            'timeout': 600,
        }

        self.logger.info('Running mdb_postgresql.is_replica in group %s', group_name)
        shipment_result = self.session.post(
            urllib.parse.urljoin(self.deploy_url, 'v1/shipments'), headers=self.deploy_headers, json=shipment_data
        )
        shipment_result.raise_for_status()
        shipment_id = shipment_result.json()['id']
        if not self.wait_shipment(shipment_id):
            self.logger.error('Shipment %s failed', shipment_id)
            raise RuntimeError(
                'Unable to get replica status for hosts ' f'{", ".join(other_hosts)}: shipment {shipment_id} failed'
            )
        jobs = self.session.get(
            urllib.parse.urljoin(self.deploy_url, f'v1/jobs?shipmentId={shipment_id}'), headers=self.deploy_headers
        )
        jobs.raise_for_status()
        ext_ids = {x['extId'] for x in jobs.json()['jobs']}

        for host in other_hosts:
            job_results = self.session.get(
                urllib.parse.urljoin(self.deploy_url, f'v1/jobresults?fqdn={host}'), headers=self.deploy_headers
            )
            job_results.raise_for_status()
            for res in job_results.json()['jobResults']:
                if res['extID'] in ext_ids:
                    result = json.loads(base64.b64decode(res['result']))
                    if not result['return']:
                        self.logger.info('Primary is %s', host)
                        return host
        raise RuntimeError(f'Unable to find primary in group {group_name}')

    @retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
    def unregister(self, fqdn):
        """
        Unregister fqdn in deploy api
        """
        unregister_result = self.session.post(
            urllib.parse.urljoin(self.deploy_url, f'v1/minions/{fqdn}/unregister'), headers=self.deploy_headers
        )
        unregister_result.raise_for_status()

    def reinit_container(self, fqdn):
        """
        Initialize container on another dom0
        """
        transfer_init_res = self.session.post(
            urllib.parse.urljoin(self.dbm_url, f'/api/v2/containers/{fqdn}'),
            headers=self.dbm_headers,
            json={'dom0': None},
        )
        transfer_init_res.raise_for_status()
        transfer_id = transfer_init_res.json()['transfer']
        finish_res = self.session.post(
            urllib.parse.urljoin(self.dbm_url, f'/api/v2/transfers/{transfer_id}/finish'), headers=self.dbm_headers
        )
        finish_res.raise_for_status()
        secrets = {
            '/etc/yandex/mdb-deploy/deploy_version': {
                'mode': '0644',
                'content': '2',
            },
            '/etc/yandex/mdb-deploy/mdb_deploy_api_host': {
                'mode': '0644',
                'content': urllib.parse.urlparse(self.deploy_url).netloc,
            },
        }
        create_res = self.session.post(
            urllib.parse.urljoin(self.dbm_url, f'/api/v2/containers/{fqdn}'),
            headers=self.dbm_headers,
            json={'secrets': secrets},
        )
        create_res.raise_for_status()
        self.wait_shipment(create_res.json()['deploy']['deploy_id'])

    @retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
    def run_highstate(self, fqdn, primary):
        """
        Run highstate on fqdn with init from primary
        """
        shipment_data = {
            'commands': [
                {
                    'type': 'saltutil.sync_all',
                    'arguments': [],
                    'timeout': 600,
                },
                {
                    'type': 'state.highstate',
                    'arguments': ['pillar={pillar}'.format(pillar=json.dumps({'pg-master': primary})), 'queue=True'],
                    'timeout': 24 * 3600,
                },
            ],
            'fqdns': [fqdn],
            'parallel': 1,
            'stopOnErrorCount': 1,
            'timeout': 24 * 3600 + 600,
        }

        self.logger.info('Running hs pillar={pg-master: %s} on %s', primary, fqdn)
        shipment_result = self.session.post(
            urllib.parse.urljoin(self.deploy_url, 'v1/shipments'), headers=self.deploy_headers, json=shipment_data
        )
        shipment_result.raise_for_status()
        shipment_id = shipment_result.json()['id']
        if not self.wait_shipment(shipment_id):
            raise RuntimeError('Shipment %s failed', shipment_id)

    def resetup(self, fqdn):
        """
        Resetup host
        """
        if self.check_availability(fqdn):
            raise RuntimeError(f'{fqdn} is available via ssh!')
        primary = self.find_primary(fqdn)
        self.unregister(fqdn)
        self.reinit_container(fqdn)
        self.run_highstate(fqdn, primary)


def main():
    """
    Console entry point
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', '--config', type=str, help='path to config file', default=os.path.expanduser('~/.s3_resetup.conf')
    )
    parser.add_argument('fqdn', type=str, help='Target fqdn')
    args = parser.parse_args()
    resetup = S3Resetup(args.config)
    resetup.resetup(args.fqdn)
