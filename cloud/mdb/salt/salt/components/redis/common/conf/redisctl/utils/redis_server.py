#!/usr/bin/env python
# -*- coding: utf-8 -*-
from .config import get_sentinel_config_option, get_sharded_config_lines
from .process import get_ip, get_pids


def get_redis_pids(config):
    pidfile = config.get_redisctl_options('pidfile')
    return get_pids(pidfile)


def get_info_fields(conn, fields):
    info = conn.info()
    return [info.get(field) for field in fields]


def is_master(conn):
    """
    Check if the host has role 'master'.
    """
    role = get_info_fields(conn, ['role'])[0]
    return role, role == 'master'


def check_master(data):
    """
    Check result of is_master func
    :param data:
    :return:
    """
    _, is_master_flag = data
    return is_master_flag


def is_sharded(dbaas_conf):
    return len(dbaas_conf['cluster_hosts']) > len(dbaas_conf['shard_hosts'])


def is_master_in_config(dbaas_conf, config):
    sentinel_conf, cluster_conf_path = config.get_redisctl_options('sentinel_conf', 'cluster_conf_path')
    if is_sharded(dbaas_conf):
        res = is_master_in_sentinel_config(sentinel_conf)
    else:
        res = is_master_in_sharded_config(cluster_conf_path)
    return res


def is_single_noded(dbaas_conf):
    return len(dbaas_conf['shard_hosts']) == 1


def is_master_in_sentinel_config(conf_path):
    """
    Check if local sentinel config monitors current host IP as master IP
    :return:
    """
    # sentinel monitor lom-stable 2a02:6b8:c1b:3411:0:1589:1e8d:d6f4 6379 2
    master_ip = get_sentinel_config_option('sentinel monitor', config_path=conf_path, index=3)
    host_ip = get_ip()
    return host_ip == master_ip


def is_master_in_sharded_config(conf_path):
    """
    Check if local sharded config monitors current host IP as master IP
    :return:
    """
    # d181a11d921876322d63d9eda93e03bfcc279354 2a02:6b8:c14:7419:0:1589:6934:4f4e:6379@16379 myself,master - 0 1639814480000 1 connected 0-5460
    for line in get_sharded_config_lines(config_path=conf_path):
        if 'myself,master' in line:
            return True
    return False
