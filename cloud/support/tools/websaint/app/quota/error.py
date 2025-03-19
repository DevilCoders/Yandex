#!/usr/bin/env python3
"""This module contains QuotaService errors."""


class QuotaServiceError(Exception):
    pass


class TooManyParams(QuotaServiceError):
    pass


class ServiceError(QuotaServiceError):
    pass


class EnvError(QuotaServiceError):
    pass


class SSLError(QuotaServiceError):
    pass


class CredentialsError(QuotaServiceError):
    pass


class BadRequest(QuotaServiceError):
    pass


class Unauthorized(QuotaServiceError):
    pass


class PermissionDenied(QuotaServiceError):
    pass


class NetworkError(QuotaServiceError):
    pass


class ResourceNotFound(QuotaServiceError):
    pass


class HTTPError(QuotaServiceError):
    pass


class FeatureNotImplemented(QuotaServiceError):
    pass


class ConvertValueError(QuotaServiceError):
    pass


class ValidateError(QuotaServiceError):
    pass


class LogicError(QuotaServiceError):
    pass


class ConfigError(QuotaServiceError):
    pass


class TimedOut(QuotaServiceError):
    def __init__(self):
        super().__init__('Timed out.')
