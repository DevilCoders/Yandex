import enum


class ProberExecutionStatus(enum.Enum):
    SUCCESS = 0
    FAIL = 1
    TIMEOUT = 2
    FAILED_TO_START = 3
