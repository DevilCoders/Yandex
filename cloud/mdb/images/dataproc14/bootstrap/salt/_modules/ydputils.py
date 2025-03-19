# -*- coding: utf-8 -*-
"""
Module to provide methods for working with pillar through metadata
:platform: all
"""

from __future__ import absolute_import

import logging

log = logging.getLogger(__name__)

try:
    import sys
    import requests
    import retrying
    MODULES_OK = True
except ImportError:
    MODULES_OK = False


__virtualname__ = 'ydputils'


ROLE_MASTERNODE = 'hadoop_cluster.masternode'
ROLE_DATANODE = 'hadoop_cluster.datanode'
ROLE_COMPUTENODE = 'hadoop_cluster.computenode'

NODEMANAGER_ROLES = [ROLE_DATANODE, ROLE_COMPUTENODE]
MiB = 1024 * 1024


def __virtual__():
    """
    Determine whether or not to load this module
    """
    if MODULES_OK:
        return __virtualname__
    return False


class DataprocException(Exception):
    pass


class DataprocWrongPillarException(DataprocException):
    pass


def node_roles(fqdn):
    """
    Function returns list of roles for fqdn
    """
    if fqdn is None:
        raise Exception('Incorrect fqdn {fqdn}'.format(fqdn=fqdn))
    topology = __salt__['pillar.get']('data:topology', None)
    if topology is None:
        raise Exception('Topology is empty, check your pillar')
    roles = []
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if fqdn in subcluster['hosts']:
            roles.append(subcluster['role'].replace('hadoop_cluster.', ''))
    if not roles:
        return ['computenode']
    return roles


def roles():
    """
    Function returns list of roles for current instance
    """
    return node_roles(__salt__['grains.get']('dataproc:fqdn', None))


def check_roles(available_roles):
    """
    Function returns boolean if intersection of current node roles and
    available roles are not empty
    """
    node_roles = set(roles())
    available_roles = set(available_roles)
    return len(available_roles.intersection(node_roles)) > 0


def is_masternode():
    """
    Function returns True if current node has role masternode
    """
    return check_roles(['masternode'])


def is_datanode():
    """
    Function returns True if current node has role datanode
    """
    return check_roles(['datanode'])


def is_computenode():
    """
    Function returns True if current node has role computenode
    """
    return check_roles(['computenode'])


def is_instance_group_node():
    """
    Function returns True if current node is a part of instance
    """
    return bool(__salt__['pillar.get']('data:instance:instance_group_id', None))


def get_datanodes():
    """
    Function retuns list of datanodes fqdns
    """
    topology = __salt__['pillar.get']('data:topology', None)
    datanodes = []
    if topology is None:
        raise Exception('Topology is empty, check your pillar')
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] == ROLE_DATANODE:
            datanodes.extend(subcluster['hosts'])
    if not datanodes:
        raise Exception('Not found datanodes in topology')
    return datanodes


def get_masternodes():
    """
    Function retuns list of masternode fqdns
    """
    topology = __salt__['pillar.get']('data:topology', None)
    if topology is None:
        raise Exception('Topology is empty, check your pillar')
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] == ROLE_MASTERNODE:
            return subcluster['hosts']
    raise Exception('Not found masternodes in topology')


def get_nodemanagers_count():
    """
    Function calculate count of all nodemanagers.
    This count needs for calculation parameters for yarn and mapred properties
    on ResourceManager.
    """
    topology = __salt__['pillar.get']('data:topology', None)
    if topology is None:
        raise Exception('Topology is empty, check your pillar')
    NM_ROLES = {ROLE_DATANODE, ROLE_COMPUTENODE}
    node_managers = set()
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] in NM_ROLES:
            node_managers.update(subcluster.get('hosts', {}))
    if not node_managers:
        raise Exception("Not found hosts with YARN nodemanagers")
    return len(node_managers)


def pkg_download_list(package_set):
    """
    Function create line of packages with versions for apt download command
    Example input:
        masternode:
            hadoop-hdfs-namenode: 2.8.5
            hadoop-common: any
        datanode:
            hadoop-hdfs-datanode: 2.8.5
            hadoop-common: any
    Example output:
        hadoop-hdfs-namenode=2.8.5 hadoop-hdfs-datanode=2.8.5 hadoop-common
    """
    packages = {}
    for label, dct in package_set.items():
        for package, version in dct.items():
            if package not in packages:
                packages[package] = version
            else:
                if packages[package] != version:
                    raise Exception(
                        'multiple version for one package {package}'
                        ': {v1} vs {v2}?'.format(
                            package=package, v1=package[package], v2=version))
    ret = []
    for pkg, ver in packages.items():
        if ver == 'any':
            ret.append(pkg)
        else:
            ret.append('{pkg}={ver}'.format(pkg=pkg, ver=ver))
    return ' '.join(ret)


