#!/usr/bin/env python3
"""This module contains custom errors."""


class CoreError(Exception):
    pass


class ConfigError(CoreError):
    pass


class EmployeeNotExist(CoreError):
    pass


class BadRequest(CoreError):
    pass


class Unauthorized(CoreError):
    pass


class PermissionDenied(CoreError):
    pass


class NetworkError(CoreError):
    pass


class NotFound(CoreError):
    pass


class HTTPError(CoreError):
    pass


class FeatureNotImplemented(CoreError):
    pass


class ValidateError(CoreError):
    pass


class LogicError(CoreError):
    pass


class SelectionError(CoreError):
    pass


class RespsError(CoreError):
    pass

class StaffError(CoreError):
    pass

class TimedOut(CoreError):
    def __init__(self):
        super().__init__('Timed out.')
