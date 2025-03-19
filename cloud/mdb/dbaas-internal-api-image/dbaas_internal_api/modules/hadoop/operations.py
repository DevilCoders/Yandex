"""
Hadoop operations description
"""

from ...core.types import ResponseType
from ...utils.operations import EMPTY_RESPONSE as _EMPTY
from ...utils.operations import (
    OperationHandle,
    annotation_maker,
    describe_operations,
    modify_hadoop_job_event_details,
    modify_hadoop_job_metadata,
    modify_subcluster_event_details,
    modify_subcluster_metadata,
)
from .constants import MY_CLUSTER_TYPE
from .traits import HadoopOperations

__a = annotation_maker('yandex.cloud.dataproc.v1')
__et = annotation_maker('yandex.cloud.events.dataproc')
_CLUSTER = (ResponseType.cluster, __a('Cluster'))
_SUBCLUSTER = (ResponseType.subcluster, __a('Subcluster'))
_HADOOPJOB = (ResponseType.hadoop_job, __a('HadoopJob'))

describe_operations(
    cluster_type=MY_CLUSTER_TYPE,
    operation_types=HadoopOperations,
    handles={
        HadoopOperations.create: OperationHandle(
            'Create Data Proc cluster',
            metadata_annotation=__a('CreateClusterMetadata'),
            event_type=__et('CreateCluster'),
            response=_CLUSTER,
        ),
        HadoopOperations.modify: OperationHandle(
            'Modify Data Proc cluster',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
        ),
        HadoopOperations.start: OperationHandle(
            'Start Data Proc cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterMetadata'),
            event_type=__et('StartCluster'),
        ),
        HadoopOperations.stop: OperationHandle(
            'Stop Data Proc cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StopClusterMetadata'),
            event_type=__et('StopCluster'),
        ),
        HadoopOperations.delete: OperationHandle(
            'Delete Data Proc cluster',
            metadata_annotation=__a('DeleteClusterMetadata'),
            event_type=__et('DeleteCluster'),
            response=_EMPTY,
        ),
        HadoopOperations.subcluster_create: OperationHandle(
            'Create Data Proc subcluster',
            metadata_annotation=__a('CreateSubclusterMetadata'),
            event_type=__et('CreateSubcluster'),
            metadata=modify_subcluster_metadata,
            event_details=modify_subcluster_event_details,
            response=_SUBCLUSTER,
        ),
        HadoopOperations.subcluster_modify: OperationHandle(
            'Modify Data Proc subcluster',
            metadata_annotation=__a('ModifySubclusterMetadata'),
            event_type=__et('UpdateSubcluster'),
            metadata=modify_subcluster_metadata,
            event_details=modify_subcluster_event_details,
            response=_SUBCLUSTER,
        ),
        HadoopOperations.subcluster_delete: OperationHandle(
            'Delete Data Proc subcluster',
            metadata_annotation=__a('DeleteSubclusterMetadata'),
            event_type=__et('DeleteSubcluster'),
            metadata=modify_subcluster_metadata,
            event_details=modify_subcluster_event_details,
            response=_SUBCLUSTER,
        ),
        HadoopOperations.job_create: OperationHandle(
            'Create Data Proc job',
            metadata_annotation=__a('CreateJobMetadata'),
            event_type=__et('HadoopJobCreate'),
            metadata=modify_hadoop_job_metadata,
            event_details=modify_hadoop_job_event_details,
            response=_HADOOPJOB,
        ),
    },
)
