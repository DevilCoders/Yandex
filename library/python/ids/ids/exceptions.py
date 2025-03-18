# -*- coding: utf-8 -*-

import socket
from requests.exceptions import RequestException


class IDSException(Exception):
    def __init__(self, *args, **kwargs):
        """
        Предполагается, что в extra кладется dict
        с дополнительными параметрами об исключении.
        """
        self.extra = kwargs.pop('extra', None)
        super(IDSException, self).__init__(*args, **kwargs)


class OperationNotPermittedError(IDSException):
    pass


class KeyAlreadyExistsError(IDSException):
    pass


class KeyIsAbsentError(IDSException):
    pass


class ConflictError(IDSException):
    pass


class DuplicateIssueNotFoundError(ConflictError):
    pass


class WrongValueError(IDSException):
    pass


class EmptyIteratorError(IDSException):
    pass


class ResponseError(IDSException):
    def __init__(self, *args, **kwargs):
        response = kwargs.pop('response', None)
        self.response = response
        self.status_code = None if response is None else response.status_code
        super(ResponseError, self).__init__(*args, **kwargs)


class AuthError(ResponseError):
    pass


class BackendError(ResponseError):
    pass


class ConfigurationError(IDSException):
    pass


REQUESTS_ERRORS = (
    RequestException,
    # TODO: после обновления requests до >=1.0 выпилить
    # наш старый requests 0.13 сейчас не ловит
    # socket.error на connection refused
    # https://github.com/kennethreitz/requests/issues/748
    socket.error,
)
