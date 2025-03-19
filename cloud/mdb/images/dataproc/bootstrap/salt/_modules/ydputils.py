# -*- coding: utf-8 -*-
"""
Module to provide methods for working with pillar through metadata
:platform: all
"""

from __future__ import absolute_import

import logging
from abc import abstractmethod, ABCMeta
from builtins import set
from dataclasses import dataclass
from enum import Enum

log = logging.getLogger(__name__)

try:
    import sys
    import requests
    import retrying
    from ipaddress import IPv4Network

    MODULES_OK = True
except ImportError:
    MODULES_OK = False


__virtualname__ = 'ydputils'


ROLE_MASTERNODE = 'hadoop_cluster.masternode'
ROLE_DATANODE = 'hadoop_cluster.datanode'
ROLE_COMPUTENODE = 'hadoop_cluster.computenode'

NODEMANAGER_ROLES = [ROLE_DATANODE, ROLE_COMPUTENODE]

MASTERNODE_CORES_RATIO = 0.9
MiB = 1024 * 1024

# For arcadia tests, populate __salt__ variable
__salt__ = {}


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
    topology = __salt__['pillar.get']('data:topology', None)  # noqa
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
    return node_roles(__salt__['grains.get']('dataproc:fqdn', None))  # noqa


def get_current_fqdn():
    """
    Function returns fqdn for current instance
    """
    return __salt__['grains.get']('dataproc:fqdn', None)  # noqa


def get_send_states_activated():
    """
    Return True if send states to agent
    """
    default = False
    try:
        return __salt__['pillar.get']('data:properties:dataproc:send_states_activated', default)
    except KeyError:
        return default


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
    return bool(__salt__['pillar.get']('data:instance:instance_group_id', None))  # noqa


def get_datanodes(subclusters=None):
    """
    Function retuns list of datanodes fqdns
    """
    datanodes = []
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa

    for _, subcluster in subclusters.items():
        if subcluster['role'] == ROLE_DATANODE:
            datanodes.extend(subcluster['hosts'])
    return datanodes


def get_masternodes():
    """
    Function retuns list of masternode fqdns
    """
    topology = __salt__['pillar.get']('data:topology', None)  # noqa
    if topology is None:
        raise Exception('Topology is empty, check your pillar')
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] == ROLE_MASTERNODE:
            return subcluster['hosts']
    raise Exception('Not found masternodes in topology')


@dataclass
class Resources:
    cores: int
    memory_mb: int

    @property
    def total_memory_mb(self):  # -> int
        return self.memory_mb


@dataclass
class ContainerResources(Resources):
    memory_overhead_mb: int = 0

    @property
    def total_memory_mb(self):  # -> int
        return self.memory_mb + self.memory_overhead_mb


@dataclass
class NodeResourcesLayout:
    """
    Distribution of node system resources (CPU, RAM) between master application (Spark Driver / Yarn Application Master)
      and regular spark executors (if any).
    Remains valid while sum of assigned resources is not bigger than total resources of node.
    """

    total_resources: Resources
    # Spark driver / Yarn Application Master
    master_resources: ContainerResources
    guaranteed_executor_count: int = 0
    executor_resources: ContainerResources = ContainerResources(cores=0, memory_mb=0)

    def assert_valid(self):  # -> None
        if (
            self.master_resources.cores + self.guaranteed_executor_count * self.executor_resources.cores
            > self.total_resources.cores
        ):
            raise AssertionError(
                f"{self.master_resources.cores} [master cores] "
                f"+ {self.guaranteed_executor_count} [executor count] "
                f"* {self.executor_resources.cores} [executor cores] "
                f"> {self.total_resources.cores} [available cores]"
            )
        if (
            self.master_resources.total_memory_mb
            + self.guaranteed_executor_count * self.executor_resources.total_memory_mb
            > self.total_resources.total_memory_mb
        ):
            raise AssertionError(
                f"{self.master_resources.total_memory_mb} [master memory with overhead, MiB] "
                f"+ {self.guaranteed_executor_count} [executor count] "
                f"* {self.executor_resources.total_memory_mb} [executor memory with overhead, MiB] "
                f"> {self.total_resources.cores} [available memory, MiB]"
            )


class DeployMode(Enum):
    """
    See `spark.submit.deployMode`
    https://spark.apache.org/docs/latest/configuration.html#application-properties

    The deploy mode of Spark driver program, either "client" or "cluster",
    Which means to launch driver program locally ("client") or remotely ("cluster")
      on one of the nodes inside the cluster.
    """

    CLIENT = 'client'
    CLUSTER = 'cluster'


