import abc

from yc_common import logging

log = logging.get_logger(__name__)


class BaseMigration(abc.ABC):
    @property
    def name(self) -> str:
        if not hasattr(self, "_name"):
            self._name = self.__module__.split(".")[-1]

        return self._name

    @abc.abstractmethod
    def execute(self) -> None:
        pass

    @abc.abstractmethod
    def rollback(self) -> None:
        pass
