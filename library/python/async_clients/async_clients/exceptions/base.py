from typing import Optional


class BaseClientsException(Exception):
    def __init__(self, message: Optional[str] = None, **kwargs):
        super().__init__(message)
        for key, value in kwargs.items():
            setattr(self, key, value)


class NoRetriesLeft(BaseClientsException):
    pass


class BadResponseStatus(BaseClientsException):
    pass


class AIOHTTPClientException(BaseClientsException):
    pass


class AuthKwargsMissing(BaseClientsException):
    pass


class InitClientError(BaseClientsException):
    pass
