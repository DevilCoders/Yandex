#!/usr/bin/env python3
"""This module contains Exception classes."""


class MetricsCollectorError(Exception):
    pass


class ConfigError(MetricsCollectorError):
    pass


class IssueInitError(MetricsCollectorError):
    pass


class DatabaseError(MetricsCollectorError):
    pass
