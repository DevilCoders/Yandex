import typing

from .engine import CachingLevenshteinEngine
from ..common import Metric, MetricData


class LevenshteinData(MetricData):
    score: float

    def __init__(self, score: float):
        self.score = score

    def to_json(self) -> typing.Dict:
        return {'score': self.score}

    @staticmethod
    def from_json(fields: typing.Dict) -> 'LevenshteinData':
        return LevenshteinData(score=fields['score'])

    def __eq__(self, other: 'LevenshteinData'):
        def round_func(score):
            return float('{0:.5f}'.format(score))

        return round_func(self.score) == round_func(other.score)


class Levenshtein(Metric):
    engine: CachingLevenshteinEngine

    def __init__(self, name: str, engine: CachingLevenshteinEngine):
        super(Levenshtein, self).__init__(name=name)
        self.engine = engine

    def get_metric_data(self, hyp: str, ref: str) -> LevenshteinData:
        score = self.engine(hyp, ref)
        return LevenshteinData(score)

    @staticmethod
    def calculate_metric(metric_data: LevenshteinData) -> float:
        return metric_data.score

    @staticmethod
    def aggregate_metric(
        metric_value_list: typing.List[float], metric_data_list: typing.List[LevenshteinData]
    ) -> typing.Dict[str, float]:
        return {'mean': sum(metric_value_list) / len(metric_value_list)}

    @staticmethod
    def are_stop_words_applicable() -> bool:
        return False
