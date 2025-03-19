"""
DBaaS Internal API Hadoop cluster traits
"""
from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName
from ...utils.types import ComparableEnum
from .constants import COMPUTE_SUBCLUSTER_TYPE, DATA_SUBCLUSTER_TYPE, MASTER_SUBCLUSTER_TYPE, MY_CLUSTER_TYPE


class HadoopTasks(ComparableEnum):
    """Hadoop cluster tasks"""

    create = 'hadoop_cluster_create'
    start = 'hadoop_cluster_start'
    stop = 'hadoop_cluster_stop'
    modify = 'hadoop_cluster_modify'
    delete = 'hadoop_cluster_delete'
    subcluster_create = 'hadoop_subcluster_create'
    subcluster_modify = 'hadoop_subcluster_modify'
    subcluster_delete = 'hadoop_subcluster_delete'
    job_create = 'hadoop_job_create'


class HadoopOperations(ComparableEnum):
    """Hadoop cluster operations"""

    create = 'hadoop_cluster_create'
    modify = 'hadoop_cluster_modify'
    delete = 'hadoop_cluster_delete'
    start = 'hadoop_cluster_start'
    stop = 'hadoop_cluster_stop'
    subcluster_create = 'hadoop_subcluster_create'
    subcluster_modify = 'hadoop_subcluster_modify'
    subcluster_delete = 'hadoop_subcluster_delete'
    job_create = 'hadoop_job_create'


@unique
class ClusterService(Enum):
    """
    Possible host services.
    """

    hdfs = 'ClusterServiceHDFS'
    yarn = 'ClusterServiceYARN'
    mapreduce = 'ClusterServiceMapReduce'
    hive = 'ClusterServiceHive'
    tez = 'ClusterServiceTez'
    zookeeper = 'ClusterServiceZookeeper'
    hbase = 'ClusterServiceHbase'
    sqoop = 'ClusterServiceSqoop'
    flume = 'ClusterServiceFlume'
    spark = 'ClusterServiceSpark'
    zeppelin = 'ClusterServiceZeppelin'
    oozie = 'ClusterServiceOozie'
    livy = 'ClusterServiceLivy'

    @classmethod
    def to_enums(cls, services):
        """
        Cast strings list to enums list
        """
        if not services:
            return []
        return [cls.to_enum(service) for service in services]

    @classmethod
    def to_enum(cls, service):
        """
        Cast string to enum
        """
        return cls.__members__[service]

    @classmethod
    def to_strings(cls, services):
        """
        Cast enums list to string list
        """
        if not services:
            return []
        return [service.name for service in services]

    @classmethod
    def to_string(cls, service):
        """
        Cast enum to string
        """
        return service.name


class HadoopRoles(ComparableEnum):
    """
    Roles of Hadoop clusters.
    """

    master = MASTER_SUBCLUSTER_TYPE
    data = DATA_SUBCLUSTER_TYPE
    compute = COMPUTE_SUBCLUSTER_TYPE


class InstanceRoles:
    """
    User's instances roles.
    """

    roles = {
        MASTER_SUBCLUSTER_TYPE: 'masternode',
        DATA_SUBCLUSTER_TYPE: 'datanode',
        COMPUTE_SUBCLUSTER_TYPE: 'computenode',
    }

    @classmethod
    def get_humanize_role(cls, role):
        """
        Get user's role for cluster
        """
        return cls.roles.get(role)


class DataprocClusterName(ClusterName):
    """
    Describes dataproc valid cluster name value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9_-]*', min: int = 0, max: int = 63, name: str = 'Cluster name', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


@register_cluster_traits(MY_CLUSTER_TYPE)
class HadoopClusterTraits:
    """
    Traits of Hadoop clusters.
    """

    name = 'hadoop'
    url_prefix = name
    service_slug = 'data-proc'
    roles = HadoopRoles
    tasks = HadoopTasks
    operations = HadoopOperations
    cluster_name = DataprocClusterName()
    password = None
    versions_column = VersionsColumn.cluster
    versions_component = 'hadoop'
    auth_actions = DEFAULT_AUTH_ACTIONS


@unique
class HostHealth(Enum):
    """
    Possible host health.
    """

    unknown = 'HostHealthUnknown'
    alive = 'HostHealthAlive'
    dead = 'HostHealthDead'
    degraded = 'HostHealthDegraded'


@unique
class JobStatus(Enum):
    """
    Possible job statuses.
    """

    PROVISIONING = 'PROVISIONING'
    PENDING = 'PENDING'
    RUNNING = 'RUNNING'
    ERROR = 'ERROR'
    DONE = 'DONE'
    CANCELLED = 'CANCELLED'
    CANCELLING = 'CANCELLING'

    @classmethod
    def to_enum(cls, status):
        """
        Cast string to enum
        """
        return cls.__members__[status]

    @classmethod
    def terminal_statuses(cls):
        """
        Return terminal statuses
        """
        return ['ERROR', 'DONE', 'CANCELLED']

    @classmethod
    def all_statuses(cls):
        """
        Return all statuses
        """
        return list(cls.__members__)

    @classmethod
    def active_statuses(cls):
        """
        Return non terminal statuses
        """
        return list(set(cls.all_statuses()) - set(cls.terminal_statuses()))

    @classmethod
    def has_member(cls, status):
        """
        Check if enum has such member
        """
        return status in cls.all_statuses()

    @classmethod
    def is_terminal(cls, status):
        """
        Check if status is terminal
        """
        return status in cls.terminal_statuses()

    @classmethod
    def is_success(cls, state):
        """
        Check if state is success terminal state
        """
        return state == 'DONE'


# Requirements of services for running specific job type
DATAPROC_JOB_SERVICES = {
    "mapreduce_job": [ClusterService.yarn, ClusterService.mapreduce],
    "spark_job": [ClusterService.spark],
    "pyspark_job": [ClusterService.spark],
    "hive_job": [ClusterService.hive, ClusterService.yarn, ClusterService.mapreduce],
}
