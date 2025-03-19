"""
Common steps definition
"""

# pylint: disable=no-name-in-module

import time

from behave import then, when


@then('we sleep for {seconds:d} seconds')
@when('we sleep for {seconds:d} seconds')
def step_run_tool(_, seconds):
    """
    Sleep
    """
    time.sleep(seconds)
