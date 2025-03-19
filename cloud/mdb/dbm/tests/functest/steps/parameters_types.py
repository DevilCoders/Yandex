"""
Additional types for parameters
"""

from behave import register_type
from parse import with_pattern


def reg_as_parameter(reg):
    """
    Create type as reg
    """

    @with_pattern(reg)
    def parse_fqdn(text):
        """
        Actually don't parse anything,
        just regexp definition
        """
        return text

    return parse_fqdn


register_type(
    FQDN=reg_as_parameter(r'[\w.-]+'),
    Name=reg_as_parameter(r'[\w.-]+'),
    CPU=reg_as_parameter(r'[\w.-]+'),
)
