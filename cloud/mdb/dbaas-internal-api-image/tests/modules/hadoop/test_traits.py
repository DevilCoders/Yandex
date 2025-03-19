"""
Test for check Hadoop's jobs feature availability for clusters
"""

from dbaas_internal_api.modules.hadoop.traits import JobStatus


def test_job_status_is_success():
    assert JobStatus.is_success('DONE')
