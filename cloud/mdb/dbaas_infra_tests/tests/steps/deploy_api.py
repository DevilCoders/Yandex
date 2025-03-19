"""
Steps related to Deploy API.
"""

import json
import logging

import requests
from behave import given

from tests.helpers import deploy_api
from tests.helpers.workarounds import retry


@given('Deploy API is up and running')
@given('up and running Deploy API')
@retry(wait_fixed=2500, stop_max_attempt_number=80)
def wait_for_deploy_api(context):
    """
    Wait until MDB Deploy API is ready to accept incoming requests.
    """
    base_url = deploy_api.get_base_url(context)

    logging.info('Pinging deploy API')
    response = requests.get('{0}/v1/ping'.format(base_url), verify=False)
    response.raise_for_status()
    logging.info('Deploy API is ok')

    group_name = context.conf['deploy']['group']
    master_fqdn = 'salt-master01.{net}'.format(net=context.conf['network_name'])

    # Create test deploy group and salt master
    logging.info('Checking deploy group in deploy API')
    if deploy_api.request(context, 'v1/groups/{group}'.format(group=group_name), method='GET') is None:
        logging.info('Creating deploy group in deploy API')
        data = {
            'Name': group_name,
        }
        deploy_api.request(context, 'v1/groups', method='POST', data=json.dumps(data))

    logging.info('Checking master in deploy API')
    if deploy_api.request(context, 'v1/masters/{fqdn}'.format(fqdn=master_fqdn), method='GET') is None:
        logging.info('Creating master in deploy API')
        data = {
            'fqdn': master_fqdn,
            'group': group_name,
            'isopen': True,
        }
        deploy_api.request(context, 'v1/masters', method='POST', data=json.dumps(data))

    logging.info('Checking master is alive')
    master = deploy_api.request(context, 'v1/masters/{fqdn}'.format(fqdn=master_fqdn), method='GET', deserialize=True)
    if 'isAlive' not in master or not master['isAlive']:
        logging.info('Master info: %r', master)
        raise RuntimeError('Wait for master is opened timed out')

    logging.info('Deploy is ready')
