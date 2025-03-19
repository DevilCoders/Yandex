"""
Test for validation dataproc topoligies and services
"""
import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError

from dbaas_internal_api.modules.hadoop import constants
from dbaas_internal_api.modules.hadoop.pillar import HadoopPillar
from dbaas_internal_api.modules.hadoop.traits import ClusterService
from dbaas_internal_api.modules.hadoop.validation import validate_subclusters_and_services, validate_s3_bucket


def new_pillar():
    """
    Return mocked empty pillar
    """
    return HadoopPillar({'data': {'unmanaged': {'topology': {'subclusters': {}}}}})


def test_incorrect_with_datanodes_wo_data_services():
    pillar = new_pillar()
    pillar.services = [ClusterService.yarn]
    pillar.set_subcluster({'name': 'master', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1'})
    pillar.set_subcluster({'name': 'data', 'role': constants.DATA_SUBCLUSTER_TYPE, 'subcid': '2'})

    expected_exception = 'Creating cluster with datanodes without any of services hdfs, oozie is forbidden'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_subclusters_and_services(pillar)
    assert expected_exception in str(excinfo.value)


def test_incorrect_wo_datanodes_with_data_services():
    pillar = new_pillar()
    pillar.services = [ClusterService.hdfs]
    pillar.set_subcluster({'name': 'master', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1'})
    pillar.set_subcluster({'name': 'compute', 'role': constants.COMPUTE_SUBCLUSTER_TYPE, 'subcid': '2'})

    expected_exception = 'Creating cluster without datanodes and with services hdfs is forbidden.'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_subclusters_and_services(pillar)
    assert expected_exception in str(excinfo.value)


def test_lightweight_spark():
    pillar = new_pillar()
    pillar.services = [ClusterService.yarn, ClusterService.spark, ClusterService.livy]
    pillar.user_s3_bucket = 'bucket-with-wild-strawberry'
    pillar.set_subcluster(
        {'name': 'driver', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1', 'services': pillar.services}
    )
    pillar.set_subcluster(
        {
            'name': 'executors',
            'role': constants.COMPUTE_SUBCLUSTER_TYPE,
            'subcid': '2',
            'services': [ClusterService.yarn],
        }
    )

    validate_subclusters_and_services(pillar)
    validate_s3_bucket(pillar)


def test_lightweight_hive_metastore():
    pillar = new_pillar()
    pillar.services = [ClusterService.hive]
    pillar.user_s3_bucket = 'bucket-with-wild-strawberry'
    pillar.set_subcluster(
        {'name': 'metastore', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1', 'services': pillar.services}
    )

    validate_subclusters_and_services(pillar)
    validate_s3_bucket(pillar)


def test_lightweight_cluster_without_bucket():
    pillar = new_pillar()
    pillar.services = [ClusterService.yarn, ClusterService.spark, ClusterService.livy]
    pillar.set_subcluster(
        {'name': 'driver', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1', 'services': pillar.services}
    )
    pillar.set_subcluster(
        {
            'name': 'executors',
            'role': constants.COMPUTE_SUBCLUSTER_TYPE,
            'subcid': '2',
            'services': [ClusterService.yarn],
        }
    )

    expected_exception = 'Unable to use lightweight cluster without s3 bucket'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_s3_bucket(pillar)
    assert expected_exception in str(excinfo.value)
