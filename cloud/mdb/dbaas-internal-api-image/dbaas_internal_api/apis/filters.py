# coding: utf-8
"""
DBaaS Internal API filters helpers
"""

from datetime import date, datetime
from typing import Dict, List, NamedTuple, Optional, Tuple, Union, cast

from ..core.exceptions import DbaasClientError, DbaasNotImplementedError
from ..utils.filters_parser import Filter, Operator

AttributeType = Union[type, Tuple[type, ...]]

FilterConstraint = NamedTuple('FilterConstraint', [('type', AttributeType), ('operators', List[Operator])])

_FILTER_TYPES = {
    date: 'a timestamp',
    datetime: 'a timestamp',
    str: 'a string',
    int: 'a number',
}


def _describe_types(types_tuple):
    if not isinstance(types_tuple, tuple):
        return _FILTER_TYPES[types_tuple]
    return ' or '.join(set(_describe_types(t) for t in types_tuple))


def _type_is_ok(value, attribute_type):
    if isinstance(value, list):
        return all(isinstance(sub_value, attribute_type) for sub_value in value)
    return isinstance(value, attribute_type)


def verify_filters(parsed_filters: Optional[List[Filter]], constraints: Dict[str, FilterConstraint]) -> None:
    """
    Verify that filters matches its constraints
    """
    # None or empty dict is okay
    if not parsed_filters:
        return
    for flt in parsed_filters:
        if flt.attribute not in constraints:
            raise DbaasClientError(
                "Filter by '{flt.attribute}' " "('{flt.filter_str}') is not supported.".format(flt=flt)
            )
        check = constraints[flt.attribute]
        if not _type_is_ok(flt.value, check.type):
            raise DbaasClientError(
                "Filter '{flt.filter_str}' has "
                "wrong '{flt.attribute}' attribute type. Expected {type}.".format(
                    flt=flt, type=_describe_types(check.type)
                )
            )
        if flt.operator not in check.operators:
            raise DbaasNotImplementedError("Operator '{flt.operator.value}' not implemented.".format(flt=flt))


# shortcut for define name filter
NAME_EQUALS = {
    'name': FilterConstraint(
        type=str,
        operators=[Operator.equals],
    ),
}


def get_name_filter(parsed_filters: Optional[List[Filter]]) -> Optional[str]:
    """
    Verify that we got only one filter with `name=`.
    Return filter.value if it exists
    """
    verify_filters(parsed_filters, NAME_EQUALS)
    if parsed_filters:
        if len(parsed_filters) > 1:
            raise DbaasNotImplementedError('Only one condition in filter implemented now')
        return cast(str, parsed_filters[0].value)
    return None
