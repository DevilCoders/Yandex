import datetime
import json
from unittest.mock import patch

import boto3
import pytest
from botocore.exceptions import ClientError

import settings
from agent.config.models import (
    Cluster, AgentConfig, BashProberRunner, ClusterMetadata, Prober, ProberConfig, ProberWithConfig
)
from agent.config.s3.downloader import (
    AgentConfigS3Downloader,
    produce_prober_config_variable_combinations_by_matrix
)

testdata_prober = Prober(id=0, name="dns", slug="dns", runner=BashProberRunner(), files=[])
testdata_prober_config_right_hosts_re = ProberConfig(
    prober_id=0,
    hosts_re="vla.*[2,3].cloud.yandex.net",
    is_prober_enabled=False,
    interval_seconds=10,
    timeout_seconds=10,
    s3_logs_policy="FAIL",
)
testdata_prober_config_bad_hosts_re = testdata_prober_config_right_hosts_re.copy(
    update={"hosts_re": "vla.*[2,3.cloud.yandex.net"}
)
testdata_prober_config_with_variables = testdata_prober_config_right_hosts_re.copy(
    update={
        "matrix_variables": {"url": ["https://yandex.ru", "https://google.com"], "ip_version": [4, 6]},
        "variables": {"url": "${matrix.url}", "ip_version": "${matrix.ip_version}", "option": "--verbose"}
    }
)

testdata_cluster_empty = Cluster(id=0, name="test_cluster", slug="testcluster", variables={}, prober_configs=[])
testdata_cluster_bad_hosts_re = testdata_cluster_empty.copy(
    update={"prober_configs": [testdata_prober_config_bad_hosts_re, testdata_prober_config_right_hosts_re]}
)


def create_agent_config_s3_downloader():
    return AgentConfigS3Downloader(
        settings.S3_ENDPOINT,
        settings.S3_ACCESS_KEY_ID,
        settings.S3_SECRET_ACCESS_KEY,
        settings.AGENT_CONFIGURATIONS_S3_BUCKET,
        settings.S3_PREFIX,
    )


@pytest.mark.parametrize(
    "hostname,cluster,prober,expected_config",
    (
            (
                    "vla04-s15-2.cloud.yandex.net",
                    testdata_cluster_empty,
                    testdata_prober,
                    AgentConfig(cluster=ClusterMetadata(testdata_cluster_empty), probers=[]),
            ),
            (
                    "vla04-s15-2.cloud.yandex.net",
                    testdata_cluster_bad_hosts_re,
                    testdata_prober,
                    AgentConfig(
                        cluster=ClusterMetadata(testdata_cluster_empty),
                        probers=[
                            ProberWithConfig(
                                unique_key=(testdata_prober.id, ),
                                prober=testdata_prober,
                                config=testdata_prober_config_right_hosts_re
                            )
                        ],
                    ),
            ),
    )
)
def test_download_config_with_hosts_re(
    hostname: str, cluster: Cluster, prober: Prober, expected_config: AgentConfig, mocked_s3
):
    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=(cluster, False)):
        with patch.object(AgentConfigS3Downloader, "_get_prober", return_value=(prober, False)):
            assert expected_config == create_agent_config_s3_downloader().get_config(cluster.id, hostname)


def test_get_object():
    prober_json = json.dumps(
        {
            "id": 1,
            "name": "test prober",
            "slug": "test-prober",
            "runner": {"type": "BASH", "command": "echo OK"},
            "files": []
        }
    )

    object_modified_time = datetime.datetime.now()
    head_object_return_value = {
        "LastModified": object_modified_time,
    }

    with patch.object(boto3.session, "Session"):
        config_downloader = create_agent_config_s3_downloader()

    # download new object
    with patch.object(config_downloader, "s3") as s3, \
            patch.object(s3, "head_object", return_value=head_object_return_value) as s3_head_object, \
            patch.object(AgentConfigS3Downloader, "_download_content", return_value=prober_json):
        assert config_downloader._s3_objects_cache == {}
        s3_object, from_cache = config_downloader._get_object("s3_object_path", Prober)
        assert not from_cache
        s3_head_object.assert_called_once_with(Bucket=config_downloader.bucket, Key="s3_object_path")
        assert config_downloader._s3_objects_cache["s3_object_path"].object == s3_object
        assert config_downloader._s3_objects_cache["s3_object_path"].last_modified == object_modified_time

    def head_object_func(*args, **kwargs):
        assert kwargs == {
            "Bucket": config_downloader.bucket,
            "Key": "s3_object_path",
            "IfModifiedSince": object_modified_time,
        }
        raise ClientError(error_response={"Error": {"Code": "304"}}, operation_name="head_object")

    # s3 object not changed yet
    with patch.object(config_downloader, "s3") as s3, \
            patch.object(s3, "head_object", new=head_object_func), \
            patch.object(AgentConfigS3Downloader, "_download_content") as download_content_mock:
        assert len(config_downloader._s3_objects_cache) == 1
        _, from_cache = config_downloader._get_object("s3_object_path", Prober)
        assert from_cache
        download_content_mock.assert_not_called()

    # s3 object changed
    object_modified_time2 = datetime.datetime.now()
    head_object_return_value["LastModified"] = object_modified_time2
    with patch.object(config_downloader, "s3") as s3, \
            patch.object(s3, "head_object", return_value=head_object_return_value) as s3_head_object, \
            patch.object(AgentConfigS3Downloader, "_download_content", return_value=prober_json):
        assert len(config_downloader._s3_objects_cache) == 1
        s3_object2, from_cache = config_downloader._get_object("s3_object_path", Prober)
        assert not from_cache
        s3_head_object.assert_called_once_with(
            Bucket=config_downloader.bucket, Key="s3_object_path", IfModifiedSince=object_modified_time,
        )
        assert config_downloader._s3_objects_cache["s3_object_path"].object == s3_object2
        assert config_downloader._s3_objects_cache["s3_object_path"].last_modified == object_modified_time2


def test_produce_prober_config_variable_combinations_by_matrix():
    combinations = list(produce_prober_config_variable_combinations_by_matrix(testdata_prober_config_with_variables))
    assert len(combinations) == 4
    assert combinations[0] == {"url": "https://yandex.ru", "ip_version": 4}
    assert combinations[1] == {"url": "https://yandex.ru", "ip_version": 6}
    assert combinations[2] == {"url": "https://google.com", "ip_version": 4}
    assert combinations[3] == {"url": "https://google.com", "ip_version": 6}
