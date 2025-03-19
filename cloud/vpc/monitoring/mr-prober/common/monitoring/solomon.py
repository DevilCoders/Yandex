import enum
import logging
from typing import List

import requests

import settings
from common.async_helper import AsyncHelper


__all__ = ["MetricKind", "SolomonMetric", "SolomonClient", "AsyncSolomonClient"]


class MetricKind(enum.Enum):
    DGAUGE = "DGAUGE"
    GAUGE = "GAUGE"
    COUNTER = "COUNTER"
    RATE = "RATE"
    HIST = "HIST"
    HIST_RATE = "HIST_RATE"
    DSUMMARY = "DSUMMARY"

    def __repr__(self):
        return repr(self.value)


class SolomonMetric:
    def __init__(self, timestamp: int, value: float, kind: MetricKind = MetricKind.DGAUGE, **labels):
        self.kind = kind
        self.timestamp = timestamp
        self.value = value
        self.labels = labels

    def __eq__(self, other: "SolomonMetric"):
        if not isinstance(other, SolomonMetric):
            return False
        return self.kind == other.kind \
               and self.timestamp == other.timestamp \
               and self.value == other.value \
               and self.labels == other.labels

    def __repr__(self):
        return f"{self.__class__.__name__}(" \
               f"kind={self.kind!r}, ts={self.timestamp}, value={self.value}, labels={self.labels}"


class SolomonClient:
    def __init__(self, endpoint: str = settings.SOLOMON_AGENT_ENDPOINT):
        self._endpoint = endpoint

    def send_metrics(self, metrics: List[SolomonMetric]):
        # See https://docs.yandex-team.ru/solomon/data-collection/dataformat/json about the data format
        sensors = [{
            "kind": metric.kind.value,
            "labels": metric.labels,
            "timeseries": [
                {
                    "ts": metric.timestamp,
                    "value": metric.value
                }
            ]
        } for metric in metrics]

        try:
            r = requests.post(self._endpoint, json={"sensors": sensors}, timeout=3)
        except requests.RequestException as e:
            logging.warning(
                f"Could not send metrics {[metric.labels for metric in metrics]} to solomon-agent: {e}", exc_info=e
            )
            return

        if not r.ok:
            logging.warning(f"Got error {r.reason}({r.status_code}) when sending metrics to solomon-agent")
            logging.info(f"Response from solomon-agent: {r.text}")


class AsyncSolomonClient(AsyncHelper, SolomonClient):
    def __init__(self, *args, **kwargs):
        super(AsyncSolomonClient, self).__init__(*args, **kwargs)
        self.extract_method_to_thread("send_metrics")
