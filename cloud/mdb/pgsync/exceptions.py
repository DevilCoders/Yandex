# coding: utf8
"""
Describes exception classes used in PGSync.
"""


class PGSyncException(Exception):
    """
    Generic pgsync exception.
    """

    pass


class SwitchoverException(PGSyncException):
    """
    Exception for fatal errors during switchover.
    """

    pass
