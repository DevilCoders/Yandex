# -*- encoding: utf-8 -*-
from __future__ import unicode_literals


class RefsClientException(Exception):
    """Базовое исключение приложения."""


class ConnectionError(RefsClientException):
    """Ошибка, связанная с невозможностью подключения к серверу."""


class ValidationError(RefsClientException):
    """Ошибка валидации каких-либо данных."""


class ApiCallError(RefsClientException):
    """Ошибка при обрашении к ручке API."""

    def __init__(self, message, status_code):
        super(ApiCallError, self).__init__(message)

        self.status_code = int(status_code)
