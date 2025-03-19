import enum
from typing import Optional

__all__ = ["ExitCode", "StopCliProcess"]


class ExitCode(enum.Enum):
    CONFIG_NOT_FOUND = 2
    BAD_CONFIG = 3
    ENVIRONMENT_NOT_FOUND = 4
    API_KEY_NOT_FOUND = 5
    APPLY_CANCELED_BY_USER = 6
    API_CONSISTENCY_IS_BROKEN = 7
    INVALID_ARGUMENTS = 8


class StopCliProcess(Exception):
    def __init__(self, message: str, exit_code: Optional[ExitCode] = None, *args):
        if exit_code:
            self.exit_code = exit_code.value
        else:
            self.exit_code = 1
        super().__init__(message, *args)
