import typing

from .engine import CachingMEREngine
from ..common import Metric, MetricData


class MERData(MetricData):
    score: float

    def __init__(self, score: float):
        self.score = score

    def to_json(self) -> dict:
        return {'score': self.score}

    @staticmethod
    def from_json(fields: dict) -> 'MERData':
        return MERData(score=fields['score'])

    def __eq__(self, other: 'MERData'):
        def round_func(score):
            return float('{0:.5f}'.format(score))

        return round_func(self.score) == round_func(other.score)


class MER(Metric):
    engine: CachingMEREngine

    def __init__(self, name: str, engine: CachingMEREngine):
        super(MER, self).__init__(name=name)
        self.engine = engine

    def get_metric_data(self, hyp: str, ref: str) -> MERData:
        score = self.engine(hyp, ref)
        return MERData(score)

    @staticmethod
    def calculate_metric(metric_data: MERData) -> float:
        return metric_data.score

    @staticmethod
    def aggregate_metric(
        metric_value_list: typing.List[float],
        metric_data_list: typing.List[MERData],
    ) -> typing.Dict[str, float]:
        return {'mean': sum(metric_value_list) / len(metric_value_list)}

    @staticmethod
    def are_stop_words_applicable() -> bool:
        return False
