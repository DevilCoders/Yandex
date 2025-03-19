from abc import ABC, abstractmethod
from typing import Dict, Sequence


class Logger(ABC):
    @abstractmethod
    def log_metrics(self, metrics: Dict[str, float], step: int):
        pass

    def finalize(self):
        pass


class LoggerCollection(Logger):
    def __init__(self, logger_sequence: Sequence[Logger]):
        self._logger_sequence = logger_sequence

    def log_metrics(self, metrics: Dict[str, float], step: int):
        for logger in self._logger_sequence:
            logger.log_metrics(metrics=metrics, step=step)

    def finalize(self):
        for logger in self._logger_sequence:
            logger.finalize()


class DummyLogger(Logger):
    def log_metrics(self, metrics: Dict[str, float], step: int):
        pass
