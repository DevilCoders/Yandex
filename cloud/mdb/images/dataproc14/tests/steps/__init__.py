"""
Module with definition of test steps.
"""

import parse
from behave import register_type


@parse.with_pattern(r'[^"]+')
def parse_parameter(text):
    """
    Match anything except "
    """
    return text


register_type(Param=parse_parameter)
