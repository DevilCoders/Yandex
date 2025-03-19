# -*- coding: utf-8 -*-
'''
Module to provide MDB-specific MongoDB functionality to Salt

'''
from __future__ import absolute_import, print_function, unicode_literals

# Import python libs
import logging
import os.path

# Import mongodb libs
try:
    import pymongo
    import bson

    assert pymongo
    assert bson
    HAS_MONGODB = True
except ImportError:
    HAS_MONGODB = False

log = logging.getLogger(__name__)
_default = object()

WALG_LOGDIR = '/var/log/wal-g'
WALG_BACKUP_TIMEOUT_MINUTES = 20 * 60  # 20 hours
WALG_PURGE_TIMEOUT_MINUTES = 30

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


def pillar(key, default=_default):
    """
    Like __salt__['pillar.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised regardless of 'pillar_raise_on_missing' option.
    """
    value = __salt__['pillar.get'](key, default=default)
    if value is _default:
        raise KeyError('Pillar key not found: {0}'.format(key))

    return value


def grains(key, default=_default, **kwargs):
    """
    Like __salt__['grains.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised.
    """
    value = __salt__['grains.get'](key, default=default, **kwargs)
    if value is _default:
        raise KeyError('Grains key not found: {0}'.format(key))

    return value


def hostname():
    return grains('id')


def cluster_id():
    return pillar('data:dbaas:cluster_id', None)


def shard_id():
    return pillar('data:dbaas:shard_id', None)


def subcluster_id():
    return pillar('data:dbaas:subcluster_id')


def node_group_id():
    return shard_id() or subcluster_id()


def restart_lock_path(srv=None):
    return os.path.join("mongodb", cluster_id(), node_group_id(), "restart_lock", srv)


def zookeeper_hosts():
    return pillar('data:mongodb:zk_hosts', [])


def shard_hosts():
    return pillar('data:dbaas:shard_hosts', [])


def wait_after_start_secs():
    return pillar('data:mongodb:wait_after_start_secs', 60)


def user_password(user_name):
    return pillar('data:mongodb:users')[user_name]['password']


def wt_cache_bytes():
    return pillar('data:dbaas:flavor:memory_guarantee') // 2


def wt_cache_bytes_for_restore():
    return max(pillar('data:dbaas:flavor:memory_guarantee') // 4, 268435455)


def wt_engine_config_from_cache_bytes(bytes):
    return "cache_size={bytes}".format(bytes=bytes)


def version():
    """
    Get MongoDB full version num and package version
    """
    version = pillar('data:versions:mongodb', None)

    if version is not None:
        pkg_version = version.get('package_version', None)
        edition = version.get('edition', 'default')
        if edition in ['default', '']:
            edition = 'org'
        pkg_suffix = '-{}'.format(edition)

        minor_version = version.get('minor_version', '0.0.0').split('.')
        full_num = 0
        for v in minor_version:
            full_num = full_num * 100 + int(v)
            # minor_version: 5.0.3
            # FullNum:
            # 0
            # 0*100 + 5 = 5
            # 5*100 + 0 = 500
            # 500*100 + 3 = 50003

        return {
            'major_num': full_num // 100,
            'full_num': full_num,
            'pkg_version': pkg_version,
            'pkg_suffix': pkg_suffix,
            'edition': edition,
        }

    # data:versions:mongodb is empty (Delete some day)

    version_pillar = pillar('data:mongodb:version')
    assert version_pillar.get('major_num', None) is not None
    assert version_pillar.get('full_num', None) is not None

    full_num = version_pillar.get('full_num', None)

    # If no specific package version is specified, generate it
    pkg_ver = int(full_num)
    pkg_minor = pkg_ver % 100
    pkg_ver //= 100
    pkg_middle = pkg_ver % 100
    pkg_ver //= 100
    # For example:
    # full_num = 30619
    # pkg_minor = 30619 % 100 = 19
    # pkg_midle = 30619 // 100 % 100 = 306 % 100 = 6
    # pkg_ver = 30619 // 100 // 100 = 306 // 100 = 3
    version_pillar.update(
        {
            'full_num': full_num,
            'pkg_version': version_pillar.get('pkg', "{}.{}.{}".format(pkg_ver, pkg_middle, pkg_minor)),
            'pkg_suffix': '-org',
            'edition': 'org',
        }
    )
    return version_pillar


def version_major_num():
    return int(version()['major_num'])


def walg_logdir():
    return WALG_LOGDIR


def walg_backup_timeout_minutes():
    return WALG_BACKUP_TIMEOUT_MINUTES


def walg_purge_timeout_minutes():
    return WALG_PURGE_TIMEOUT_MINUTES


def backup_service_enabled():
    return pillar('data:backup:use_backup_service', None)


def backup_id():
    return pillar('backup_id')


def backup_name():
    return pillar('backup_name')


def backup_is_permanent():
    return pillar('backup_is_permanent')


def get_primary(service='mongod', **kwargs):
    kwargs['service'] = service
    args = __salt__['mdb_mongodb_helpers.get_mongo_connection_args'](**kwargs)
    return {
        'primary': __salt__['mongodb.find_rs_primary'](args)
    }
