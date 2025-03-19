"""Custom exceptions for backend core"""

from bootstrap.common.exceptions import BootstrapError


class DbInMigrationError(BootstrapError):
    """Error is raised when accessing database which is in migration process at the moment"""
    pass


class UnsupportedDbVersionError(BootstrapError):
    """Error is raised when accessing database with unsuppoorted version"""
    pass