@dataclass
class ClusterResourcesLayout(metaclass=ABCMeta):
    """
    Holder of master-node and compute-node resource layouts, with properties mapped to Spark application properties.
    Two implementations differs based on chosen `deployMode`; see subclasses below for details.
    """

    master_node: NodeResourcesLayout
    compute_node: NodeResourcesLayout
    deploy_mode: DeployMode = None

    @property
    def spark_executor_cores(self):  # -> int
        """
        :return: `spark.executor.cores`

        https://spark.apache.org/docs/latest/configuration.html#application-properties
        The number of cores to use on each executor.
        """
        return self.compute_node.executor_resources.cores

    @property
    def spark_executor_memory_mb(self):  # -> int
        """
        :return: `spark.executor.memory` in MiB

        https://spark.apache.org/docs/latest/configuration.html#application-properties
        Amount of memory to use per executor process.
        """
        return self.compute_node.executor_resources.memory_mb

    @property
    @abstractmethod
    def spark_driver_cores(self):  # -> int
        """
        :return: `spark.driver.cores`

        https://spark.apache.org/docs/latest/configuration.html#application-properties
        Number of cores to use for the driver process, **only in cluster mode**.
        """
        pass

    @property
    @abstractmethod
    def spark_driver_memory_mb(self):  # -> int
        """
        :return: `spark.driver.memory` in MiB

        https://spark.apache.org/docs/latest/configuration.html#application-properties
        Amount of memory to use for the driver process, i.e. where SparkContext is initialized.
        """
        pass

    @property
    def yarn_appmaster_cores(self):  # -> int
        """
        :return: `spark.yarn.am.cores`

        https://spark.apache.org/docs/latest/running-on-yarn.html
        Number of cores to use for the YARN Application Master **in client mode**.
        In cluster mode, use `spark.driver.cores` instead.
        """
        return self.compute_node.master_resources.cores

    @property
    def yarn_appmaster_memory_mb(self):  # -> int
        """
        :return: `spark.yarn.am.memory` in MiB

        https://spark.apache.org/docs/latest/running-on-yarn.html
        Amount of memory to use for the YARN Application Master **in client mode**.
        In cluster mode, use `spark.driver.memory` instead.
        """
        return self.compute_node.master_resources.memory_mb

    @staticmethod
    def _default_memory_overhead_mb(memory_mb):  # -> int
        # memory_mb: int
        """Method that Spark uses for default memory overhead calculations."""
        return max(384, memory_mb // 10)

    @staticmethod
    def _spark_executor_res(
        compute_node_res,
        executors_count,
        min_memory_allocation_mb,
    ):  # -> ContainerResources
        # compute_node_res: Resources,
        # executors_count: int,
        # min_memory_allocation_mb: int,
        """
        Divide given compute-node resources by count of executors,
          align memory by `min_memory_allocation_mb` unit (maybe aligning is legacy).
        """
        per_executor_memory_mb = round_down(
            value=compute_node_res.memory_mb // executors_count,
            increment=min_memory_allocation_mb,
        )
        overhead_mb = ClusterResourcesLayout._default_memory_overhead_mb(per_executor_memory_mb)
        return ContainerResources(
            cores=max(1, compute_node_res.cores // executors_count),
            memory_mb=per_executor_memory_mb - overhead_mb,
            memory_overhead_mb=overhead_mb,
        )


@dataclass
class HeavyweightClusterLayout(ClusterResourcesLayout):
    """
    Cluster with HDFS or other heavy workload on master node.
    In this configuration we default Spark to run Spark Driver on one of compute nodes via `deployMode=cluster`
      and request executor-comparable size of resources for it.
    Also, with `deployMode=cluster`, `spark.yarn.am.*` settings will not be used by Spark,
      so we set them with `spark.driver.*` values.
    """

    deploy_mode: DeployMode = DeployMode.CLUSTER

    @property
    def spark_driver_cores(self):  # -> int
        return self.compute_node.master_resources.cores

    @property
    def spark_driver_memory_mb(self):  # -> int
        return self.compute_node.master_resources.memory_mb

    @classmethod
    def calculate(
        cls,
        master_node_res,
        compute_node_res,
        executors_per_vm,
        min_memory_allocation_mb,
        driver_memory_fraction,
    ):  # -> "HeavyweightClusterLayout"
        # master_node_res: Resources,
        # compute_node_res: Resources,
        # executors_per_vm: int,
        # min_memory_allocation_mb: int,
        # driver_memory_fraction: float,
        """
        Because Spark Driver requested resources are to be **reasonable portion** of compute-node available resources,
          when we calculate executor resources to be requested, we **do not subtract** `yarn_appmaster_res`
          from `compute_node_res` to increase resources utilization on nodes without Spark Driver running
        Example: this way, for `N` compute-node cluster and `k<N` running applications, there would be `N-k` compute
          nodes fully utilized while `k` nodes would be under-utilized by `executor_res - _cluster_spark_driver_res`.
        In this example, we assume `k` and `executor_res - _cluster_spark_driver_res` to be small.
        """
        executor_res = cls._spark_executor_res(
            compute_node_res=compute_node_res,
            executors_count=executors_per_vm,
            min_memory_allocation_mb=min_memory_allocation_mb,
        )
        return cls(
            # Master node resources do not really matter for Spark in `deployMode=cluster` mode.
            master_node=NodeResourcesLayout(
                total_resources=master_node_res,
                master_resources=ContainerResources(0, 0),
            ),
            compute_node=NodeResourcesLayout(
                total_resources=compute_node_res,
                master_resources=cls._cluster_spark_driver_res(
                    driver_memory_fraction=driver_memory_fraction,
                    max_memory_mb=compute_node_res.total_memory_mb,
                    executor_res=executor_res,
                ),
                # -1 for possible Yarn Application Master running on compute node
                guaranteed_executor_count=executors_per_vm - 1,
                executor_resources=executor_res,
            ),
        )

    @staticmethod
    def _cluster_spark_driver_res(driver_memory_fraction, max_memory_mb, executor_res):
        #  driver_memory_fraction: float, max_memory_mb: int, executor_res: Resources
        """
        TODO
          For `deployMode=cluster` Spark Driver will be executed on compute-node, so we should align its
          requested resources with unit of executor requested resources for lower fragmentation
        """
        # driver_memory_mb = max(
        #     int(max_memory_mb * driver_memory_fraction),
        #     executor_res.total_memory_mb,
        # )
        driver_memory_mb = int(max_memory_mb * driver_memory_fraction)
        driver_overhead_mb = ClusterResourcesLayout._default_memory_overhead_mb(driver_memory_mb)
        return ContainerResources(
            cores=executor_res.cores,
            # memory_mb=driver_memory_mb - driver_overhead_mb,
            memory_mb=driver_memory_mb,
            memory_overhead_mb=driver_overhead_mb,
        )


@dataclass
class LightweightClusterLayout(ClusterResourcesLayout):
    """
    Cluster with no HDFS or other heavy workload on master node.
    In this configuration we default Spark to run Spark Driver on master node via `deployMode=client`
      and request all available master-node resources for it.
    Also, with Spark Driver running on client (master-node), Yarn Application Master doesn't need much
      of compute-node resources, so we request only __reasonable amount__ of it, leaving rest for Spark Executors.
    """

    deploy_mode: DeployMode = DeployMode.CLIENT

    @property
    def spark_driver_cores(self):  # -> int
        return self.master_node.master_resources.cores

    @property
    def spark_driver_memory_mb(self):  # -> int
        return self.master_node.master_resources.memory_mb

    @classmethod
    def calculate(
        cls,
        master_node_res,
        compute_node_res,
        executors_per_vm,
        min_memory_allocation_mb,
    ):  # -> "LightweightClusterLayout"
        # master_node_res: Resources,
        # compute_node_res: Resources,
        # executors_per_vm: int,
        # min_memory_allocation_mb: int,
        """
        Because Yarn Application Master requested resources are to be **small portion** of compute-node available resources,
          when we calculate executor resources to be requested, **we subtract** `yarn_appmaster_res`
          from `compute_node_res` to guarantee that on every compute node will be enough resources
          to run `executors_per_vm` units, even if Yarn Application Master is running on that node.
        Example: this way, for `N` compute-node cluster and `k<N` running applications, there would be `N-k` compute
          nodes with `yarn_appmaster_res` under-utilization of resources, while `k` nodes would be fully utilized.
        In this example, we assume `N-k` an `yarn_application_res` to be small.
        """
        yarn_appmaster_res = cls._yarn_appmaster_res()
        compute_executors_res = Resources(
            cores=compute_node_res.cores - yarn_appmaster_res.cores,
            memory_mb=compute_node_res.memory_mb - yarn_appmaster_res.memory_mb,
        )
        return cls(
            master_node=NodeResourcesLayout(
                total_resources=master_node_res,
                master_resources=cls._client_spark_driver_res(master_node_res=master_node_res),
            ),
            compute_node=NodeResourcesLayout(
                total_resources=compute_node_res,
                master_resources=yarn_appmaster_res,
                guaranteed_executor_count=executors_per_vm,
                executor_resources=cls._spark_executor_res(
                    compute_node_res=compute_executors_res,
                    executors_count=executors_per_vm,
                    min_memory_allocation_mb=min_memory_allocation_mb,
                ),
            ),
        )

    @staticmethod
    def _client_spark_driver_res(master_node_res):  # -> ContainerResources
        # master_node_res: Resources
        """For `deployMode=client` Spark Driver will be executed on master-node, so use all of its resources."""
        overhead_mem_mb = ClusterResourcesLayout._default_memory_overhead_mb(master_node_res.memory_mb)
        return ContainerResources(
            cores=master_node_res.cores,
            memory_mb=round_down(master_node_res.memory_mb - overhead_mem_mb, increment=1024),
            memory_overhead_mb=overhead_mem_mb,
        )

    @staticmethod
    def _yarn_appmaster_res():  # -> ContainerResources
        """For `deployMode=client` Yarn Application Master doesn't need much of resources."""
        total_memory_mb = 1024
        overhead_mem_mb = ClusterResourcesLayout._default_memory_overhead_mb(total_memory_mb)
        return ContainerResources(
            cores=1,
            memory_mb=total_memory_mb - overhead_mem_mb,
            memory_overhead_mb=overhead_mem_mb,
        )


def calculate_cluster_resources(subclusters, available_memory_ratio=None, driver_memory_fraction=None):
    # available_memory_ratio: float = None, driver_memory_fraction: float = None
    available_memory_ratio = available_memory_ratio or get_nodemanager_available_memory_ratio()
    driver_memory_fraction = driver_memory_fraction or get_spark_driver_memory_fraction()
    master_node_res = Resources(
        cores=get_max_allocation_cores_for_master(subclusters),
        memory_mb=get_max_allocation_mb_for_master(subclusters, available_memory_ratio),
    )
    compute_node_res = Resources(
        cores=get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters),
        memory_mb=get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, available_memory_ratio),
    )
    executors_per_vm = get_spark_executors_per_vm(subclusters)

    deploy_mode = get_spark_deploy_mode(subclusters)
    if deploy_mode == DeployMode.CLIENT.value:
        result = LightweightClusterLayout.calculate(
            master_node_res=master_node_res,
            compute_node_res=compute_node_res,
            executors_per_vm=executors_per_vm,
            min_memory_allocation_mb=get_yarn_sched_min_allocation_mb(subclusters, available_memory_ratio),
        )
    else:
        result = HeavyweightClusterLayout.calculate(
            master_node_res=master_node_res,
            compute_node_res=compute_node_res,
            executors_per_vm=executors_per_vm,
            min_memory_allocation_mb=get_yarn_sched_min_allocation_mb(subclusters, available_memory_ratio),
            driver_memory_fraction=driver_memory_fraction,
        )
    result.master_node.assert_valid()
    result.compute_node.assert_valid()
    return result


