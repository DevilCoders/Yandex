# -*- encoding: utf-8
"""
Container-related functions
"""

import json
from datetime import datetime, timedelta
from re import match
from uuid import uuid4

from flask import abort, jsonify

from .common import parse_float, parse_int, parse_query_string
from .config import app_config
from .db import execute
from .dom0 import GeoLockNotAvailable, get_dom0_host, select_dom0_host, start_deploy, start_restore_deploy
from .transfers import get_transfer_by_container
from .volume_backups import get_volume_backups
from .volumes import add_volume, format_volume, get_container_volumes, restore_volume


def get_containers(query_string):
    """
    Get list of containers with filter, limit and offset
    defined by query_string
    """
    query_args = {
        'limit': 50,
        'offset': 0,
    }
    container_keys = {
        'limit': parse_int,
        'offset': parse_int,
        'fqdn': str,
        'dom0': str,
        'generation': parse_int,
        'cluster_name': str,
        'cpu_guarantee': parse_float,
        'cpu_limit': parse_float,
        'memory_guarantee': parse_int,
        'memory_limit': parse_int,
        'hugetlb_limit': parse_int,
        'net_guarantee': parse_int,
        'net_limit': parse_int,
        'io_limit': parse_int,
        'extra_properties': json.loads,
        'project_id': str,
        'managing_project_id': str,
    }

    parsed_string = parse_query_string(query_string, container_keys)

    query_args.update(parsed_string)

    for key in container_keys:
        query_args[key] = query_args.get(key, None)

    return execute('get_containers', **query_args) or []


def get_container(fqdn):
    """
    Get container by fqdn
    """
    ret = execute('get_container', fqdn=fqdn)
    if not ret:
        res = jsonify({'error': 'No such container'})
        res.status_code = 404
        abort(res)

    return ret[0]


def select_dom0_and_handle_geo_lock(project, cluster, container):
    """
    Wrapper around select_dom0_host
    Translate GeoLockNotAvailable to 503 error
    """
    try:
        return select_dom0_host(project, cluster, container)
    except GeoLockNotAvailable as exc:
        res = jsonify({'error': 'Another allocation in progress. ' + exc.args[0]})
        res.status_code = 503
        abort(res)
    # that return for pylint, it doesn't known that abort raises
    return None


def handle_overcommit(container, init_deploy=True, old_volumes=None):
    """
    Decide should we move or not
    """
    # Try allocate empty container on our dom0
    dummy = {
        'dom0': container['dom0'],
        'generation': container['generation'],
        'volumes': [],
    }
    config = app_config()
    if select_dom0_host(None, None, dummy) or any(
        match(p, container['fqdn']) for p in config['SKIP_OVERCOMMIT_FQDN_REGEXES']
    ):
        if init_deploy:
            return {
                'deploy': start_deploy(container['dom0'], container['fqdn']),
            }
        return {}

    # We got overcommit
    # Let's try to find new dom0 for our container
    return prepare_transfer(container, old_volumes=old_volumes)


def prepare_transfer(container, dom0=None, old_volumes=None):
    """
    Insert placeholder and and transfer structures
    """
    if not old_volumes:
        old_volumes = {}
    volumes = get_container_volumes(container['fqdn'])
    for volume in volumes:
        volume.pop('container')
        volume.pop('dom0')
    old_dom0 = get_dom0_host(container['dom0'])
    resources = {
        'geo': old_dom0['geo'],
        'generation': container['generation'],
        'volumes': volumes,
    }
    for key in [
        'cpu_guarantee',
        'memory_guarantee',
        'hugetlb_limit',
        'net_guarantee',
        'io_limit',
    ]:
        if container.get(key) is not None:
            resources[key] = container[key]
    if dom0:
        resources['dom0'] = dom0

    new_dom0 = select_dom0_and_handle_geo_lock(old_dom0['project'], container['cluster_name'], resources)

    if not new_dom0:
        res = jsonify({'error': 'Unable to reallocate container'})
        res.status_code = 400
        abort(res)

    placeholder = str(uuid4())
    transfer_id = str(uuid4())
    resources['fqdn'] = placeholder
    resources['dom0'] = new_dom0
    resources['bootstrap_cmd'] = '/bin/false'
    create_container(container['cluster_name'], new_dom0, resources)
    execute(
        'create_transfer',
        fetch=False,
        id=transfer_id,
        src_dom0=old_dom0['fqdn'],
        dest_dom0=new_dom0,
        container=container['fqdn'],
        placeholder=placeholder,
    )
    for volume in volumes:
        execute(
            'create_volume_backup',
            fetch=False,
            container=container['fqdn'],
            path=volume['path'],
            dom0=old_dom0['fqdn'],
            dom0_path=volume['dom0_path'],
            space_limit=old_volumes.get(volume['path'], volume['space_limit']),
            disk_id=volume['disk_id'],
        )
    return {'transfer': transfer_id}


