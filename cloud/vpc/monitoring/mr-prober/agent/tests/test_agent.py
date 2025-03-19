import pathlib
import time
from typing import List
from unittest.mock import patch, MagicMock

import botocore.exceptions

import settings
from agent.config import StaticAgentConfigLoader
from agent.config.models import (
    ClusterMetadata, Cluster, ProberWithConfig, Prober, BashProberRunner, ProberFile,
    ProberConfig, AgentConfig,
)
from agent.main import update_probers_map_from_config, load_config_with_retry
from common.monitoring.solomon import SolomonClient
from agent.telemetry import AgentTelemetrySender, SolomonMetric, MetricKind
from database.models import UploadProberLogPolicy


class DummySolomonClient(SolomonClient):
    def send_metrics(self, metrics: List[SolomonMetric]):
        pass


def get_test_config() -> AgentConfig:
    """
    For development & testing purposes only
    """
    cluster = ClusterMetadata(
        Cluster(
            id=1,
            name="Meeseeks",
            slug="meeseeks",
            variables={},
            prober_configs=[],
        )
    )
    probers_config = [
        ProberWithConfig(
            unique_key=(1,),
            prober=Prober(
                id=1,
                name="Sleep",
                slug="sleep",
                runner=BashProberRunner(
                    command="/bin/bash -c 'echo Timeout; sleep 4'",
                ),
                files=[
                    ProberFile(
                        id=1,
                        prober_id=1,
                        is_executable=True,
                        relative_file_path="metadata.sh",
                        md5_hexdigest="d41d8cd98f00b204e9800998ecf8427e",
                    )
                ],
                files_location=pathlib.Path("/tmp"),
            ),
            config=ProberConfig(
                prober_id=1,
                is_prober_enabled=True,
                interval_seconds=5,
                timeout_seconds=3,
                s3_logs_policy=UploadProberLogPolicy.FAIL,
            ),
        ),
        ProberWithConfig(
            unique_key=(2,),
            prober=Prober(
                id=2,
                name="Echo",
                slug="echo",
                runner=BashProberRunner(
                    command="echo Success; echo $HOME",
                ),
                files=[
                    ProberFile(
                        id=2,
                        prober_id=2,
                        is_executable=False,
                        relative_file_path="dns.sh",
                        md5_hexdigest="d41d8cd98f00b204e9800998ecf8427f",
                    )
                ],
                files_location=pathlib.Path("/tmp"),
            ),
            config=ProberConfig(
                prober_id=2,
                is_prober_enabled=True,
                interval_seconds=7,
                timeout_seconds=5,
                s3_logs_policy=UploadProberLogPolicy.ALL,
            ),
        )]

    return AgentConfig(cluster=cluster, probers=probers_config)


def test_update_probers_map_from_config_from_empty():
    config = get_test_config()
    probers_map = {}
    solomon_client = DummySolomonClient()
    with patch("agent.main.ProberProcess") as ProberProcess:
        update_probers_map_from_config(probers_map, StaticAgentConfigLoader(config), solomon_client)

    assert ProberProcess.call_count == 2
    ProberProcess.assert_any_call(config.probers[0].prober, config.probers[0].config, config.cluster, solomon_client, 0)
    ProberProcess.assert_any_call(config.probers[1].prober, config.probers[1].config, config.cluster, solomon_client, 1)

    assert probers_map.keys() == {(1,), (2,)}
    assert isinstance(probers_map[(1,)], type(ProberProcess.return_value))
    assert isinstance(probers_map[(2,)], type(ProberProcess.return_value))


def test_update_probers_map_from_config_with_existing_prober():
    config = get_test_config()
    solomon_client = DummySolomonClient()
    with patch("agent.main.ProberProcess") as ProberProcess:
        probers_map = {
            (1,): ProberProcess.return_value
        }
        with patch.object(ProberProcess.return_value, "update_prober_if_changed") as update_prober_if_changed, \
                patch.object(ProberProcess.return_value, "update_config_if_changed") as update_config_if_changed:
            update_probers_map_from_config(probers_map, StaticAgentConfigLoader(config), solomon_client)

    assert ProberProcess.call_count == 1
    ProberProcess.assert_any_call(config.probers[1].prober, config.probers[1].config, config.cluster, solomon_client, 1)

    update_prober_if_changed.assert_called_once_with(config.probers[0].prober)
    update_config_if_changed.assert_called_once_with(config.probers[0].config)

    assert probers_map.keys() == {(1,), (2,)}


def test_prober_has_been_deleted_from_config():
    # 1. Create a config with 2 probers and empty probers_map
    config = get_test_config()
    probers_map = {}
    solomon_client = DummySolomonClient()
    with patch("agent.main.ProberProcess"):
        update_probers_map_from_config(probers_map, StaticAgentConfigLoader(config), solomon_client)

        # 2. Delete one prober in config and update probers_map
        config.probers.pop()
        update_probers_map_from_config(probers_map, StaticAgentConfigLoader(config), solomon_client)

    # 3. Check that only one prober left in probers_map
    assert len(probers_map) == 1


def test_agent_telemetry_metrics_send():
    solomon_client_mock = MagicMock()
    solomon_client_mock.send_metrics = MagicMock()
    agent_start_timer_mock = MagicMock()
    agent_start_timer_mock.get_total_seconds = MagicMock(return_value=345.345635345)

    cluster_slug = "test-cluster"
    sender = AgentTelemetrySender(DummySolomonClient(), cluster_slug)
    sender._solomon_client = solomon_client_mock
    sender._agent_start_time = agent_start_timer_mock
    time_now = 1634124163.5359619
    probers_count = 26
    expected_metrics = [
        SolomonMetric(int(time_now), 1, metric="keep_alive", kind=MetricKind.GAUGE, cluster_slug=cluster_slug),
        SolomonMetric(
            int(time_now), probers_count, metric="probers_count", kind=MetricKind.GAUGE, cluster_slug=cluster_slug
        ),
        SolomonMetric(int(time_now), 345, metric="uptime", kind=MetricKind.COUNTER, cluster_slug=cluster_slug),
    ]

    with patch.object(time, "time", return_value=time_now):
        sender.send_agent_telemetry(probers_count)
        solomon_client_mock.send_metrics.assert_called_once_with(expected_metrics)


def test_load_config_with_retry():
    expected_config = get_test_config()
    config_loader = StaticAgentConfigLoader(expected_config)
    cluster_config = load_config_with_retry(config_loader)
    assert cluster_config == expected_config

    config_loader = MagicMock()
    config_loader.load = MagicMock()
    config_loader.load.side_effect = botocore.exceptions.ClientError(
        error_response={"Error": {"Code": "404"}}, operation_name="head_object"
    )
    settings.AGENT_GET_CLUSTER_CONFIG_RETRY_DELAY = 0.01
    settings.AGENT_GET_CLUSTER_CONFIG_RETRY_ATTEMPTS = 3
    cluster_config = load_config_with_retry(config_loader)
    assert cluster_config is None
    assert config_loader.load.call_count == 3
