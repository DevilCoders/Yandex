# -*- encoding: utf-8
"""
Deploy specific code
"""
import json
import logging
import uuid
from urllib.parse import urljoin

import requests
from dbaas_common import tracing
from flask import current_app
from retrying import retry

from .config import app_config


class DeployApiError(Exception):
    """
    Unexpected deploy api response error
    """


class DeployAPI:
    """
    Deploy API
    """

    def __init__(self):
        config = app_config()
        self.base_url = urljoin(config['DEPLOY_API_V2_URL'], 'v1/')
        self.salt_env = config.get('SALT_ENV')
        self.deploy_info = {
            'deploy_version': 2,
            'deploy_api_url': self.base_url,
            'deploy_env': config['DEPLOY_ENV'],
        }
        self.ca_path = config['CA_VERIFY']
        self.headers = {
            'Authorization': f'OAuth {config["DEPLOY_API_V2_TOKEN"]}',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.deploy_timeout = config.get('DEPLOY_TIMEOUT', 20 * 60)

    # pylint: disable=dangerous-default-value
    def _base_request(self, path, method, data=None, ext_headers={}):
        """
        Make request to mdb-deploy-api
        """

    @retry(stop_max_attempt_number=3, wait_exponential_multiplier=500, wait_exponential_max=3000)
    def _make_request(self, path, method, data=None):
        """
        Make request to mdb-deploy-api
        """
        request_id = str(uuid.uuid4())
        logger = logging.LoggerAdapter(
            current_app.logger,
            extra={
                'deploy_request_id': request_id,
            },
        )
        headers = dict(self.headers, **{'X-Request-Id': request_id})
        kwargs = {
            'method': method,
            'url': urljoin(self.base_url, path),
            'headers': headers,
            'verify': self.ca_path,
            'timeout': (1, 2),
        }
        if data:
            kwargs['data'] = json.dumps(data)
        res = requests.request(**kwargs)
        logger.info(
            'deploy %s %s %s %s',
            method.upper(),
            kwargs['url'],
            kwargs['data'] if data else '-',
            res.status_code,
        )
        if res.status_code > 300:
            msg = 'deploy V2 unexpected {method} {url} result: {code} {text}'
            msg = msg.format(
                url=kwargs['url'],
                method=method.upper(),
                code=res.status_code,
                text=res.text,
            )
            logger.warning(msg)
            raise DeployApiError(msg)

        logger.info('deploy V2 result: %r, text: %s', res, res.text)
        return res.json()

    @tracing.trace('Deploy Container')
    def deploy(self, dom0, container, add_delete_flag):
        """
        Initiates container deployment
        """
        tracing.set_tag('dbm.dom0.fqdn', dom0)
        tracing.set_tag('dbm.container.fqdn', container)
        tracing.set_tag('deploy.minion.fqdn', dom0)
        tracing.set_tag('deploy.version', 2)

        job_pillar = {'target-container': container}
        if add_delete_flag:
            job_pillar['is-deleting'] = True
        args = ['components.dom0porto.containers', 'concurrent=True']
        if self.salt_env:
            args.append(f'saltenv={self.salt_env}')
        args.append(f'pillar={json.dumps(job_pillar)}')
        data = {
            'commands': [
                {
                    'type': 'state.sls',
                    'arguments': args,
                    'timeout': self.deploy_timeout,
                },
            ],
            'fqdns': [dom0],
            'parallel': 1,
            'stopOnErrorCount': 1,
            'timeout': self.deploy_timeout,
        }
        current_app.logger.debug('deploy V2 shipment with data: %s', data)
        res = self._make_request('shipments', 'post', data)
        return res['id'], self.deploy_info

    @tracing.trace('Deploy Volume Backup Delete')
    def deploy_volume_backup_delete(self, dom0, dom0_path, delete_token):
        """
        Initiates volume backup delete deployment
        """
        tracing.set_tag('dbm.dom0.fqdn', dom0)
        tracing.set_tag('dbm.dom0.path', dom0_path)
        tracing.set_tag('deploy.minion.fqdn', dom0)
        tracing.set_tag('deploy.version', 2)

        job_pillar = {'target-dom0-path': dom0_path, 'delete-token': delete_token}
        args = ['components.dom0porto.delete_volume_backup', 'concurrent=True']
        if self.salt_env:
            args.append(f'saltenv={self.salt_env}')
        args.append(f'pillar={json.dumps(job_pillar)}')
        data = {
            'commands': [
                {
                    'type': 'state.sls',
                    'arguments': args,
                    'timeout': self.deploy_timeout,
                },
            ],
            'fqdns': [dom0],
            'parallel': 1,
            'stopOnErrorCount': 1,
            'timeout': self.deploy_timeout,
        }
        res = self._make_request('shipments', 'post', data)
        return res['id'], self.deploy_info

    @tracing.trace('Deploy Container Restore')
    def deploy_restore(self, dom0, container):
        """
        Initiates container restore deployment
        """
        tracing.set_tag('dbm.dom0.fqdn', dom0)
        tracing.set_tag('dbm.container.fqdn', container)
        tracing.set_tag('deploy.minion.fqdn', dom0)
        tracing.set_tag('deploy.version', 2)

        job_pillar = {'target-container': container}
        pre_args = ['components.dom0porto.restore_container', 'concurrent=True']
        args = ['components.dom0porto.containers', 'concurrent=True']
        if self.salt_env:
            for i in (args, pre_args):
                i.append(f'saltenv={self.salt_env}')
        for i in (args, pre_args):
            i.append(f'pillar={json.dumps(job_pillar)}')
        data = {
            'commands': [
                {
                    'type': 'state.sls',
                    'arguments': pre_args,
                    'timeout': self.deploy_timeout,
                },
                {
                    'type': 'state.sls',
                    'arguments': args,
                    'timeout': self.deploy_timeout,
                },
            ],
            'fqdns': [dom0],
            'parallel': 1,
            'stopOnErrorCount': 1,
            'timeout': 2 * self.deploy_timeout,
        }
        current_app.logger.debug('deploy V2 shipment with data: %s', data)
        res = self._make_request('shipments', 'post', data)
        return res['id'], self.deploy_info
