"""
Workarounds for our tests
"""

import retrying

from tests import configuration


def retry(wait_fixed, stop_max_attempt_number, **kwargs):
    """
    Like retrying.retry, but with optional multiplier
    """
    multiplier = configuration.static()['retries_multiplier']
    return retrying.retry(
        wait_fixed=wait_fixed * multiplier, stop_max_attempt_number=stop_max_attempt_number * multiplier, **kwargs
    )