def pkg_install_dict(package_set):
    """
    Returns dict of packages to install for my roles
    Example input:
        package_set:
            masternode:
                hadoop-hdfs-namenode: 2.8.5
                hadoop-common: any
            datanode:
                hadoop-hdfs-datanode: 2.8.5
                hadoop-common: any
        roles: [masternode, datanode]
    Example output:
            hadoop-hdfs-namenode: 2.8.5
            hadoop-hdfs-datanode: 2.8.5
            hadoop-common: any
    """
    ret = {}
    for role in roles():
        if role not in package_set:
            continue
        for pkg, ver in package_set.get(role).items():
            if ret.get(pkg) is None:
                ret[pkg] = ver
            else:
                if ret[pkg] != ver:
                    raise Exception(
                        'multiple version for one package {pkg}:'
                        '{v1} vs {v2}?'.format(pkg=pkg, v1=ret[pkg], v2=ver))
    return ret


def is_presetup():
    """
    Returns:
      * true if we should only prepare environment for installing
      * false if we should install all
    """
    return __salt__['pillar.get']('data:presetup', False)


def round_down(value, increment, minimum=None, maximum=None):
    """
    Method rounding down value by increment.
    """
    if not minimum:
        minimum = increment
    if not maximum:
        maximum = sys.maxint
    rounded = value - (value % increment)
    return int(min(maximum, max(rounded, minimum)))


def fqdn_ipv4():
    """
    Returns list of announced ipv4 addresses except localhost
    """
    return list(filter(lambda addr: not addr.startswith('127.0.'), __salt__['grains.get']('dataproc:fqdn_ip4', [])))


