# -*- encoding: utf-8
"""
Hypervisor-related functions
"""
import zlib

from psycopg2.errors import LockNotAvailable  # type: ignore
from retrying import retry

from . import generations
from .db import execute, rollback
from .deploy import DeployAPI


class GeoLockNotAvailable(Exception):
    """
    Geo lock not available
    """


def get_dom0_host(fqdn):
    """
    Get dom0 host from db
    """
    res = execute('get_dom0_info', fqdn=fqdn)
    if res:
        return res[0]

    return {}


def update_dom0_host(fqdn, updated):
    """
    Modify dom0 host in db
    """
    kwargs = {
        'fqdn': fqdn,
        'project': updated['project'],
        'geo': updated['geo'],
        'switch': updated.get('switch'),
        'cpu_cores': updated['cpu_cores'],
        'memory': updated['memory'],
        'ssd_space': updated['ssd_space'],
        'sata_space': updated['sata_space'],
        'max_io': updated['max_io'],
        'net_speed': updated['net_speed'],
        'generation': updated['generation'],
    }

    execute('upsert_dom0', fetch=False, **kwargs)
    update_dom0_disks(fqdn, updated['disks'])

    return {}


def update_dom0_disks(fqdn, disks):
    """
    Sync disks with heartbeat info
    """
    db_disks = execute('get_dom0_disks', fqdn=fqdn)

    new_ids = [x['id'] for x in disks]
    old_ids = [x['id'] for x in db_disks]
    with_data = [x['id'] for x in db_disks if x['has_data']]

    delete = [x for x in old_ids if x not in new_ids]
    if delete:
        execute('delete_dom0_disks', fetch=False, dom0=fqdn, ids=tuple(delete))

    for disk in disks:
        # Handle race condition on new container create and heartbeat
        if not disk['has_data'] and disk['id'] in with_data:
            volume = execute('get_volume_by_disk_id', disk_id=disk['id'])
            if volume:
                continue
        execute('upsert_dom0_disk', fetch=False, dom0=fqdn, **disk)


def geo_to_hash(geo):
    """
    Convert geo to hash

    >>> len(set(geo_to_hash(g) for g in ['man', 'vla', 'sas', 'myt', 'iva']))
    5
    """
    return zlib.crc32(geo.encode('utf-8'))


def _lock_geo(geo):
    """
    Lock geo.
    Get locker info if geo is locked
    and we don't get its lock withing lock_timeout
    """
    if not geo:
        return
    geo_id = geo_to_hash(geo)
    try:
        execute('lock_geo', geo_id=geo_id, fetch=False)
    except LockNotAvailable as exc:
        rollback()
        locker = execute('get_geo_locker', geo_id=geo_id)
        info = 'unknown'
        if locker:
            # query return one row
            locker = locker[0]
            info = (
                '{usename}@{datname} pid:{pid} '
                'session({application_name}) '
                "started at '{xact_start}'"
                "in '{state}' state".format(**locker)
            )
        raise GeoLockNotAvailable(
            'Geo {geo} lock not available. Locked by: {info}'.format(
                geo=geo,
                info=info,
            )
        ) from exc


