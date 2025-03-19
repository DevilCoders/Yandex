from typing import Any
from abc import ABC, abstractmethod


class ExecuteQueryAdapter(ABC):

    @abstractmethod
    def execute_query(self, query: str) -> Any:
        pass
