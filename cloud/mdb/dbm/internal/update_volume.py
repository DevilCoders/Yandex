# -*- encoding: utf-8
"""
Update volume function (separated to avoid cyclic imports)
"""

from flask import abort, jsonify

from .containers import get_container, handle_overcommit, prepare_transfer
from .db import execute
from .volumes import get_container_volumes


def _check_raw_disk_overcommit(size, dom0, disk_id):
    disks = execute('get_dom0_disks', fqdn=dom0)
    for disk in disks:
        limit = disk['max_space_limit']
        if disk['id'] == disk_id and size > limit:
            return True

    return False


def update_volume(container, path, updated, init_deploy=True):
    """
    Modify volumes in db and initiate ship
    """
    raw_container = get_container(container)
    raw_volume = None
    volumes = get_container_volumes(container)
    for volume in volumes:
        if volume['path'] == path:
            raw_volume = volume
            break
    if not raw_volume:
        res = jsonify({'error': 'No such volume'})
        res.status_code = 404
        abort(res)
    size_changed = updated.get('space_limit', updated.get('space_guarantee'))
    old_volumes = {x['path']: x.get('space_limit', x.get('space_guarantee')) for x in volumes}
    if raw_volume['dom0_path'].startswith('/disks') and size_changed:
        if _check_raw_disk_overcommit(size_changed, raw_container['dom0'], raw_volume['disk_id']):
            return prepare_transfer(raw_container, old_volumes=old_volumes)
    kwargs = {
        'container': container,
        'path': path,
        'read_only': None,
        'space_guarantee': None,
        'space_limit': None,
        'inode_guarantee': None,
        'inode_limit': None,
    }

    for key, value in updated.items():
        kwargs[key] = value

    execute('update_volume', **kwargs)
    return handle_overcommit(raw_container, init_deploy, old_volumes=old_volumes)
