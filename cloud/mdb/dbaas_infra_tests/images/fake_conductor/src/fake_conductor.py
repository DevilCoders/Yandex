#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from copy import deepcopy
from random import SystemRandom

from flask import abort, jsonify, make_response
from marshmallow.validate import Equal
from webargs.fields import Boolean, Integer, List, Nested, Str
from webargs.flaskparser import use_args

from server_mock import ServerMock, check_auth

APP = ServerMock('fake_conductor')

DCS = {}
GROUPS = {}
HOSTS = {}


@APP.reset_handler
def drop_state():
    """
    Cleanup all in-memory state
    """
    for i in [DCS, GROUPS, HOSTS]:
        i.clear()


@APP.before_request
def init_store():
    """
    Get dcs and groups from config
    """
    if not DCS:
        DCS.update(deepcopy(APP.config['DCS']))

    if not GROUPS:
        for group in APP.config['GROUPS']:
            item = deepcopy(group)
            GROUPS[str(item['id'])] = item
            GROUPS[item['name']] = item


def get_random_id(target):
    """
    Returns random integer (not in target)
    """
    rand = SystemRandom()
    new_id = rand.randint(0, 65535)
    while new_id in target:
        new_id = rand.randint(0, 65535)

    return new_id


@APP.route('/api/v1/groups/<group>', methods=['GET'])
@check_auth
def get_group_info(group):
    group_obj = GROUPS.get(group)
    if group_obj is None:
        return make_response(jsonify({'error': 'Group not found'}), 404)

    return jsonify(format_group(group_obj))


@APP.route('/api/v1/groups/<group>/parent_groups', methods=['GET'])
@check_auth
def get_parent_groups(group):
    group_obj = GROUPS.get(group)
    if group_obj is None:
        return make_response(jsonify({'error': 'Group not found'}), 404)

    parents = [GROUPS[str(pid)] for pid in group_obj['parent_ids']]
    return jsonify({
        'value': [format_group(p) for p in parents],
    })


def format_group(group):
    id = group['id']
    return {
        'id': id,
        'value': {
            'name': group['name'],
            'project': group['project'],
            'parent_groups': {
                'self': '/groups/{0}/parent_groups'.format(id),
            },
        },
    }


@APP.route('/api/v1/groups/', methods=['POST'])
@use_args({
    'group':
        Nested({
            'name': Str(required=True),
            'project': Nested({
                'id': Integer(required=True),
            }),
            'export_to_racktables': Boolean(required=True),
            'parent_ids': List(Integer(), required=True),
        },
               location='json',
               required=True),
})
@check_auth
def create_group(args):
    group = args['group']

    if group['name'] in GROUPS:
        abort(400, 'Group already exists')

    for pid in group['parent_ids']:
        if str(pid) not in GROUPS:
            abort(400, 'Parent group {0} not found'.format(pid))

    existing_ids = set(g['id'] for g in GROUPS.values())
    group['id'] = get_random_id(existing_ids)

    GROUPS[str(group['id'])] = group
    GROUPS[group['name']] = group

    return make_response(b'', 201)


@APP.route('/api/v1/groups/<group>', methods=['PUT'])
@use_args({
    'group': Nested({
        'parent_ids': List(Integer(), required=True),
    }, location='json', required=True),
})
@check_auth
def change_group(args, group):
    group_obj = GROUPS.get(group)
    if group_obj is None:
        return make_response(jsonify({'error': 'Group not found'}), 404)

    opts = args['group']

    for pid in opts['parent_ids']:
        if str(pid) not in GROUPS:
            abort(400, 'Parent group {0} not found'.format(pid))

    group_obj.update(opts)

    return make_response(b'', 200)


@APP.route('/api/v1/groups/<group>', methods=['DELETE'])
@check_auth
def delete_group(group):
    group_obj = GROUPS.get(group)
    if group_obj is None:
        return make_response(jsonify({'error': 'Group not found'}), 404)

    if group_obj['id'] in (h['group']['id'] for h in HOSTS.values()):
        abort(400, 'Group has hosts')

    del GROUPS[str(group_obj['id'])]
    del GROUPS[group_obj['name']]

    return make_response(b'', 200)


@APP.route('/api/v1/datacenters/<geo>', methods=['GET'])
@check_auth
def get_dc(geo):
    if geo not in DCS:
        return make_response(jsonify({'error': 'Datacenter not found'}), 404)

    return jsonify({'id': DCS[geo]})


@APP.route('/api/v1/hosts/<host>', methods=['GET'])
@use_args({
    'format': Str(validate=Equal('json'), location='query', required=True),
})
@check_auth
def get_host_info(_, host):
    if host in HOSTS:
        return jsonify(HOSTS[host])

    return make_response(jsonify({'error': 'Host not found'}), 404)


@APP.route('/api/v1/hosts/', methods=['POST'])
@use_args({
    'host':
        Nested({
            'fqdn': Str(required=True),
            'short_name': Str(required=True),
            'group': Nested({
                'id': Integer(required=True),
            }),
            'datacenter': Nested({
                'id': Integer(required=True),
            }),
        },
               location='json',
               required=True),
})
@check_auth
def create_host(args):
    host = args['host']

    groups = set(g['id'] for g in GROUPS.values())
    if host['group']['id'] not in groups:
        abort(400, 'Group not found')

    if host['datacenter']['id'] not in DCS.values():
        abort(400, 'Datacenter not found')

    if host['fqdn'] in HOSTS:
        abort(400, 'Host already exists')

    ins_dict = {
        'fqdn': host['fqdn'],
        'short_name': host['short_name'],
        'group': host['group'],
        'value': {
            'group': {
                'self': '/groups/{gid}'.format(gid=host['group']['id']),
            },
        },
    }

    ins_dict['datacenter'] = [key for key, value in DCS.items() if value == host['datacenter']['id']][0]

    ins_dict['id'] = get_random_id([x['id'] for x in HOSTS.values()])

    HOSTS[host['fqdn']] = ins_dict

    return make_response(b'', 201)


@APP.route('/api/v1/hosts/<host>', methods=['PUT'])
@use_args({
    'host': Nested({
        'group': Nested({
            'id': Integer(required=True),
        }),
    }, location='json', required=True),
})
@check_auth
def change_host(args, host):
    if host not in HOSTS:
        return make_response(jsonify({'error': 'Host not found'}), 404)

    opts = args['host']

    groups = set(g['id'] for g in GROUPS.values())
    if opts['group']['id'] not in groups:
        abort(400, 'Group not found')

    HOSTS[host].update(opts)

    return make_response(b'', 200)


@APP.route('/api/v1/hosts/<host>', methods=['DELETE'])
@check_auth
def delete_host(host):
    if host not in HOSTS:
        return make_response(jsonify({'error': 'Host not found'}), 404)

    del HOSTS[host]

    return make_response(b'', 200)


if __name__ == '__main__':
    APP.run(host='0.0.0.0', debug=True)
