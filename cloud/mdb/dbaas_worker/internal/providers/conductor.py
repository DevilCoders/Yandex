"""
Conductor interaction module
"""

from dbaas_common import retry, tracing

from .common import Change
from .http import HTTPClient, HTTPErrorHandler
from ..exceptions import ExposedException


class ConductorError(ExposedException):
    """
    Base conductor error
    """


class ConfigurationError(Exception):
    """
    Configuration error
    """


class ConductorApi(HTTPClient):
    """
    Conductor provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if self._disabled:
            return
        headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.conductor.token),
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self._init_session(
            self.config.conductor.url,
            'api/v1/',
            default_headers=headers,
            error_handler=HTTPErrorHandler(ConductorError),
        )

    @property
    def _disabled(self) -> bool:
        return not self.config.conductor.enabled

    # pylint: disable=no-self-use
    def conductor_group_name_from_id(self, group_id):
        """
        Generate conductor group name from id
        """
        return 'db_{0}'.format(group_id.replace('-', '_'))

    def _datacenter_info(self, name):
        """
        Return datacenter info (empty dict if datacenter not found)
        """
        if self._disabled:
            return {}
        res = self._make_request('datacenters/{name}'.format(name=name), expect=[200, 404])
        if 'not found' in res.get('error', ''):
            return {}
        return res

    def _group_info(self, name):
        """
        Return group info (empty dict if group not found)
        """
        res = self._make_request('groups/{name}'.format(name=name), expect=[200, 404])
        if 'not found' in res.get('error', ''):
            return {}
        return res

    def group_to_hosts(self, group_name: str) -> list:
        """
        Return fqdns in group
        """
        if self._disabled:
            return []
        response_data = self._make_request(f'groups/{group_name}/hosts', expect=[200])
        result = []
        for item in response_data['value']:
            result.append(item['value']['fqdn'])
        return result

    def _group_change(self, name, root):
        """
        Change group
        """
        parent_group = self._group_info(root)
        if not parent_group:
            raise ConfigurationError('Root group {group} not found'.format(group=root))
        self._make_request(
            'groups/{group}'.format(group=name),
            'put',
            data={
                'group': {
                    'parent_ids': [parent_group['id']],
                },
            },
        )

    @retry.on_exception(ConductorError, factor=10, max_wait=60, max_tries=6)
    def _group_check(self, name):
        """
        Special method to ensure that group exists after creation
        """
        self._make_request('groups/{name}'.format(name=name), expect=[200])

    def _group_create(self, name, root):
        """
        Create group
        """
        parent_group = self._group_info(root)
        if not parent_group:
            raise ConfigurationError('Root group {group} not found'.format(group=root))
        self._make_request(
            'groups/',
            'post',
            expect=[201],
            data={
                'group': {
                    'name': name,
                    'project': {
                        'id': parent_group['value']['project']['id'],
                    },
                    'parent_ids': [parent_group['id']],
                    'export_to_racktables': False,
                },
            },
        )
        self._group_check(name)

    def _group_delete(self, name):
        """
        Delete group
        """
        self._make_request('groups/{name}'.format(name=name), 'delete')

    def _host_info(self, fqdn):
        """
        Return host info (empty dict if host not found)
        """
        res = self._make_request('hosts/{fqdn}?format=json'.format(fqdn=fqdn), expect=[200, 404])
        if 'not found' in res.get('error', ''):
            return {}
        return res

    def _host_change(self, fqdn, group):
        """
        Move host to group
        """
        group_info = self._group_info(group)
        if not group_info:
            raise ConfigurationError('Group {group} not found'.format(group=group))
        self._make_request(
            'hosts/{fqdn}'.format(fqdn=fqdn),
            'put',
            data={
                'host': {
                    'group': {
                        'id': group_info['id'],
                    },
                },
            },
        )

    @retry.on_exception(ConductorError, factor=10, max_wait=60, max_tries=6)
    def _host_check(self, fqdn):
        """
        Special method to ensure that host exists after creation
        """
        self._make_request('hosts/{fqdn}?format=json'.format(fqdn=fqdn), expect=[200])

    def _host_create(self, fqdn, geo, group):
        """
        Create host in conductor
        """
        group_info = self._group_info(group)
        if not group_info:
            raise ConductorError('Group {group} not found'.format(group=group))

        host_data = {
            'fqdn': fqdn,
            'short_name': fqdn.split('.')[0],
            'group': {
                'id': group_info['id'],
            },
        }
        mapped_geo = self.config.conductor.geo_map.get(geo, geo)
        # create host without datacenter if that geo present in mapping and mapped to ''
        if mapped_geo:
            dc_info = self._datacenter_info(mapped_geo)
            if not dc_info:
                raise ConfigurationError('Datacenter {geo} not found'.format(geo=geo))
            host_data['datacenter'] = {'id': dc_info['id']}

        self._make_request(
            'hosts/',
            'post',
            expect=[201],
            data={
                'host': host_data,
            },
        )
        self._host_check(fqdn)

    def _host_delete(self, fqdn):
        """
        Delete host from conductor
        """
        self._make_request(
            'hosts/{fqdn}'.format(fqdn=fqdn),
            'delete',
        )

    @retry.on_exception((ConductorError, KeyError), factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Conductor Group Exists')
    def _group_exists(self, name, root):
        tracing.set_tag('conductor.group.name', name)
        tracing.set_tag('conductor.root', root)

        current = self._group_info(name)
        if not current:
            self._group_create(name, root)
        else:
            parent = self._make_request(current['value']['parent_groups']['self'].lstrip('/'))
            if parent['value'][0]['value']['name'] != root:
                self._group_change(name, root)

    def group_exists(self, group_id, root):
        """
        Create or modify existing conductor group
        """
        if self._disabled:
            return
        name = self.conductor_group_name_from_id(group_id)
        self.add_change(
            Change(f'conductor_group.{name}', 'created', rollback=lambda task, safe_revision: self._group_absent(name))
        )
        self._group_exists(name, root)

    def group_has_root(self, group_id, root):
        """
        Modify existing conductor group
        """
        if self._disabled:
            return
        name = self.conductor_group_name_from_id(group_id)
        self.add_change(Change(f'conductor_group.{name}', 'root sync'))

        tracing.set_tag('conductor.group.name', name)
        tracing.set_tag('conductor.root', root)

        current = self._group_info(name)
        parent = self._make_request(current['value']['parent_groups']['self'].lstrip('/'))
        if parent['value'][0]['value']['name'] != root:
            self._group_change(name, root)

    @retry.on_exception(ConductorError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Conductor Host Exists')
    def _host_exists(self, fqdn, geo, group):
        tracing.set_tag('conductor.host.fqdn', fqdn)
        tracing.set_tag('conductor.host.geo', geo)
        tracing.set_tag('conductor.group.name', group)

        current = self._host_info(fqdn)
        if not current:
            self._host_create(fqdn, geo, group)
        else:
            parent = self._make_request(current['value']['group']['self'].lstrip('/'))
            if parent['value']['name'] != group:
                self._host_change(fqdn, group)

    def host_exists(self, fqdn, geo, group_id, group=None):
        """
        Create host (or move existing into group)
        """
        if self._disabled:
            return
        if not group:
            group = self.conductor_group_name_from_id(group_id)
        self.add_change(
            Change(f'conductor_host.{fqdn}', 'created', rollback=lambda task, safe_revision: self._host_absent(fqdn))
        )
        self._host_exists(fqdn, geo, group)

    @retry.on_exception(ConductorError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Conductor Group Absent')
    def _group_absent(self, name):
        tracing.set_tag('conductor.group.name', name)

        current = self._group_info(name)
        if current:
            self._group_delete(name)

    def group_absent(self, group_id):
        """
        Delete group from conductor if exists
        """
        if self._disabled:
            return
        name = self.conductor_group_name_from_id(group_id)
        self.add_change(Change(f'conductor_group.{name}', 'removed'))
        self._group_absent(name)

    @retry.on_exception(ConductorError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Conductor Host Absent')
    def _host_absent(self, fqdn):
        tracing.set_tag('conductor.host.fqdn', fqdn)

        current = self._host_info(fqdn)
        if current:
            self._host_delete(fqdn)

    def host_absent(self, fqdn):
        """
        Delete host from conductor if exists
        """
        if self._disabled:
            return
        self.add_change(Change(f'conductor_host.{fqdn}', 'removed'))
        self._host_absent(fqdn)
