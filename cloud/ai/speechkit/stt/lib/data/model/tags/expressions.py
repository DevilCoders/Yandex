"""
Module for work with boolean tags expressions.

Supports only disjunction of tags conjunctions, parenthesis are not supported.

Input

CLIENT:mtt-u16 MODE:stream PERIOD:2020-05
IMPORT:yandex-call-center PERIOD:2020-03

corresponds to expression

CLIENT:mtt-u16 AND MODE:stream AND PERIOD:2020-05
OR
IMPORT:yandex-call-center AND PERIOD:2020-03
"""

import typing

from .model import RecordTagDataRequest


def parse_tag_conjunctions(tag_filters: typing.List[str]) -> typing.List[typing.Set[RecordTagDataRequest]]:
    """
    Input parser.
    """
    tag_conjunctions = []
    for tag_filter in tag_filters:
        tag_conjunction = set()
        for tag in tag_filter.split():
            tag_conjunction.add(RecordTagDataRequest.from_str(tag))
        tag_conjunctions.append(tag_conjunction)
    return tag_conjunctions


def validate_tag_conjunctions(tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]]):
    """
    Conjunction must not be subset of another conjunction.

    Does not process type-only and negation filters properly.
    """
    for i, curr_conjunction in enumerate(tag_conjunctions):
        other_conjunctions = tag_conjunctions.copy()
        other_conjunctions.pop(i)
        for other_conjunction in other_conjunctions:
            if curr_conjunction.issubset(other_conjunction):
                raise ValueError(
                    f'Tag conjunction {curr_conjunction} is subset of other tag conjunction {other_conjunction}'
                )
