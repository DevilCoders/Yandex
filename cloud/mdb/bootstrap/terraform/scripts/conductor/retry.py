"""
Retry helper
"""

import logging
from typing import Callable, Tuple, Type, Union

import tenacity
import tenacity.stop

GiveUpHandler = Callable[[Exception], bool]


def on_exception(exception: Union[Type[Exception], Tuple[Type[Exception], ...]],
                 max_tries: int = 3,
                 factor: float = 1,
                 max_wait: int = 3600,
                 giveup: GiveUpHandler = None) -> tenacity.retry:
    """
    Returns decorator for retry triggered by exception.
    """

    retry_predicate = tenacity.retry_if_exception_type(exception)
    if giveup is not None:
        retry_predicate = tenacity.retry_all(retry_predicate, tenacity.retry_if_exception(lambda e: not giveup(e)))

    # tenacity and backoff both use expo with full_jitter.
    # But:
    # Backoff compute it as:
    #
    #  factor * exp_base ** (retry.attempt_number - 1)
    #
    # https://github.com/litl/backoff/blob/91d25b91107294aa30c5cacd1685ab166ecee66e/backoff/_wait_gen.py#L16-L23
    #
    # > tenacity.wait_exponential
    #
    #     multiplier * self.exp_base ** retry_state.attempt_number
    #
    # https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/tenacity/tenacity/wait.py?rev=4496276#L155-162
    #
    # that's way multiplier=factor/2
    #
    wait = tenacity.wait_random_exponential(multiplier=factor / 2, max=max_wait)
    return tenacity.retry(
        wait=wait,
        retry=retry_predicate,
        reraise=True,  # reraise original exception instead of tenacity.RetryError
        stop=tenacity.stop_after_attempt(max_tries),
        before_sleep=tenacity.before_sleep_log(logging.getLogger('retry'), logging.ERROR),
    )
