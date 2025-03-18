
class BclClientException(Exception):
    """Базовое исключение приложения."""


class ConnectionError(BclClientException):
    """Ошибка, связанная с невозможностью подключения к серверу."""


class ValidationError(BclClientException):
    """Ошибка валидации каких-либо данных."""


class ApiCallError(BclClientException):
    """Ошибка при обрашении к ручке API."""

    def __init__(self, message, status_code):
        super(ApiCallError, self).__init__(message)

        self.msg = message
        self.status_code = int(status_code)
