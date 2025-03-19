from abc import ABC, abstractmethod


class CloudInitModule(ABC):
    scalar = False  # if true one cannot append values, only overwrite.

    @property
    @abstractmethod
    def module_name(self) -> str:
        pass
