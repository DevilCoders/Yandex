from dbaas_common import retry
from .exceptions import DeadlineExceededError, AuthenticationError, TemporaryUnavailableError, InternalError

client_retry = retry.on_exception(
    (
        DeadlineExceededError,
        AuthenticationError,
        TemporaryUnavailableError,
    ),
    factor=10,
    max_wait=60,
    max_tries=6,
)

client_retry_on_read_method = retry.on_exception(
    (
        DeadlineExceededError,
        AuthenticationError,
        TemporaryUnavailableError,
        InternalError,
    ),
    factor=10,
    max_wait=60,
    max_tries=6,
)