def is_lightweight(subclusters=None):
    """
    Fuction returns True for cluster without datanodes
    and False for others
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa

    for _, subcluster in subclusters.items():
        if subcluster['role'] == ROLE_DATANODE:
            return False
    return True


def get_spark_deploy_mode(subclsuters=None):
    """
    Method returns where spark driver will be executed
    Please, do not use it for spark-cli, pyspark-cli and spark-sql-cli
    """
    default = DeployMode.CLUSTER.value
    if is_lightweight(subclsuters):
        default = DeployMode.CLIENT.value
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.submit.deployMode', default)
    except KeyError:
        return default


def get_s3_bucket():
    """
    Function retuns s3 bucket name
    """
    return __salt__['pillar.get']('data:s3_bucket')  # noqa


def get_cid():
    """
    Function retuns cluster id
    """
    return __salt__['pillar.get']('data:agent:cid')  # noqa


def get_cluster_services():
    """
    Function returns list of services for cluster
    """
    return __salt__['pillar.get']('data:services', [])  # noqa


def get_datanodes_count():
    return len(get_datanodes())


def get_min_computenodes_count():
    """
    minimum possible number of compute nodes
    """

    topology = __salt__['pillar.get']('data:topology', {})  # noqa
    nodes = 0
    node_managers = set()
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] == ROLE_COMPUTENODE:
            autosc = subcluster.get('instance_group_config', {}).get('scale_policy', {})
            min_count = autosc.get('auto_scale', {}).get('initial_size')
            if min_count:
                nodes += min_count
                continue

            node_managers.update(subcluster.get('hosts', {}))
    nodes += len(node_managers)
    return nodes


def get_max_computenodes_count():
    """
    maximum possible number of compute nodes
    """

    topology = __salt__['pillar.get']('data:topology', {})  # noqa
    nodes = 0
    node_managers = set()
    for subcid, subcluster in topology.get('subclusters', {}).items():
        if subcluster['role'] == ROLE_COMPUTENODE:
            autosc = subcluster.get('instance_group_config', {}).get('scale_policy', {})
            max_count = autosc.get('auto_scale', {}).get('initial_size')
            if max_count:
                nodes += max_count
                continue

            node_managers.update(subcluster.get('hosts', {}))
    nodes += len(node_managers)
    return nodes


def get_max_worker_count():
    return get_datanodes_count() + get_max_computenodes_count()


def get_min_worker_count():
    return get_datanodes_count() + get_min_computenodes_count()


def get_nodemanagers_count():
    """
    Function calculate count of all nodemanagers.
    This count needs for calculation parameters for yarn and mapred properties
    on ResourceManager.
    """
    topology = __salt__['pillar.get']('data:topology', None)  # noqa
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
                        ': {v1} vs {v2}?'.format(package=package, v1=package[package], v2=version)
                    )
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
                        'multiple version for one package {pkg}:' '{v1} vs {v2}?'.format(pkg=pkg, v1=ret[pkg], v2=ver)
                    )
    return ret


def is_presetup():
    """
    Returns:
      * true if we should only prepare environment for installing
      * false if we should install all
    """
    return __salt__['pillar.get']('data:presetup', False)  # noqa


def round_down(value, increment, minimum=None, maximum=None):
    """
    Method rounding down value by increment.
    """
    if not minimum:
        minimum = increment
    if not maximum:
        maximum = sys.maxsize
    rounded = value - (value % increment)
    return int(min(maximum, max(rounded, minimum)))


def fqdn_ipv4():
    """
    Returns list of announced ipv4 addresses except localhost
    """
    return list(
        filter(lambda addr: not addr.startswith('127.0.'), __salt__['grains.get']('dataproc:fqdn_ip4', []))
    )  # noqa


def get_default_spark_executors_per_vm(subclusters=None):
    """
    Return a number of spark executors for calculation container sizes
    Try to fill single session on lightweight cluster, otherwize try to fit spark-session on single executor
    """
    return 2 if not is_lightweight(subclusters) else 1


def get_spark_executors_per_vm(subclusters=None):  # -> int
    """
    Return a number of spark executors with user overriding
    """
    default = get_default_spark_executors_per_vm(subclusters)
    try:
        return __salt__['pillar.get']('data:properties:dataproc:spark_executors_per_vm', default)
    except KeyError:
        return default


def get_nodemanager_available_memory_ratio():  # -> float
    default = 0.8
    try:
        return __salt__['pillar.get']('data:properties:dataproc:nodemanager_available_memory_ratio', default)
    except KeyError:
        return default


def get_spark_driver_memory_fraction():  # -> float
    default = 0.25
    try:
        return __salt__['pillar.get']('data:properties:dataproc:spark_driver_memory_fraction', default)
    except KeyError:
        return default


def get_yarn_sched_min_allocation_mb(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Find yarn node managers with minimum memory and return min_allocation for them
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    node_with_min_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid'])
                )
            if node_with_min_memory is None or node_with_min_memory >= memory:
                node_with_min_memory = memory
    if node_with_min_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = get_nodemanager_available_memory_ratio()
    return round_down(node_with_min_memory * nodemanager_available_memory_ratio / MiB, 256, minimum=256, maximum=1024)


def get_yarn_sched_max_allocation_mb(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Find yarn node managers with maximum memory and return max_allocation for them
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    # Find yarn nodes with maximum memory
    node_with_max_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid'])
                )
            if node_with_max_memory is None or memory >= node_with_max_memory:
                node_with_max_memory = memory
    if node_with_max_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = get_nodemanager_available_memory_ratio()
    return round_down(
        node_with_max_memory * nodemanager_available_memory_ratio / MiB,
        get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio),
    )


def get_yarn_sched_max_allocation_vcores(subclusters=None):
    """
    Find yarn node managers with max vcores and return vcores for max_allocation
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    # Find yarn nodes with maximum memory
    max_cores = None
    for _, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            cores = subcluster.get('resources', {}).get('cores')
            if cores is None:
                raise DataprocWrongPillarException(
                    "Not found cores resources for subcluster: {subcid}".format(subcid=subcluster['subcid'])
                )
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
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    min_memory = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            memory = subcluster.get('resources', {}).get('memory')
            if memory is None:
                raise DataprocWrongPillarException(
                    "Not found memory resources for subcluster: {subcid}".format(subcid=subcluster['subcid'])
                )
            if min_memory is None or memory <= min_memory:
                min_memory = memory
    if min_memory is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = get_nodemanager_available_memory_ratio()
    return round_down(
        min_memory * nodemanager_available_memory_ratio / MiB,
        get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio),
    )


