from typing import Dict, Tuple
from abc import ABC, abstractmethod
from collections import defaultdict


class MetricData(ABC):
    @abstractmethod
    def to_json(self) -> Dict:
        pass

    @staticmethod
    @abstractmethod
    def from_json(fields: Dict) -> 'MetricData':
        pass


class Metric(ABC):
    name: str

    def __init__(self, name: str):
        self.name = name

    @abstractmethod
    def get_metric_data(self, hyp: str, ref: str) -> MetricData:
        pass

    @staticmethod
    @abstractmethod
    def calculate_metric(metric_data: MetricData) -> float:
        pass


def evaluate_metric(references: Dict[str, str],
                    hypotheses: Dict[str, str],
                    metric_calcer: Metric) -> Tuple[float, Dict[str, Dict]]:
    metric_data = {}
    metric_values = defaultdict(float)

    assert len(references) == len(hypotheses)
    for record_id, reference in references.items():
        assert record_id in hypotheses
        hypothesis = hypotheses[record_id]
        metric_data[record_id] = metric_calcer.get_metric_data(hypothesis, reference)
        metric_values[record_id] = metric_calcer.calculate_metric(metric_data[record_id])

    mean_metric_value = sum(metric_values.values()) / max(1, len(metric_values))

    full_report = {
        record_id: {
            'metric_value': metric_values[record_id],
            'metric_data': data.to_json(),
            'reference': references[record_id],
            'hypothesis': hypotheses[record_id]
        }
        for record_id, data in metric_data.items()
    }

    return mean_metric_value, full_report
