# -*- encoding: utf-8
"""
Volume-related functions
"""

import os

from flask import abort, jsonify

from .common import parse_bool, parse_int, parse_query_string
from .db import execute


def get_volumes(query_string):
    """
    Get list of volumes with filter, limit and offset
    defined by query_string
    """
    query_args = {
        'limit': 50,
        'offset': 0,
    }
    volume_keys = {
        'limit': parse_int,
        'offset': parse_int,
        'container': str,
        'dom0': str,
        'path': str,
        'dom0_path': str,
        'backend': str,
        'read_only': parse_bool,
        'space_guarantee': parse_int,
        'space_limit': parse_int,
        'inode_guarantee': parse_int,
        'inode_limit': parse_int,
        'pending_backup': parse_bool,
    }

    parsed_string = parse_query_string(query_string, volume_keys)

    query_args.update(parsed_string)

    for key in volume_keys:
        query_args[key] = query_args.get(key)

    return execute('get_volumes', **query_args) or []


def get_container_volumes(container):
    """
    Get volumes of container
    """
    return execute('get_container_volumes', container=container) or []


def format_volume(record):
    """
    Properly format volume raw for pillar
    """
    ret = {}
    fields = [
        'path',
        'dom0_path',
        'backend',
        'read_only',
        'space_guarantee',
        'space_limit',
        'inode_guarantee',
        'inode_limit',
        'pending_backup',
    ]
    for field in fields:
        if record[field] is not None:
            ret[field] = record[field]

    return ret


def add_volume(fqdn, dom0, data):
    """
    Add volume (for internal use)
    """
    if data.get('dom0_path', '').startswith('/disks'):
        disk = execute('get_free_disk', dom0=dom0)
        data['disk_id'] = disk[0]['id']
        data['dom0_path'] = os.path.join('/disks', disk[0]['id'], fqdn)
        execute('allocate_disk', fetch=False, id=disk[0]['id'])
    else:
        data['disk_id'] = None
    return execute('insert_volume', fetch=False, dom0=dom0, container=fqdn, **data)


def restore_volume(fqdn, dom0, data, backup_data):
    """
    Restore volume (for internal use)
    """
    if not backup_data:
        res = jsonify({'error': f'Missing backup for volume at {data["path"]}'})
        res.status_code = 400
        abort(res)

    if backup_data['pending_delete']:
        res = jsonify({'error': f'Backup for volume at {data["path"]} is undergoing deletion process on dom0'})
        res.status_code = 400
        abort(res)

    if data.get('dom0_path', '').startswith('/disks') and not backup_data.get('disk_id'):
        res = jsonify({'error': f'Volume at {data["path"]} requires disk_id but backup has no one'})
        res.status_code = 400
        abort(res)

    if backup_data['disk_id']:
        data['disk_id'] = backup_data['disk_id']
        data['dom0_path'] = backup_data['dom0_path']
        execute('allocate_disk', fetch=False, id=data['disk_id'])
    else:
        data['disk_id'] = None

    for field in ('dom0_path', 'disk_id', 'space_limit'):
        if data[field] != backup_data[field]:
            res = jsonify(
                {
                    'error': (
                        f'Volume at {data["path"]} {field} has value '
                        f'{data[field]} but backup has {backup_data[field]}'
                    ),
                }
            )
            res.status_code = 400
            abort(res)
    execute('insert_volume', fetch=False, dom0=dom0, container=fqdn, **data)
    execute('delete_volume_backups', fetch=False, container=fqdn, dom0=dom0, path=data['path'])
