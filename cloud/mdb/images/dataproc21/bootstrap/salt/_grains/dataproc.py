# -*- coding: utf-8 -*-
"""
Custom grains for Yandex.Cloud Data Proc
:platform: all
"""

import requests
import retrying
import logging
import yaml

log = logging.getLogger(__name__)

METADATA_SERVICE = '169.254.169.254'
METADATA_TIMEOUT = 15.0


def fetch_instance_metadata():
    """
    For stability we use metadata service for getting fqdn of current node.
    It's better, because sometimes DNS service could not response.
    """
    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _get_instance_metadata():
        r = requests.get(
            'http://{service}/computeMetadata/v1/'.format(service=METADATA_SERVICE),
            headers={'Metadata-Flavor': 'Google'},
            params={'alt': 'json', 'recursive': True},
            timeout=METADATA_TIMEOUT)
        r.raise_for_status()
        return r.json()

    dataproc_grains = {}
    instance_metadata = _get_instance_metadata()
    instance = instance_metadata['instance']

    dataproc_grains['fqdn'] = instance['hostname']
    dataproc_grains['folder_id'] = instance_metadata['yandex']['folderId']
    if 'user-data' in instance['attributes']:
        try:
            user_data = dataproc_grains['instance'] = yaml.safe_load(instance['attributes']['user-data'])
            if 'data' in user_data and 'instance' in user_data['data']:
                dataproc_grains['instance'] = user_data['data']['instance']
                fqdn = dataproc_grains['instance'].get('fqdn')
                if fqdn:
                    dataproc_grains['fqdn'] = fqdn.rstrip('.')
        except yaml.YAMLError:
            logging.exception('Could not parse user metadata')
    ips = []
    for iface in instance.get('networkInterfaces', []):
        ips.append(iface['ip'])
    if ips:
        dataproc_grains['fqdn_ip4'] = ips

    return {'dataproc': dataproc_grains}
