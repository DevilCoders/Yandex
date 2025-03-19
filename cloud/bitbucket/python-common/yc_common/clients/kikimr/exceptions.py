"""Provides generic KiKiMR exceptions"""

from yc_common.clients.kikimr.config import KikimrEndpointConfig
from yc_common.exceptions import Error


class KikimrError(Error):
    parent_error = None


class ValidationError(KikimrError):
    pass


class KikimrConsistencyError(KikimrError):
    pass


class QueryError(KikimrError):
    def __init__(self, query, message, *args):
        if args:
            message = message.format(*args)

        super().__init__("`{}` query has failed ({}): {}.", query, type(self), message.rstrip("."))
        self.query = query
        self.message = message


class CompileQueryError(QueryError):
    """Errors while compile query, which can be workaround by fallback to text query"""
    pass


class BadRequest(QueryError):
    pass


class PathNotFoundError(QueryError):
    pass


class NotDirectoryError(QueryError):
    pass


class NotTableError(QueryError):
    pass


class DirectoryIsNotEmptyError(QueryError):
    pass


class TableNotFoundError(QueryError):
    pass


class ColumnNotFoundError(QueryError):
    pass


class ColumnAlreadyExistsError(QueryError):
    pass


class PreconditionFailedError(QueryError):
    pass


class UnretryableConnectionError(QueryError):
    pass


class YdbCreateSessionTimeout(KikimrError):
    pass


class RetryableError(QueryError):
    def __init__(self, database_config: KikimrEndpointConfig, query, message, *args, request_timeout=None,
                 retry_timeout=None, sleep_multiply=1, sleep_max_time=None):
        super().__init__(*((query, message) + args))
        self.request_timeout = database_config.request_timeout if request_timeout is None else request_timeout
        self.retry_timeout = database_config.retry_timeout if retry_timeout is None else retry_timeout
        self.sleep_multiply = sleep_multiply
        self.sleep_max_time = sleep_max_time


class RetryableConnectionError(RetryableError):
    pass


class RetryableRequestTimeoutError(RetryableConnectionError):
    pass


class RetryableSessionTimeoutError(RetryableConnectionError):
    pass


class RetryableErrorExponentionalBackooff(RetryableError):
    def __init__(self, *args, **kwargs):
        kwargs.setdefault("sleep_multiply", 3)
        kwargs.setdefault("sleep_max_time", 60)
        super().__init__(*args, **kwargs)


class YdbOverloaded(RetryableErrorExponentionalBackooff):
    pass


class YdbUnavailable(RetryableError):
    pass


class YdbDeadlineExceeded(RetryableErrorExponentionalBackooff):
    pass

class TransactionLocksInvalidatedError(RetryableError):
    pass


class BadSession(QueryError):
    pass
