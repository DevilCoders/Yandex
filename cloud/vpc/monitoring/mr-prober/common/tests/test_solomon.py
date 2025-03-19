import json
import time
from typing import List
from unittest.mock import patch

import pytest
import requests

import settings
from common.monitoring.solomon import SolomonClient, SolomonMetric, MetricKind


@pytest.mark.parametrize(
    "metrics",
    [
        [
            SolomonMetric(int(time.time()), 23),
            SolomonMetric(int(time.time()), 24, MetricKind.GAUGE, label1="label1value"),
        ],
    ]
)
def test_send_metric(metrics: List[SolomonMetric]):
    with patch.object(requests, "post") as requests_post, patch.object(requests_post, "ok", return_value=True):
        SolomonClient().send_metrics(metrics)
        expected_json = {
            "sensors": [
                {
                    "kind": metric.kind.value,
                    "labels": metric.labels,
                    "timeseries": [
                        {
                            "ts": metric.timestamp,
                            "value": metric.value,
                        }
                    ]
                } for metric in metrics]
        }
        requests_post.assert_called_once_with(settings.SOLOMON_AGENT_ENDPOINT, timeout=3, json=expected_json)

        # checks that expected_json is a valid object for serialization
        assert json.dumps(expected_json)