def update_container(fqdn, updated):
    """
    Modify containers in db and initiate ship
    """
    container = get_container(fqdn)
    kwargs = {
        'fqdn': fqdn,
        'cpu_guarantee': None,
        'cpu_limit': None,
        'generation': None,
        'memory_guarantee': None,
        'memory_limit': None,
        'hugetlb_limit': None,
        'net_guarantee': None,
        'net_limit': None,
        'io_limit': None,
        'extra_properties': None,
        'bootstrap_cmd': None,
        'secrets': None,
        'secrets_expire': None,
        'project_id': None,
        'managing_project_id': None,
    }

    for key, value in updated.items():
        kwargs[key] = value
        if value and key != 'dom0':
            container[key] = value

    if kwargs['secrets'] is not None:
        kwargs['secrets_expire'] = datetime.now() + timedelta(hours=1)

    execute('update_container', **kwargs)
    if 'dom0' in updated and container['dom0'] != updated['dom0']:
        return prepare_transfer(container, updated['dom0'])

    return handle_overcommit(container)


def format_container_options(record):
    """
    Get container options in salt-friendly format
    """
    # pylint: disable=too-many-branches
    fqdn = record.pop('fqdn')
    bootstrap_cmd = record.pop('bootstrap_cmd')
    secrets = record.pop('secrets')
    pending_delete = record.pop('pending_delete')
    container_resources = [
        'cpu_guarantee',
        'cpu_limit',
        'io_limit',
        'memory_guarantee',
        'memory_limit',
        'hugetlb_limit',
        'net_guarantee',
        'net_limit',
    ]
    options = {k: v for k, v in record.items() if k in container_resources}

    for param in ('cpu_guarantee', 'cpu_limit'):
        if record[param] is not None:
            if record[param].is_integer():
                options[param] = '%dc' % record[param]
            elif (record[param] * 10).is_integer():
                options[param] = '%.1fc' % record[param]
            else:
                options[param] = '%.2fc' % record[param]

    for param in ('net_guarantee', 'net_limit'):
        if record[param] is not None:
            options[param] = 'default: %d' % record[param]

    remove = []
    for key, value in options.items():
        if value is None:
            remove.append(key)

    for i in remove:
        options.pop(i)

    if record.get('extra_properties', {}):
        for key, value in record['extra_properties'].items():
            options[key] = str(value)

    for param in ('project_id', 'managing_project_id'):
        if record[param] is not None:
            options[param] = record[param]

    for key, value in options.items():
        options[key] = str(value)
    if bootstrap_cmd:
        options['bootstrap_cmd'] = bootstrap_cmd
    if secrets:
        options['secrets'] = secrets
    if pending_delete is not None:
        options['pending_delete'] = pending_delete
    if pending_delete:
        options['delete_token'] = record['delete_token']

    return fqdn, options


def get_pillar(dom0fqdn):
    """
    ext_pillar method for dom0 minion
    """
    ret = {'data': {'porto': {}}}
    res = execute('get_pillar', dom0=dom0fqdn)

    root = ret['data']['porto']

    for record in res:
        if record['fqdn'] not in root:
            fqdn, opts = format_container_options(record)
            root[fqdn] = {'container_options': opts, 'volumes': []}

        root[fqdn]['volumes'].append(format_volume(record))

    #
    # Temporary hack needed for MDB-5836
    #
    res = execute('get_dom0_flags', fqdn=dom0fqdn)
    for record in res:
        ret['data']['use_vlan688'] = record['use_vlan688']

    return ret


def create_cluster(project, cluster):
    """
    Create new cluster if not exists
    """
    execute('upsert_cluster', fetch=False, cluster=cluster, project=project)


