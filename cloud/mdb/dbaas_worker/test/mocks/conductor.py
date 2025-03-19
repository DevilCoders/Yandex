"""
Simple conductor mock
"""

import json

from httmock import urlmatch

from .utils import handle_http_action, http_return


def format_group(group):
    """
    Make conductor group more conductor-like
    """
    return {
        'id': group['id'],
        'value': {
            'name': group['name'],
            'project': group['project'],
            'parent_groups': {
                'self': f'/groups/{group["id"]}/parent_groups',
            },
        },
    }


def conductor(state):
    """
    Setup mock with state
    """

    @urlmatch(netloc='conductor.test', method='get', path='/api/v1/groups/[0-9]+/parent_groups')
    def get_parent_groups(url, _):
        group_id = url.path.split('/')[-2]
        ret = handle_http_action(state, f'conductor-parent-groups-get-{group_id}')
        if ret:
            return ret

        group_obj = state['conductor']['groups'].get(group_id)

        if group_obj is None:
            return http_return(code=404, body={'error': 'Group not found'})

        return http_return(
            body={'value': [format_group(state['conductor']['groups'][str(i)]) for i in group_obj['parent_ids']]},
        )

    @urlmatch(netloc='conductor.test', method='get', path='/api/v1/groups/.+')
    def get_group(url, _):
        group_name = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-group-get-{group_name}')
        if ret:
            return ret

        group_obj = state['conductor']['groups'].get(group_name)

        if group_obj is None:
            return http_return(code=404, body={'error': 'Group not found'})

        return http_return(body=format_group(group_obj))

    @urlmatch(netloc='conductor.test', method='POST', path='/api/v1/groups/')
    def create_group(_, request):
        group = json.loads(request.body)['group']
        ret = handle_http_action(state, f'conductor-group-create-{group["name"]}')
        if ret:
            return ret

        if group['name'] in state['conductor']['groups']:
            return http_return(code=400, body=f'Group {group["name"]} already exists')

        for parent_id in group['parent_ids']:
            if str(parent_id) not in state['conductor']['groups']:
                return http_return(code=400, body=f'Parent group {parent_id} not found')

        group_id = max([value['id'] for value in state['conductor']['groups'].values()] + [0]) + 1

        group['id'] = group_id

        state['conductor']['groups'][str(group['id'])] = group
        state['conductor']['groups'][group['name']] = group

        return http_return(code=201)

    @urlmatch(netloc='conductor.test', method='PUT', path='/api/v1/groups/.+')
    def change_group(url, request):
        group_name = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-group-change-{group_name}')
        if ret:
            return ret

        group_obj = state['conductor']['groups'].get(group_name)

        if group_obj is None:
            return http_return(code=404, body={'error': 'Group not found'})

        data = json.loads(request.body)

        for parent_id in data['group']['parent_ids']:
            if str(parent_id) not in state['conductor']['groups']:
                return http_return(code=400, body=f'Parent group {parent_id} not found')

        group_obj.update({'parent_ids': data['group']['parent_ids']})

        return http_return()

    @urlmatch(netloc='conductor.test', method='DELETE', path='/api/v1/groups/.+')
    def delete_group(url, _):
        group_name = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-group-delete-{group_name}')
        if ret:
            return ret

        group_obj = state['conductor']['groups'].get(group_name)

        if group_obj is None:
            return http_return(code=404, body={'error': 'Group not found'})

        if group_obj['id'] in (x['group']['id'] for x in state['conductor']['hosts'].values()):
            return http_return(code=400, body='Group has hosts')

        del state['conductor']['groups'][str(group_obj['id'])]
        del state['conductor']['groups'][group_name]

        return http_return()

    @urlmatch(netloc='conductor.test', method='GET', path='/api/v1/datacenters/.+')
    def get_dc(url, _):
        datacenter_name = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-datacenter-get-{datacenter_name}')
        if ret:
            return ret

        datacenter_id = state['conductor']['dcs'].get(datacenter_name)

        if datacenter_id is None:
            return http_return(code=404, body={'error': 'Datacenter not found'})

        return http_return(body={'id': datacenter_id})

    @urlmatch(netloc='conductor.test', method='GET', path='/api/v1/hosts/.+')
    def get_host(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-host-get-{fqdn}')
        if ret:
            return ret

        host_obj = state['conductor']['hosts'].get(fqdn)

        if not host_obj:
            return http_return(code=404, body={'error': 'Host not found'})

        return http_return(body=host_obj)

    @urlmatch(netloc='conductor.test', method='GET', path='/api/v1/groups/.+/hosts')
    def get_group2hosts(url, _):
        group = url.path.split('/')[-2]
        ret = handle_http_action(state, f'conductor-group2hosts-get-{group}')
        if ret:
            return ret

        hosts = state['conductor']['group2hosts'].get(group)

        if not hosts:
            return http_return(code=404, body={'error': 'Group not found'})

        return http_return(body=hosts)

    @urlmatch(netloc='conductor.test', method='POST', path='/api/v1/hosts/')
    def create_host(_, request):
        host = json.loads(request.body)['host']
        ret = handle_http_action(state, f'conductor-host-create-{host["fqdn"]}')
        if ret:
            return ret

        if str(host['group']['id']) not in state['conductor']['groups']:
            return http_return(code=400, body='Group not found')

        if host['datacenter']['id'] not in state['conductor']['dcs'].values():
            return http_return(code=400, body='Datacenter not found')

        host_obj = {
            'fqdn': host['fqdn'],
            'short_name': host['short_name'],
            'group': host['group'],
            'value': {
                'group': {
                    'self': f'/groups/{host["group"]["id"]}',
                },
            },
        }

        host_obj['datacenter'] = [
            key for key, value in state['conductor']['dcs'].items() if value == host['datacenter']['id']
        ][0]

        host_obj['id'] = max([value['id'] for value in state['conductor']['hosts'].values()] + [0]) + 1

        state['conductor']['hosts'][host['fqdn']] = host_obj

        return http_return(code=201)

    @urlmatch(netloc='conductor.test', method='PUT', path='/api/v1/hosts/.+')
    def change_host(url, request):
        fqdn = url.path.split('/')[-1]
        group_id = json.loads(request.body)['host']['group']['id']
        ret = handle_http_action(state, f'conductor-host-change-{fqdn}')
        if ret:
            return ret

        host_obj = state['conductor']['hosts'].get(fqdn)

        if not host_obj:
            return http_return(code=404, body={'error': 'Host not found'})

        if str(group_id) not in state['conductor']['groups']:
            return http_return(code=400, body='Group not found')

        state['conductor']['hosts'][fqdn]['group']['id'] = group_id

        return http_return()

    @urlmatch(netloc='conductor.test', method='DELETE', path='/api/v1/hosts/.+')
    def delete_host(url, _):
        fqdn = url.path.split('/')[-1]
        ret = handle_http_action(state, f'conductor-host-delete-{fqdn}')
        if ret:
            return ret

        host_obj = state['conductor']['hosts'].get(fqdn)

        if not host_obj:
            return http_return(code=404, body={'error': 'Host not found'})

        del state['conductor']['hosts'][fqdn]

        return http_return()

    for group in state['conductor']['groups'].copy().values():
        state['conductor']['groups'][str(group['id'])] = group

    return (
        get_group2hosts,
        get_parent_groups,
        get_group,
        create_group,
        change_group,
        delete_group,
        get_dc,
        get_host,
        create_host,
        change_host,
        delete_host,
    )