def get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters=None):
    """
    Find maximum vcores that will works for all yarn node managers
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    min_cores = None
    for subcid, subcluster in subclusters.items():
        if subcluster['role'] in NODEMANAGER_ROLES:
            cores = subcluster.get('resources', {}).get('cores')
            if cores is None:
                raise DataprocWrongPillarException(
                    "Not found cores resources for subcluster: {subcid}".format(subcid=subcluster['subcid'])
                )
            if min_cores is None or cores <= min_cores:
                min_cores = cores
    if min_cores is None:
        raise DataprocWrongPillarException("Not found subclusters with yarn nodemanagers")

    return min_cores


def get_max_allocation_mb_for_master(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Formula: round( { min(memory of masternode) * available_memory_ratio (0.8)}, step=1024)
    Find max_allocation that will works for masternode
    Method needed for lightweight configuration when SparkDriver will run on masternode
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    master_memory = next(
        subcluster.get('resources', {}).get('memory')
        for subcluster in subclusters.values()
        if subcluster['role'] == ROLE_MASTERNODE
    )
    if not master_memory:
        raise DataprocWrongPillarException("Not found memory resources for masternodes")
    if nodemanager_available_memory_ratio is None:
        nodemanager_available_memory_ratio = get_nodemanager_available_memory_ratio()
    step = round_down(master_memory * nodemanager_available_memory_ratio / MiB, 256, minimum=256, maximum=1024)
    return round_down(master_memory * nodemanager_available_memory_ratio / MiB, step)


def get_max_allocation_cores_for_master(subclusters=None):
    """
    Find cores allocation that will works for masternode
    Method needed for lightweight configuration when SparkDriver will run on masternode
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    masternode_cores = next(
        subcluster.get('resources', {}).get('cores')
        for subcluster in subclusters.values()
        if subcluster['role'] == ROLE_MASTERNODE
    )

    if not masternode_cores:
        raise DataprocWrongPillarException("Not found available cores on masternode")

    return max(1, int(masternode_cores * MASTERNODE_CORES_RATIO))


