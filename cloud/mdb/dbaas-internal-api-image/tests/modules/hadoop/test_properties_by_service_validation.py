"""
Test for validation Hadoop's properties by services
"""
from unittest.mock import Mock

import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError
from dbaas_internal_api.modules.hadoop.traits import ClusterService
from dbaas_internal_api.modules.hadoop.validation import validate_properties_by_services

pillar = Mock()


def test_incorrect_properties():
    pillar.services = [ClusterService.hdfs, ClusterService.yarn]
    pillar.properties = {
        'core:fs.defaultFS': 'myhadoop',
        'hdfs:dfs.replication': 1,
        'yarn:yarn.timeline-service.generic-application-history.enabled': 'true',
        'yarn:yarn.nodemanager.aux-services.mapreduce_shuffle.class': 'org.apache.hadoop.mapred.ShuffleHandler',
        'hive:hive.server2.metrics.enabled': 'true',
    }

    expected_exception = (
        'Properties `hive.server2.metrics.enabled` for services '
        '`hive` has been set, but the next services is missing: `hive`. '
        'Please, create cluster with this services.'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_properties_by_services(pillar)
    assert expected_exception in str(excinfo.value)


def test_correct_properties():
    pillar.properties = {
        'hdfs:dfs.replication': 1,
        'yarn:yarn.timeline-service.generic-application-history.enabled': 'true',
        'yarn:yarn.nodemanager.aux-services.mapreduce_shuffle.class': 'org.apache.hadoop.mapred.ShuffleHandler',
        'hive:hive.server2.metrics.enabled': 'true',
    }
    pillar.services = [ClusterService.hdfs, ClusterService.yarn, ClusterService.hive]

    validate_properties_by_services(pillar)


def test_multiple_incorrect_properties():
    pillar.properties = {
        'hdfs:dfs.replication': 1,
        'yarn:yarn.timeline-service.generic-application-history.enabled': 'true',
        'yarn:yarn.nodemanager.aux-services.mapreduce_shuffle.class': 'org.apache.hadoop.mapred.ShuffleHandler',
        'hive:hive.server2.metrics.enabled': 'true',
        'spark:spark.dynamicallocation.enabled': 'true',
    }
    pillar.services = [ClusterService.hdfs, ClusterService.yarn]

    expected_exception = (
        'Properties `hive.server2.metrics.enabled, spark.dynamicallocation.enabled` for services '
        '`hive, spark` has been set, but the next services is missing: `hive, spark`. '
        'Please, create cluster with this services.'
    )
    with pytest.raises(DbaasClientError) as excinfo:
        validate_properties_by_services(pillar)
    assert expected_exception in str(excinfo.value)