def get_yarn_sched_min_allocation_mb(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Find yarn node managers with minimum memory and return min_allocation for them
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    node_with_min_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid']))
            if node_with_min_memory is None or node_with_min_memory >= memory:
                node_with_min_memory = memory
    if node_with_min_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = __salt__['pillar.get']('data:properties:dataproc:nodemanager_available_memory_ratio', 0.8)  # noqa
    return round_down(node_with_min_memory * nodemanager_available_memory_ratio / MiB, 256, minimum=256, maximum=1024)


def get_yarn_sched_max_allocation_mb(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Find yarn node managers with maximum memory and return max_allocation for them
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    # Find yarn nodes with maximum memory
    node_with_max_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid']))
            if node_with_max_memory is None or memory >= node_with_max_memory:
                node_with_max_memory = memory
    if node_with_max_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = __salt__['pillar.get']('data:properties:dataproc:nodemanager_available_memory_ratio', 0.8)  # noqa
    return round_down(node_with_max_memory * nodemanager_available_memory_ratio / MiB, get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio))


def get_yarn_sched_max_allocation_vcores(subclusters=None):
    """
    Find yarn node managers with max vcores and return vcores for max_allocation
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    # Find yarn nodes with maximum memory
    max_cores = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            cores = subcluster.get('resources', {}).get('cores')
            if cores is None:
                raise DataprocWrongPillarException(
                    "Not found cores resources for subcluster: {subcid}".format(subcid=subcluster['subcid']))
            if max_cores is None or cores >= max_cores:
                max_cores = cores
    if max_cores is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")

    return max_cores


def get_yarn_sched_max_allocation_mb_for_all_instances(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Formula: round( { min(memory of all node manager) * available_memory_ratio (0.8)}, step=get_yarn_sched_min_allocation_mb)
    Find max_allocation that will works for all of node managers
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    min_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid']))
            if min_memory is None or memory <= min_memory:
                min_memory = memory
    if min_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = __salt__['pillar.get']('data:properties:dataproc:nodemanager_available_memory_ratio', 0.8)  # noqa
    return round_down(min_memory * nodemanager_available_memory_ratio / MiB, get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio))


def get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters=None):
    """
    Find maximum vcores that will works for all yarn node managers
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    min_cores = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            cores = subcluster.get('resources', {}).get('cores')
            if cores is None:
                raise DataprocWrongPillarException(
                    "Not found cores resources for subcluster: {subcid}".format(subcid=subcluster['subcid']))
            if min_cores is None or cores <= min_cores:
                min_cores = cores
    if min_cores is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")

    return min_cores


def get_mapreduce_map_cores(subclusters=None, cores_per_map_task=None):
    """
    Return how many cores will be allocation for map task
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_map_task is None:
        cores_per_map_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_map_task', 1)
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_map_task, 1, minimum=1, maximum=available_cores)


def get_mapreduce_map_memory_mb(subclusters=None, cores_per_map_task=None, nodemanager_available_memory_ratio=None):
    """
    Return memory settings for mappers, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_map_task is None:
        cores_per_map_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_map_task', 1)
    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    max_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    map_memory = max_memory * cores_per_map_task / max_cores
    min_mem_allocation = get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    return round_down(map_memory, min_mem_allocation, maximum=max_memory)


def get_mapreduce_reduce_cores(subclusters=None, cores_per_reduce_task=None):
    """
    Return how many cores will be allocation for reduce tasks
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_reduce_task is None:
        cores_per_reduce_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_reduce_task', 1)
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_reduce_task, 1, minimum=1, maximum=available_cores)


def get_mapreduce_reduce_memory_mb(subclusters=None, cores_per_reduce_task=None, nodemanager_available_memory_ratio=None):
    """
    Return memory settings for reducers, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_reduce_task is None:
        cores_per_reduce_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_reduce_task', 2)
    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    max_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    map_memory = max_memory * cores_per_reduce_task / max_cores
    min_mem_allocation = get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    return round_down(map_memory, min_mem_allocation, maximum=max_memory)


def get_mapreduce_appmaster_cores(subclusters=None, cores_per_app_master=None):
    """
    Return how many cores will be allocation for application masters
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_app_master is None:
        cores_per_app_master = __salt__['pillar.get']('data:properties:dataproc:cores_per_app_master', 1)
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_app_master, 1, minimum=1, maximum=available_cores)


def get_mapreduce_appmaster_memory_mb(subclusters=None, cores_per_app_master=None, nodemanager_available_memory_ratio=None):
    """
    Return memory settings for application masters, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if cores_per_app_master is None:
        cores_per_app_master = __salt__['pillar.get']('data:properties:dataproc:cores_per_app_master', 2)
    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    max_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    map_memory = max_memory * cores_per_app_master / max_cores
    min_mem_allocation = get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    return round_down(map_memory, min_mem_allocation, maximum=max_memory)


def get_spark_executor_cores(subclusters=None, spark_executors_per_vm=None):
    """
    Return how many cores will be allocated for one executor
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if spark_executors_per_vm is None:
        spark_executors_per_vm = __salt__['pillar.get']('data:properties:dataproc:spark_executors_per_vm', 2)
    return max(1, int(get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters) / spark_executors_per_vm))


def get_spark_executor_memory_mb(subclusters=None, spark_executors_per_vm=None, nodemanager_available_memory_ratio=None):
    """
    Return memory for spark executor
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if spark_executors_per_vm is None:
        spark_executors_per_vm = __salt__['pillar.get']('data:properties:dataproc:spark_executors_per_vm', 2)
    spark_executor_cores = get_spark_executor_cores(subclusters, spark_executors_per_vm)
    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    min_mem_allocation = get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    spark_executor_mem = round_down(int(max_memory / spark_executor_cores), min_mem_allocation)

    # The amount of off-heap memory to be allocated per executor, in MiB unless otherwise specified.
    # This is memory that accounts for things like VM overheads, interned strings, other native overheads, etc.
    # This tends to grow with the executor size (typically 6-10%).
    overhead_mem_mb = max(384, int(spark_executor_mem / 10))
    spark_executor_mem = spark_executor_mem - overhead_mem_mb
    return spark_executor_mem


def get_spark_driver_mem_mb(subclusters=None, spark_driver_memory_fraction=None, nodemanager_available_memory_ratio=None):
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')
    if spark_driver_memory_fraction is None:
        spark_driver_memory_fraction = __salt__['pillar.get']('data:properties:dataproc:spark_driver_memory_fraction', 0.25)
    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    return int(max_memory * spark_driver_memory_fraction)


def get_spark_driver_result_size_mem_mb(subclusters=None, spark_driver_memory_fraction=None, spark_driver_result_size_ratio=None, nodemanager_available_memory_ratio=None):
    if spark_driver_result_size_ratio is None:
        spark_driver_result_size_ratio = __salt__['pillar.get']('data:properties:dataproc:spark_driver_result_size_ratio', 0.5)
    driver_mem_mb = get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
    return int(driver_mem_mb * spark_driver_result_size_ratio)


def get_hdfs_stats():
    """
    Method returns jmx statistics of HDFS NameNode
    """
    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get('http://{}:9870/jmx?qry=Hadoop:service=NameNode,name=NameNodeInfo'.format(fqdn), timeout=5.0)
        r.raise_for_status()
        return r.json()['beans'][0]
    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_ui_proxy_url():
    """
    Method returns address for ui-proxy
    """
    ui_proxy = __salt__['pillar.get']('data:ui_proxy', False)
    ui_proxy_url = __salt__['pillar.get']('data:agent:ui_proxy_url', None)
    cluster_id = __salt__['pillar.get']('data:agent:cid', None)
    if not ui_proxy or not ui_proxy_url or not cluster_id:
        return None
    return ui_proxy_url.replace('https://', 'https://cluster-{}.'.format(cluster_id))


def get_max_concurrent_jobs(reserved_memory_for_services, job_memory_footprint, pillar=None):
    """
    Calculate limit for maximum concurrent jobs number based on masternode hardware resources
    see MDB-12202 for reference
    """
    if not pillar:
        pillar = __salt__['pillar.get']('data')

    properties = pillar.get('properties', {})
    dataproc_properties = properties.get('dataproc', {})
    max_concurrent_jobs = int(dataproc_properties.get('max-concurrent-jobs', 0))
    if max_concurrent_jobs:
        return max_concurrent_jobs
    main_subcluster_id = pillar['subcluster_main_id']
    subclusters = pillar['topology']['subclusters']
    main_subcluster = subclusters[main_subcluster_id]
    masternode_memory_bytes = main_subcluster['resources']['memory']
    masternode_memory_for_jobs = masternode_memory_bytes - reserved_memory_for_services
    max_concurrent_jobs = max(1, int(masternode_memory_for_jobs / job_memory_footprint))
    return max_concurrent_jobs
