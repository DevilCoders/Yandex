# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop globals
"""

import datetime

import semver

MY_CLUSTER_TYPE = 'hadoop_cluster'

MASTER_SUBCLUSTER_TYPE = 'hadoop_cluster.masternode'
DATA_SUBCLUSTER_TYPE = 'hadoop_cluster.datanode'
COMPUTE_SUBCLUSTER_TYPE = 'hadoop_cluster.computenode'

FQDN_MARKS = {
    MASTER_SUBCLUSTER_TYPE: 'dataproc-m',
    DATA_SUBCLUSTER_TYPE: 'dataproc-d',
    COMPUTE_SUBCLUSTER_TYPE: 'dataproc-c',
}

SUBCLUSTER_NAME_PREFIX_BY_ROLE = {
    MASTER_SUBCLUSTER_TYPE: 'master',
    DATA_SUBCLUSTER_TYPE: 'data',
    COMPUTE_SUBCLUSTER_TYPE: 'compute',
}

MAX_CLOUD_LOGGING_PAGE_SIZE = 1000

FLUENTBIT_LOG_SEND_LAG = datetime.timedelta(seconds=60)

DATAPROC_LIGHTWEIGHT_HIVE_VERSION = semver.VersionInfo(2, 0, 44)
DATAPROC_LIGHTWEIGHT_SPARK_VERSION = semver.VersionInfo(2, 0, 39)
DATAPROC_INIT_ACTIONS_VERSION = semver.VersionInfo(2, 0, 40)
DATAPROC_AGENT_METRICS_VERSION_2 = semver.VersionInfo(2, 0, 34)
DATAPROC_AGENT_METRICS_VERSION_1 = semver.VersionInfo(1, 4, 32)
DATAPROC_VERSION_2 = semver.VersionInfo(2, 0, 0)
