# -*- encoding: utf-8
"""
Volume-backups-related functions
"""

from flask import abort, jsonify

from .common import parse_int, parse_query_string
from .db import execute
from .dom0 import start_volume_delete


def get_volume_backups(query_string):
    """
    Get list of volume backups with filter, limit and offset
    defined by query_string
    """
    query_args = {
        'limit': 50,
        'offset': 0,
    }
    volume_backup_keys = {
        'limit': parse_int,
        'offset': parse_int,
        'container': str,
        'path': str,
        'dom0': str,
        'dom0_path': str,
        'disk_id': str,
    }

    parsed_string = parse_query_string(query_string, volume_backup_keys)

    query_args.update(parsed_string)

    for key in volume_backup_keys:
        query_args[key] = query_args.get(key)

    return execute('get_volume_backups', **query_args) or []


def delete_volume_backup(dom0, fqdn, path, delete_token=None):
    """
    Initiate volume backup deletion
    """
    container = execute('get_container', fqdn=fqdn)

    if container and container[0]['dom0'] == dom0:
        res = jsonify({'error': 'Container exists'})
        res.status_code = 400
        abort(res)

    res = execute('mark_pending_delete_volume_backup', container=fqdn, path=path, dom0=dom0, delete_token=delete_token)

    if not res:
        res = jsonify({'error': 'Invalid token'})
        res.status_code = 400
        abort(res)

    return {
        'deploy': start_volume_delete(dom0, res[0]['dom0_path'], res[0]['delete_token']),
    }


def drop_volume_backup(dom0, dom0_path, delete_token):
    """
    Delete rows about volume backup from database
    """
    res = execute('drop_volume_backup', dom0=dom0, dom0_path=dom0_path, delete_token=delete_token)

    if not res:
        res = jsonify({'error': 'Invalid token'})
        res.status_code = 400
        abort(res)

    return {}
