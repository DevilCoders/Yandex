import typing


class MetricData:
    def to_json(self) -> typing.Dict:
        raise NotImplementedError

    @staticmethod
    def from_json(fields: typing.Dict) -> 'MetricData':
        raise NotImplementedError


class Metric:
    name: str

    def __init__(self, name: str):
        self.name = name

    def get_metric_data(self, hyp: str, ref: str) -> typing.Any:
        raise NotImplementedError

    @staticmethod
    def calculate_metric(metric_data: MetricData) -> float:
        raise NotImplementedError

    @staticmethod
    def aggregate_metric(
        metric_value_list: typing.List[float], metric_data_list: typing.List[MetricData]
    ) -> typing.Dict[str, float]:
        raise NotImplementedError

    @staticmethod
    def are_stop_words_applicable() -> bool:
        raise NotImplementedError
