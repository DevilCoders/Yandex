from .base import BaseClientsException


class WebmasterException(BaseClientsException):
    pass


class UnAllowedVerificationType(WebmasterException):
    pass


class DurationIsRequired(WebmasterException):
    pass
