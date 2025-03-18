ERROR_KEY = "error"
ERROR_CODE_KEY = "code"
ERROR_CODE_ALREADY_EXISTS = "already_exists"


class UnretriableError(Exception):
    """
    This custom exception class is used to omit retries for a number of http errors we do not want to retry
    """

    def __init__(self, message, error_data=None):
        super().__init__(message)

        self._error_data = error_data

    @property
    def error_data(self):
        return self._error_data


class ApprovementAlreadyExists(UnretriableError):
    """
    Raised as a result of an attempt to create approvement for ticket that has already got a running approvement

    Note: `error_data` contains the existing approvement UUID
    """
    pass


def raise_unretriable_error(message: str, error_data: dict):
    """
    Build and raise an unretriable error conforming the given error_data.

    :param message: Error message
    :param error_data: Dictified JSON response received from OK service
    """

    if not error_data or not isinstance(error_data, dict) or ERROR_KEY not in error_data:
        raise UnretriableError(message)

    if len(error_data[ERROR_KEY]) == 1 and error_data[ERROR_KEY][0][ERROR_CODE_KEY] == ERROR_CODE_ALREADY_EXISTS:
        raise ApprovementAlreadyExists(message, error_data)

    raise UnretriableError(message)
