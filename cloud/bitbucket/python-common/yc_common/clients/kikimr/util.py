"""Various utilities."""

import functools
import random
import time

from yc_common import logging
from yc_common.clients.kikimr import metrics
from yc_common.clients.kikimr.exceptions import QueryError, KikimrError, KikimrConsistencyError, RetryableError, RetryableConnectionError, \
    RetryableRequestTimeoutError, UnretryableConnectionError, ValidationError

log = logging.get_logger(__name__)

_RANDOM = random.SystemRandom()


class custom_retry_idempotent_kikimr_errors:
    def __init__(self, max_conflicts=5, warning_conflicts_on=None, error_conflicts_on=None, conflicts_expected=False):
        self.__max_conflicts = max_conflicts

        if error_conflicts_on is None:
            if conflicts_expected:
                error_conflicts_on = max_conflicts
            else:
                error_conflicts_on = max_conflicts // 2 + 1
        self.__error_conflicts_on = error_conflicts_on

        if warning_conflicts_on is None:
            if conflicts_expected:
                warning_conflicts_on = max_conflicts
            else:
                warning_conflicts_on = 1
        self.__warning_conflicts_on = warning_conflicts_on

    def __call__(self, func):
        @functools.wraps(func)
        def decorator(*args, **kwargs):
            try_number = 1
            retry_period = 1

            connection_try_number = 1
            connection_errors_tracking_start_time = time.monotonic()

            while True:
                try:
                    try:
                        metrics.query_blocks_counter.inc()
                        return func(*args, **kwargs)
                    except Exception as e:
                        if not isinstance(e, RetryableConnectionError):
                            connection_try_number = 1
                            connection_errors_tracking_start_time = time.monotonic()

                        raise
                except RetryableConnectionError as e:
                    # Connection errors are retried until retry timeout is exceeded

                    if isinstance(e, RetryableRequestTimeoutError):
                        sleep_time = 0
                        expected_request_time = e.request_timeout
                    else:
                        sleep_time = 1
                        expected_request_time = 1

                    if (connection_errors_tracking_start_time + e.retry_timeout) - (time.monotonic() + sleep_time) > expected_request_time:
                        log.warning("%s (%s) Retrying idempotent errors (#%s)...", e, type(e), connection_try_number)
                        connection_try_number += 1
                        time.sleep(sleep_time)
                    else:
                        metrics.query_block_errors_counter.labels(metrics.QueryBlockErrorTypes.CONNECTION).inc()
                        # Translate it to generic error to eliminate a possible occasional cascading retry
                        error = QueryError(e.query, "{!r} error after {} tries.", e.message, connection_try_number)
                        error.parent_error = e
                        raise error
                except (RetryableError, KikimrConsistencyError) as e:
                    # Conflict errors are retried with a fixed number of tries
                    if try_number >= self.__max_conflicts:
                        metrics.query_block_errors_counter.labels(metrics.QueryBlockErrorTypes.QUERY).inc()
                        # Translate it to generic error to eliminate a possible occasional cascading retry
                        if isinstance(e, RetryableError):
                            error = QueryError(e.query, "{!r} error after {} tries.", e.message, try_number)
                        else:
                            error = KikimrError("{!r} error after {} tries.", str(e), try_number)
                        error.parent_error = e
                        raise error

                    if try_number >= self.__error_conflicts_on:
                        log_func = log.error
                    elif try_number >= self.__warning_conflicts_on:
                        log_func = log.warning
                    else:
                        log_func = log.debug

                    sleep_time = retry_period
                    if isinstance(e, RetryableError):
                        retry_period *= e.sleep_multiply

                        if e.sleep_max_time is not None:
                            if sleep_time > e.sleep_max_time:
                                sleep_time = e.sleep_max_time

                            if retry_period > e.sleep_max_time:
                                retry_period = e.sleep_max_time

                    sleep_time *= _RANDOM.random()

                    log_func(
                        "%s (%s) Retrying idempotent errors (%s of %s, sleep %.1f seconds)...",
                        e, type(e), try_number, self.__max_conflicts - 1, sleep_time)

                    if sleep_time > 0:
                        time.sleep(sleep_time)

                    try_number += 1
                except UnretryableConnectionError:
                    metrics.query_block_errors_counter.labels(metrics.QueryBlockErrorTypes.CONNECTION).inc()
                    raise
                except QueryError:
                    metrics.query_block_errors_counter.labels(metrics.QueryBlockErrorTypes.UNKNOWN).inc()
                    raise

        return decorator


retry_idempotent_kikimr_errors = custom_retry_idempotent_kikimr_errors()


class ColumnStrippingStrategy:
    NONE = False  # This value is for backward compatibility
    STRIP = True  # This value is for backward compatibility
    STRIP_AND_MERGE = "strip-and-merge"


def strip_table_name_from_row(row, stripping_strategy):
    for column_name in list(row.keys()):
        dot_pos = column_name.rfind(".")
        if dot_pos < 0:
            continue

        new_column_name = column_name[dot_pos + 1:]
        if not new_column_name:
            raise ValidationError("Invalid column name: {!r}.", column_name)

        if new_column_name in row and (
                stripping_strategy != ColumnStrippingStrategy.STRIP_AND_MERGE or
                row[column_name] != row[new_column_name]
        ):
            raise ValidationError(
                "Column name conflict after stripping table name from {!r} column.", column_name)

        row[new_column_name] = row.pop(column_name)
