"""
Test for validation dataproc jobs
"""
import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError

from dbaas_internal_api.modules.hadoop import constants
from dbaas_internal_api.modules.hadoop.pillar import HadoopPillar
from dbaas_internal_api.modules.hadoop.traits import ClusterService
from dbaas_internal_api.modules.hadoop.validation import validate_lightweight_jobs, validate_job_services


def new_pillar():
    """
    Return mocked empty pillar
    """
    return HadoopPillar({'data': {'unmanaged': {'topology': {'subclusters': {}}}}})


def test_incorrect_spark_job_on_lightweight_cluster():
    pillar = new_pillar()
    pillar.services = [ClusterService.yarn, ClusterService.spark]
    pillar.user_s3_bucket = 'that-bucket-with-wild-strawberry'
    pillar.set_subcluster({'name': 'driver', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1'})
    pillar.set_subcluster({'name': 'executors', 'role': constants.COMPUTE_SUBCLUSTER_TYPE, 'subcid': '2'})

    expected_exception = 'Using fileUris is forbidden on lightweight clusters'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_lightweight_jobs(
            {
                "spark_job": {
                    "name": "spark-pi",
                    "file_uris": ["foo.txt", "bar.docx"],
                }
            },
            pillar,
        )
    assert expected_exception in str(excinfo.value)
    with pytest.raises(DbaasClientError) as excinfo:
        validate_lightweight_jobs(
            {
                "pyspark_job": {
                    "name": "spark-pi",
                    "file_uris": ["foo.txt", "bar.docx"],
                }
            },
            pillar,
        )
    assert expected_exception in str(excinfo.value)


def test_spark_jobs_on_lightweight_cluster():
    pillar = new_pillar()
    pillar.services = [ClusterService.yarn, ClusterService.spark]
    pillar.user_s3_bucket = 'that-bucket-with-wild-strawberry'
    pillar.set_subcluster({'name': 'driver', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1'})
    pillar.set_subcluster({'name': 'executors', 'role': constants.COMPUTE_SUBCLUSTER_TYPE, 'subcid': '2'})

    validate_lightweight_jobs(
        {
            "spark_job": {
                "name": "spark-pi",
                "main_jar_file_uri": "/usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar",
                "args": ["pi", "1", "100"],
            }
        },
        pillar,
    )


def test_incorrect_hive_job_on_lightweight_metastore_cluster():
    pillar = new_pillar()
    pillar.services = [ClusterService.hive]
    pillar.user_s3_bucket = 'that-bucket-with-wild-strawberry'
    pillar.set_subcluster({'name': 'metastore', 'role': constants.MASTER_SUBCLUSTER_TYPE, 'subcid': '1'})

    expected_exception = 'To create job of this type you need cluster with services: mapreduce, yarn'
    with pytest.raises(DbaasClientError) as excinfo:
        validate_job_services(
            {
                "hive_job": {
                    "name": "select 1",
                    "query_list": {
                        "queries": [
                            "select 1",
                        ],
                    },
                },
            },
            pillar,
        )
    assert expected_exception in str(excinfo.value)
