"""
Simple conductor client
"""

import os
import logging
import urllib.parse
from configparser import ConfigParser

import requests
import retry


class ConductorApiError(RuntimeError):
    pass


class ConductorPreconditionError(RuntimeError):
    pass


class ConductorClient:
    """
    Conductor client
    """

    def __init__(self, config):
        self.config = config
        self.base_url = urllib.parse.urljoin(self.config.get('conductor', 'url'), 'api/v1/')
        self.session = requests.Session()
        adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1)
        parsed = urllib.parse.urlparse(self.base_url)
        self.session.mount(f'{parsed.scheme}://{parsed.netloc}', adapter)
        self.headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.get('conductor', 'token')),
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self.logger = logging.getLogger('api')

    @retry.on_exception(requests.ConnectionError, factor=10, max_wait=60, max_tries=5)
    def _make_request(self, path, method='get', expect=None, data=None):
        """
        Make request to conductor
        """
        if expect is None:
            expect = [200]
        kwargs = {
            'method': method,
            'url': urllib.parse.urljoin(self.base_url, path),
            'headers': self.headers,
            'timeout': (60, 60),
        }
        if data is not None:
            kwargs['json'] = data
        res = self.session.request(**kwargs)
        if res.status_code not in expect:
            raise ConductorApiError(f'Unexpected {method.upper()} {kwargs["url"]} result: {res.status_code} {res.text}')
        return res

    def _datacenter_info(self, name):
        """
        Return datacenter info (empty dict if datacenter not found)
        """
        res = self._make_request('datacenters/{name}'.format(name=name), expect=[200, 404])
        if res.status_code == 200:
            return res.json()
        return {}

    def _project_info(self, name):
        """
        Return project info (empty dict if group not found)
        """
        res = self._make_request('projects/{name}'.format(name=name), expect=[200, 404])
        if res.status_code == 200:
            return res.json()
        return {}


    def _group_info(self, name):
        """
        Return group info (empty dict if group not found)
        """
        res = self._make_request('groups/{name}'.format(name=name), expect=[200, 404])
        if res.status_code == 200:
            return res.json()
        return {}

    def _host_info(self, fqdn):
        """
        Return host info (empty dict if host not found)
        """
        res = self._make_request('hosts/{fqdn}'.format(fqdn=fqdn), expect=[200, 404])
        if res.status_code == 200:
            return res.json()
        return {}

    @retry.on_exception(ConductorApiError, factor=10, max_wait=60, max_tries=5)
    def ensure_host_create(self, fqdn, group, geo):
        """
        Create host in conductor
        """
        host_info = self._host_info(fqdn)
        if host_info:
            self.logger.info(f'Host {fqdn} already created')
            return

        group_info = self._group_info(group)
        if not group_info:
            raise ConductorPreconditionError(f'Group {group} not found')

        data = {
            'host': {
                'fqdn': fqdn,
                'short_name': fqdn.split('.')[0],
                'group': {
                    'id': group_info['id'],
                },
            },
        }

        if geo:
            self.logger.info(f'Use DC {geo} for host creation')

            dc_info = self._datacenter_info(geo)
            if not dc_info:
                raise ConductorPreconditionError(f'Datacenter {geo} not found')
            else:
                data['host'].update({'datacenter': {
                    'id': dc_info['id'],
                }})

        self._make_request(
            'hosts/',
            'post',
            expect=[201],
            data=data,
        )

    @retry.on_exception(ConductorApiError, factor=10, max_wait=60, max_tries=5)
    def ensure_group_create(self, name, project, parent = None):
        """
        Create group in conductor
        """
        group_info = self._group_info(name)
        if group_info:
            self.logger.info(f'Group {name} already created')
            return

        project_info = self._project_info(project)
        if not project_info:
            raise ConductorPreconditionError(f'Project {project} not found')

        data = {
            'group': {
                'name': name,
                'project': {
                    'id': project_info['id'],
                },
            },
        }

        if parent:
            parent_group_info = self._group_info(parent)
            if not parent_group_info:
                raise ConductorPreconditionError(f'Parent group {parent} not found')
            data['group'].update({'parent_ids': [parent_group_info['id']]})

        self._make_request(
            'groups/',
            'post',
            expect=[201],
            data=data,
        )

    @retry.on_exception(ConductorApiError, factor=10, max_wait=60, max_tries=5)
    def ensure_host_delete(self, fqdn):
        """
        Delete host from conductor
        """
        host_info = self._host_info(fqdn)
        if not host_info:
            return
        self._make_request(
            'hosts/{fqdn}'.format(fqdn=fqdn),
            'delete',
        )

    @retry.on_exception(ConductorApiError, factor=10, max_wait=60, max_tries=5)
    def ensure_group_delete(self, name):
        """
        Delete group from conductor
        """
        group_info = self._group_info(name)
        if not group_info:
            return
        self._make_request(
            'groups/{name}'.format(name=name),
            'delete',
        )


def get_config():
    """
    Read config from disk
    """
    config = ConfigParser()
    config.read(os.path.expanduser('~/.conductor.conf'))
    return config
