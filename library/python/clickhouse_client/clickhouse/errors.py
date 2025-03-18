# coding: utf8

import re


try:
    StandardError  # py2
except NameError:
    class _BaseException(Exception):  # py3
        """ ... """
else:
    class _BaseException(StandardError):  # py2 compat
        """ ... """


try:
    class Error(StandardError):  # py2
        """ Base error class """
except NameError:
    class Error(Exception):  # py3
        """ Base error class """


class DatabaseError(Error):
    """ ... """


class InternalError(DatabaseError):
    """ ... """


class ProgrammingError(DatabaseError):
    """ ... """


class OperationalError(DatabaseError):
    """ ... """

    def __init__(self, *args, **kwargs):
        self.original_exception = kwargs.pop('orig', None)
        super(OperationalError, self).__init__(*args, **kwargs)

    @classmethod
    def from_response(cls, response, **kwargs):
        return cls("{resp.status_code}: {resp.reason}; {resp.content}".format(resp=response), **kwargs)

    @property
    def code(self):
        res = re.search(r'Code: (?P<code>\d+)', str(self))
        if res is not None:
            return int(res.group('code'))
        return -1