def insert_container(cluster, dom0, container):
    """
    Insert new container w/o volumes
    """
    kwargs = {
        'dom0': dom0,
        'fqdn': container['fqdn'],
        'generation': container['generation'],
        'cluster_name': cluster,
        'cpu_guarantee': container.get('cpu_guarantee'),
        'cpu_limit': container.get('cpu_limit'),
        'memory_guarantee': container.get('memory_guarantee'),
        'memory_limit': container.get('memory_limit'),
        'hugetlb_limit': container.get('hugetlb_limit'),
        'net_guarantee': container.get('net_guarantee'),
        'net_limit': container.get('net_limit'),
        'io_limit': container.get('io_limit'),
        'extra_properties': container.get('extra_properties'),
        'bootstrap_cmd': container['bootstrap_cmd'],
        'secrets': container.get('secrets', {}),
        'project_id': container.get('project_id'),
        'managing_project_id': container.get('managing_project_id'),
    }

    execute('insert_container', fetch=False, **kwargs)


def create_container(cluster, dom0, container):
    """
    Create new container in cluster
    """
    insert_container(cluster, dom0, container)

    for volume in container['volumes']:
        volume_kwargs = {
            'space_guarantee': None,
            'space_limit': None,
            'inode_guarantee': None,
            'inode_limit': None,
        }
        volume_kwargs.update(volume)
        add_volume(container['fqdn'], dom0, volume_kwargs)


def restore_container(cluster, container):
    """
    Create new container in cluster
    """
    insert_container(cluster, container['dom0'], container)

    volume_backups = get_volume_backups(f'container={container["fqdn"]};dom0={container["dom0"]}')

    if len(volume_backups) != len(container['volumes']):
        res = jsonify(
            {
                'error': (
                    f'Number of restore ({len(container["volumes"])}) and '
                    f'backup volumes ({len(volume_backups)}) are different'
                ),
            }
        )
        res.status_code = 400
        abort(res)

    backup_map = {x['path']: x for x in volume_backups}

    for volume in container['volumes']:
        volume_kwargs = {
            'space_guarantee': None,
            'space_limit': None,
            'inode_guarantee': None,
            'inode_limit': None,
        }
        volume_kwargs.update(volume)
        restore_volume(container['fqdn'], container['dom0'], volume_kwargs, backup_map.get(volume_kwargs['path']))


def launch_container(project, cluster, container):
    """
    Create container and initiate deploy on dom0
    """
    if execute('get_container', fqdn=container['fqdn']):
        res = jsonify({'error': 'Container already exists'})
        res.status_code = 409
        abort(res)

    if 'generation' not in container:
        container['generation'] = 1

    if container.get('restore') and not container.get('dom0'):
        res = jsonify({'error': 'Restore requires explicitly set dom0'})
        res.status_code = 400
        abort(res)

    if container.get('restore'):
        # Skip volumes (we already account them with backup)
        dummy = {x: y for x, y in container.items() if x != 'volumes'}
        dummy['volumes'] = []
        dom0 = select_dom0_and_handle_geo_lock(project, cluster, dummy)
    else:
        dom0 = select_dom0_and_handle_geo_lock(project, cluster, container)

    if not dom0:
        res = jsonify({'error': 'Unable to allocate container'})
        res.status_code = 400
        abort(res)

    create_cluster(project, cluster)

    if container.get('restore'):
        restore_container(cluster, container)
        return {'deploy': start_restore_deploy(dom0, container['fqdn'])}

    create_container(cluster, dom0, container)
    return {'deploy': start_deploy(dom0, container['fqdn'])}


def destroy_container(container, save_paths):
    """
    Mark containers as pending for delete and lauch deploy on dom0 hosts.
    Salt-call on dom0 host will destroy container, backup paths from
    `save_paths` and call `DELETE /containers/info?delete_from=DB`.

    We should generate delete_marker and store it in mdb.containers.
    It will be used to authentificate drop_container() request.
    """
    get_container(container)
    transfer = get_transfer_by_container(container)
    if transfer:
        res = jsonify(
            {
                'error': 'Active transfer exists: {id}'.format(id=transfer['id']),
            }
        )
        res.status_code = 400
        abort(res)

    if save_paths:
        saved = execute('init_volumes_backup', container=container, paths=tuple(save_paths))
        if len(saved) != len(save_paths):
            res = jsonify({'error': 'Unable to save paths'})
            res.status_code = 400
            abort(res)

    res = execute('mark_container_deleted', fqdn=container, token=str(uuid4()))
    ret = {}
    ret['deploy'] = start_deploy(res[0]['dom0'], container, True)

    return ret


def drop_container(container, token):
    """
    Delete rows about container and corresponding volumes from database
    """
    execute('delete_container_volumes', fetch=False, fqdn=container)
    res = execute('delete_container', fqdn=container, token=token)

    if not res:
        res = jsonify({'error': 'Invalid token'})
        res.status_code = 400
        abort(res)

    return {}