def get_mapreduce_map_cores(subclusters=None, cores_per_map_task=None):
    """
    Return how many cores will be allocation for map task
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_map_task is None:
        cores_per_map_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_map_task', 1)  # noqa
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_map_task, 1, minimum=1, maximum=available_cores)


def get_mapreduce_map_memory_mb(subclusters=None, cores_per_map_task=None, nodemanager_available_memory_ratio=None):
    """
    Return memory settings for mappers, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_map_task is None:
        cores_per_map_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_map_task', 1)  # noqa
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
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_reduce_task is None:
        cores_per_reduce_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_reduce_task', 1)  # noqa
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_reduce_task, 1, minimum=1, maximum=available_cores)


def get_mapreduce_reduce_memory_mb(
    subclusters=None, cores_per_reduce_task=None, nodemanager_available_memory_ratio=None
):
    """
    Return memory settings for reducers, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_reduce_task is None:
        cores_per_reduce_task = __salt__['pillar.get']('data:properties:dataproc:cores_per_reduce_task', 1)  # noqa
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
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_app_master is None:
        cores_per_app_master = __salt__['pillar.get']('data:properties:dataproc:cores_per_app_master', 1)  # noqa
    available_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    return round_down(cores_per_app_master, 1, minimum=1, maximum=available_cores)


def get_mapreduce_appmaster_memory_mb(
    subclusters=None, cores_per_app_master=None, nodemanager_available_memory_ratio=None
):
    """
    Return memory settings for application masters, that will work on all nodes
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if cores_per_app_master is None:
        cores_per_app_master = __salt__['pillar.get']('data:properties:dataproc:cores_per_app_master', 1)  # noqa

    max_memory = get_yarn_sched_max_allocation_mb_for_all_instances(subclusters, nodemanager_available_memory_ratio)
    max_cores = get_yarn_sched_max_allocation_vcores_for_all_instances(subclusters)
    map_memory = max_memory * cores_per_app_master / max_cores
    min_mem_allocation = get_yarn_sched_min_allocation_mb(subclusters, nodemanager_available_memory_ratio)
    return round_down(map_memory, min_mem_allocation, maximum=max_memory)


