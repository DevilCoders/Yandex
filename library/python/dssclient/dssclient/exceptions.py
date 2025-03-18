
class DssClientException(Exception):
    """Базовое исключение приложения."""


class ConnectionError(DssClientException):
    """Ошибка, связанная с невозможностью подключения к серверу."""

    def __init__(self, msg: str, culprit: Exception = None):
        """
        :param msg:

        :param culprit: Исключение, породившее данное.

        """
        super(ConnectionError, self).__init__(msg)
        self.culprit = culprit


class ValidationError(DssClientException):
    """Ошибка валидации каких-либо данных."""


class ApiCallError(DssClientException):
    """Ошибка при обрашении к ручке API."""
