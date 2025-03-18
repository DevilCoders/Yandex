from enum import Enum
from dataclasses import dataclass


class TaskStatus(str, Enum):
    SUCCESS = 'success'
    RETRY = 'retry'
    FAILED = 'failed'


@dataclass
class TaskContext:
    id: str
    name: str
    status: str
    args: list
    kwargs: dict
    extra: dict

    @property
    def args_kwargs(self) -> list:
        return [self.args, self.kwargs]