def get_spark_executor_cores(subclusters=None):
    """
    Return how many cores will be allocated for one executor
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa

    default = calculate_cluster_resources(subclusters=subclusters).spark_executor_cores
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.executor.cores', default)
    except KeyError:
        return default


def get_spark_driver_cores(subclusters=None):
    """
    Return how many cores will be allocated for one driver
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa

    default = calculate_cluster_resources(subclusters=subclusters).spark_driver_cores
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.driver.cores', default)
    except KeyError:
        return default


def get_spark_executor_memory_mb(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Return memory for spark executor
    """
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa

    return calculate_cluster_resources(
        subclusters=subclusters,
        available_memory_ratio=nodemanager_available_memory_ratio,
    ).spark_executor_memory_mb


def get_spark_executor_memory(subclusters=None, nodemanager_available_memory_ratio=None):
    """
    Wrapper of method get_spark_executor_memory_mb for dealing with measure suffix
    """
    default = get_spark_executor_memory_mb(subclusters, nodemanager_available_memory_ratio)
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.executor.memory', f"{default}m")
    except KeyError:
        return default


def get_spark_driver_mem_mb(
    subclusters=None,
    spark_driver_memory_fraction=None,
    nodemanager_available_memory_ratio=None,
):
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    # For standard cluster we expect deploy-mode to be `cluster`.
    # Thus, we use fraction of memory on compute node or standard slot of memory for executor, whichever is bigger.
    if spark_driver_memory_fraction is None:
        spark_driver_memory_fraction = get_spark_driver_memory_fraction()

    resources = calculate_cluster_resources(
        subclusters=subclusters,
        available_memory_ratio=nodemanager_available_memory_ratio,
        driver_memory_fraction=spark_driver_memory_fraction,
    )
    if is_lightweight(subclusters):
        return resources.master_node.master_resources.memory_mb
    return resources.compute_node.master_resources.memory_mb


def get_spark_driver_memory(
    subclusters=None,
    spark_driver_memory_fraction=None,
    nodemanager_available_memory_ratio=None,
):
    """
    Wrapper of method get_spark_driver_mem_mb for dealing with measure suffix
    """
    default = get_spark_driver_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.driver.memory', f"{default}m")
    except KeyError:
        return default


def get_spark_driver_result_size_mem_mb(
    subclusters=None,
    spark_driver_memory_fraction=None,
    spark_driver_result_size_ratio=None,
    nodemanager_available_memory_ratio=None,
):
    if spark_driver_result_size_ratio is None:
        spark_driver_result_size_ratio = __salt__['pillar.get'](
            'data:properties:dataproc:spark_driver_result_size_ratio', 0.5
        )  # noqa
    driver_mem_mb = get_spark_driver_mem_mb(
        subclusters=subclusters,
        spark_driver_memory_fraction=spark_driver_memory_fraction,
        nodemanager_available_memory_ratio=nodemanager_available_memory_ratio,
    )
    return round_down(int(driver_mem_mb * spark_driver_result_size_ratio), 128, minimum=256, maximum=4096)


def get_yarn_appmaster_cores(
    subclusters=None,
    spark_driver_memory_fraction=None,
    nodemanager_available_memory_ratio=None,
):
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if spark_driver_memory_fraction is None:
        spark_driver_memory_fraction = get_spark_driver_memory_fraction()

    return calculate_cluster_resources(
        subclusters=subclusters,
        available_memory_ratio=nodemanager_available_memory_ratio,
        driver_memory_fraction=spark_driver_memory_fraction,
    ).yarn_appmaster_cores


def get_yarn_appmaster_mem_mb(
    subclusters=None,
    spark_driver_memory_fraction=None,
    nodemanager_available_memory_ratio=None,
):
    if subclusters is None:
        subclusters = __salt__['pillar.get']('data:topology:subclusters')  # noqa
    if spark_driver_memory_fraction is None:
        spark_driver_memory_fraction = get_spark_driver_memory_fraction()

    return calculate_cluster_resources(
        subclusters=subclusters,
        available_memory_ratio=nodemanager_available_memory_ratio,
        driver_memory_fraction=spark_driver_memory_fraction,
    ).yarn_appmaster_memory_mb


def get_yarn_appmaster_memory(
    subclusters=None,
    spark_driver_memory_fraction=None,
    nodemanager_available_memory_ratio=None,
):
    """
    Wrapper of method get_yarn_appmaster_mem_mb for dealing with measure suffix
    """
    default = get_yarn_appmaster_mem_mb(subclusters, spark_driver_memory_fraction, nodemanager_available_memory_ratio)
    return f"{default}m"


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


def get_hbase_stats():
    """
    Method returns jmx statistics of HBase Master
    """

    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get(
            'http://{}:16010/jmx?qry=Hadoop:service=HBase,name=Master,sub=Server'.format(fqdn), timeout=5.0
        )
        r.raise_for_status()
        return r.json()['beans'][0]

    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_hbase_cluster_status():
    """
    Method returns /status/clsuter of HBase REST
    """

    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get('http://{}:8070/status/cluster'.format(fqdn), timeout=5.0)
        r.raise_for_status()
        return r.text

    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_oozie_status():
    """
    Get oozie status
    """

    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get('http://{}:11000/oozie/v1/admin/status'.format(fqdn), timeout=5.0)
        r.raise_for_status()
        return r.json()

    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_zeppelin_status():
    """
    Get zeppelin status
    """

    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get('http://{}:8890/api/version'.format(fqdn), timeout=5.0)
        r.raise_for_status()
        return r.json()

    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_livy_ping():
    """
    Ping livy server
    """

    @retrying.retry(wait_exponential_multiplier=1000, wait_exponential_max=15000, stop_max_attempt_number=10)
    def _request(fqdn):
        r = requests.get('http://{}:8998/metrics/ping'.format(fqdn), timeout=5.0)
        r.raise_for_status()
        return r.text

    fqdn = get_masternodes()[0]
    return _request(fqdn)


def get_internal_dns_addr():
    """
    Return a list of internal dns servers
    """
    dns = __salt__['pillar.get']('data:properties:dataproc:internal_dns', [])  # noqa
    if dns:
        return dns
    subnets = __salt__['network.ip_networks'](interface='eth*')  # noqa
    for mask in subnets:
        hosts = IPv4Network(mask).hosts()
        try:
            # skip gateway ip address
            _ = next(hosts)
            dns_server = next(hosts)
            if dns_server:
                dns.append(format(dns_server))
        except StopIteration:
            # Just skip subnet if it hasn't dns server
            continue

    if not dns:
        raise Exception('Failed to determine dns servers for current network configuration')
    return ' '.join(dns)


def get_internal_domains():
    """
    Returns a list of domains, that should works with internal cloud dns
    """
    return __salt__['pillar.get'](  # noqa
        'data:properties:dataproc:internal_domains', 'ru-central1.internal yandexcloud.net'
    )


def pip_package_version(package, version):
    """
    Method parses input from pip-properties and build a pip compatible string to installing package.
    Similar method as conda.conda_package_version()
    https://www.python.org/dev/peps/pep-0440/#version-specifiers
    """
    if version in ('any', '', None):
        # package without version
        # pip install numpy
        return str(package)
    if not {'=', '>', '<', '!', ',', '~', '*'} & set(version):
        # Version without expression, use fuzzy
        # The fuzzy constraint numpy=1.11 matches 1.11, 1.11.0, 1.11.1, 1.11.2, 1.11.18, and so on.
        return f'{package}=={version}'
    # Otherwise use complex version condition
    # pip install "numpy>1.11"
    # pip install "numpy=1.11.1|1.11.3"
    # pip install "numpy>=1.8,<2"
    return f'"{package}{version}"'


def get_ppa_repositories():
    """
    Returns a list of ppa's
    """
    ppa_list = __salt__['pillar.get']('data:properties:dataproc:ppa', '')  # noqa
    if not ppa_list:
        return []
    return [repo.strip() for repo in ppa_list.split(',') if repo.strip()]


def get_additional_packages():
    """
    Returns a list of additional packages
    """
    packages_list = __salt__['pillar.get']('data:properties:dataproc:apt', '')  # noqa
    if not packages_list:
        return []
    return [package.strip() for package in packages_list.split(',') if package.strip()]


def get_hive_warehouse_path(services=None, s3_bucket=None):  # -> str
    """
    Returns a URI to hive metastore directory
    """
    if not services:
        services = __salt__['pillar.get']('data:services', [])  # noqa
    if not s3_bucket:
        s3_bucket = get_s3_bucket()
    if 'hdfs' in services:
        return f'hdfs://{ get_masternodes()[0] }:8020/user/hive/warehouse'
    if s3_bucket:
        return f's3a://{ s3_bucket }/warehouse'
    raise DataprocException('hive warehouse undefined without hdfs and s3 bucket, specify one of them')


def get_spark_files():
    """
    Return spark.files from properties
    """
    default = ""
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.files', default)
    except KeyError:
        return default


def get_spark_jars():
    """
    Return spark.files from properties
    """
    default = ""
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.jars', default)
    except KeyError:
        return default


def get_spark_jars_packages():
    """
    Return spark.files from properties
    """
    default = ""
    try:
        return __salt__['pillar.get']('data:properties:spark:spark.jars.packages', default)
    except KeyError:
        return default