def select_dom0_host(project, cluster, container, switch_aware=True, force_raw_disks=False):
    """
    Iteration-based container scheduler

    1) If dom0 is defined check that dom0
    2) If dom0 is not defined and container requires no raw disks check
       dom0 without raw disks and max flavor reserved resources
       and ssd_space reserve
    3) Same as 2 but without ssd_space reserve
    4) Same as 3 but without resources reserve
    5) If 4 failed or container requires raw disks check dom0
       with raw disks and max flavor reserved resources and ssd_space reserve
    6) Same as 5 but without ssd_space reserve
    7) Last chance - dom0 with raw disks and without reserve consideration

    In cases 2, 3, 5, 6 we select most free dom0.
    In cases 4 and 7 we select most busy dom0 that still fits our container.
    """
    # pylint: disable=unused-argument
    # pylint: disable=too-many-return-statements
    geo = container.get('geo')
    kwargs = {
        'project': project,
        'cluster_name': cluster,
        'geo': geo,
        'generations': generations.CONTAINER_TO_DOM0[container['generation']],
        'cpu': container.get('cpu_guarantee', 0.0),
        'memory': container.get('memory_guarantee', 0) + container.get('hugetlb_limit', 0),
        'net': container.get('net_guarantee', 0),
        'io': container.get('io_limit', 0),
        'ssd': 0,
        'sata': 0,
        'raw_disks_space': 0,
        'raw_disks': 0,
        'max_raw_disk_space': 0,
        'switch_aware': switch_aware,
    }

    for volume in container['volumes']:
        size = volume.get('space_limit', volume.get('space_guarantee', 0))
        if volume['backend'] == 'tmpfs':
            kwargs['memory'] += size
            continue
        if volume['dom0_path'].startswith('/data/'):
            kwargs['ssd'] += size
            continue
        if volume['dom0_path'].startswith('/disks'):
            kwargs['raw_disks_space'] += size
            kwargs['raw_disks'] += 1
            kwargs['max_raw_disk_space'] = max(size, kwargs['max_raw_disk_space'])
            continue
        kwargs['sata'] += size

    _lock_geo(geo)

    if container.get('dom0'):
        res = execute(
            'lock_current_dom0',
            **{
                **kwargs,
                'fqdn': container['dom0'],
            }
        )
        if res:
            return res[0]['dom0']
        return ''

    scenarios = [
        'lock_most_free_dom0_with_space_reserve',
        'lock_most_free_dom0',
        'lock_most_busy_dom0',
    ]
    if kwargs['raw_disks_space'] == 0:
        scenarios = [
            'lock_most_free_dom0_without_raw_disks_with_space_reserve',
            'lock_most_free_dom0_without_raw_disks',
            'lock_most_busy_dom0_without_raw_disks',
        ] + scenarios

    for scenario in scenarios:
        res = execute(scenario, **kwargs)
        if res:
            return res[0]['dom0']
    if switch_aware:
        return select_dom0_host(
            project=project, cluster=cluster, container=container, switch_aware=False, force_raw_disks=force_raw_disks
        )
    if cluster is not None:
        return select_dom0_host(
            project=project,
            cluster=None,
            container=container,
            switch_aware=switch_aware,
            force_raw_disks=force_raw_disks,
        )
    return ''


@retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
def start_deploy(dom0, container, add_delete_flag=False):
    """
    Initiate deploy via request to deploy api
    """
    deploy_api = DeployAPI()
    jid, deploy_info = deploy_api.deploy(dom0, container, add_delete_flag)
    deploy_info.update({'host': dom0, 'deploy_id': jid})
    return deploy_info


@retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
def start_volume_delete(dom0, dom0_path, delete_token):
    """
    Initiate volume delete via request to deploy api
    """
    deploy_api = DeployAPI()
    jid, deploy_info = deploy_api.deploy_volume_backup_delete(dom0, dom0_path, delete_token)
    deploy_info.update({'host': dom0, 'deploy_id': jid})
    return deploy_info


@retry(stop_max_attempt_number=3, wait_random_min=100, wait_random_max=1000)
def start_restore_deploy(dom0, container):
    """
    Initiate container restore via request to deploy api
    """
    deploy_api = DeployAPI()
    jid, deploy_info = deploy_api.deploy_restore(dom0, container)
    deploy_info.update({'host': dom0, 'deploy_id': jid})
    return deploy_info


def update_allow_new_hosts(fqdn, allow_new_hosts, username=None):
    """
    Sync disks with heartbeat info
    """
    if allow_new_hosts:
        username = None
    res = execute(
        'update_dom0_allow_new_hosts', fqdn=fqdn, allow_new_hosts=allow_new_hosts, allow_new_hosts_updated_by=username
    )
    if res:
        return res[0]
    else:
        return None
