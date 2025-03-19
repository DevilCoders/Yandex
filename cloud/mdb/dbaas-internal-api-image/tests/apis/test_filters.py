"""
Test API filters helpers
"""

from datetime import date, datetime

import pytest

from dbaas_internal_api.apis.filters import FilterConstraint, verify_filters
from dbaas_internal_api.core.exceptions import DbaasClientError, DbaasNotImplementedError
from dbaas_internal_api.utils.filters_parser import Filter, Operator

# pylint: disable=missing-docstring, invalid-name


class Test_verify_filters:
    str_equals = FilterConstraint(
        type=str,
        operators=[Operator.equals],
    )
    date_equals = FilterConstraint(
        type=(datetime, date),
        operators=[Operator.equals, Operator.in_],
    )

    def test_success_for_one_filter(self):
        verify_filters(
            [
                Filter(
                    attribute='name', value='cluster_name', operator=Operator.equals, filter_str='name="cluster_name"'
                ),
            ],
            {'name': self.str_equals},
        )

    @pytest.mark.parametrize('empty_filters', [None, []])
    def test_success_for_empty_filters(self, empty_filters):
        verify_filters(empty_filters, {'name': self.str_equals})

    @pytest.mark.parametrize('date_value', [date.today(), datetime.now(), [date.today()], [datetime.now()]])
    def test_successes_for_date_field(self, date_value):
        verify_filters(
            [
                Filter(
                    attribute='created', value=date_value, operator=Operator.equals, filter_str='name="cluster_name"'
                ),
            ],
            {
                'created': FilterConstraint(
                    type=(datetime, date),
                    operators=[Operator.equals, Operator.in_],
                ),
            },
        )

    def test_unsupported_attribute_name(self):
        with pytest.raises(
            DbaasClientError, match=(r"Filter by 'description' " r"\('description=\"Description\"'\) is not supported")
        ):
            verify_filters(
                [
                    Filter(
                        attribute='description',
                        value='Description',
                        operator=Operator.equals,
                        filter_str='description="Description"',
                    ),
                ],
                {'name': self.str_equals},
            )

    def test_unsupported_attribute_type(self):
        with pytest.raises(
            DbaasClientError,
            match=r"Filter 'name=42' has wrong 'name' attribute type\. " r"Expected a string\.",
        ):
            verify_filters(
                [
                    Filter(attribute='name', value=42, operator=Operator.equals, filter_str='name=42'),
                ],
                {'name': self.str_equals},
            )

    def test_unsupported_attribute_type_tuple(self):
        with pytest.raises(
            DbaasClientError,
            match=r"Filter 'created=42' has wrong 'created' attribute type\. " r"Expected a timestamp\.",
        ):
            verify_filters(
                [
                    Filter(attribute='created', value=42, operator=Operator.equals, filter_str='created=42'),
                ],
                {'created': self.date_equals},
            )

    def test_unsupported_operator(self):
        with pytest.raises(
            DbaasNotImplementedError,
            match=r"Operator '<' not implemented.",
        ):
            verify_filters(
                [
                    Filter(attribute='name', value="foo", operator=Operator.less, filter_str='name<"foo"'),
                ],
                {'name': self.str_equals},
            )
