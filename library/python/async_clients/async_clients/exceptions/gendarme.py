from .base import BaseClientsException


class GendarmeException(BaseClientsException):
    pass


class DomainNameTooLongException(GendarmeException):
    pass


class DomainNotFoundException(GendarmeException):
    pass
