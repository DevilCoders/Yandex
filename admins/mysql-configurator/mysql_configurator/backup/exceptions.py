"""
Backup related exceptions
"""

class NotRunningException(Exception):
    """Raise when run not called"""
    def __repr__(self):
        return "run method not called: {}".format(str(super(NotRunningException, self)))
