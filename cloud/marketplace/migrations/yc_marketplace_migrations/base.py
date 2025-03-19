import abc


class BaseMigration(abc.ABC):
    @property
    def name(self) -> str:
        if not hasattr(self, "_name"):
            self._name = self.__module__.split(".")[-1]

        return self._name

    @abc.abstractmethod
    def execute(self) -> None:
        pass

    def rollback(self) -> None:
        pass
