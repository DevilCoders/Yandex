from .base import BaseClientsException


class PassportException(BaseClientsException):
    pass


class DomainNotFound(PassportException):
    pass


class DomainAliasNotFound(PassportException):
    pass


class DomainAlreadyExists(PassportException):
    pass


class DomainAliasAlreadyExists(PassportException):
    pass
