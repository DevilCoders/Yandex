# coding: utf-8

from __future__ import unicode_literals

from decimal import Decimal

import pytest

from cia_stuff.utils import iterables


def test_date_rangeable_get_range():
    iterable = [
        {
            'date': '2015-01-01',
        },
        {
            'date': '2018-02-01',
        },
        {
            'date': '2015-02-01',
        },
        {
            'date': '2015-02-05',
        },
        {
            'date': '2015-03-01',
        },
    ]
    ranged = iterables.DateRangeable(iterable, date_field='date')
    assert ranged.get_range('2015-01-30', '2015-02-10') == [
        {
            'date': '2015-02-01',
        },
        {
            'date': '2015-02-05',
        },
    ]


def test_date_rangeable_get_range_one_date():
    iterable = [
        {
            'date': '2015-01-01',
        },
        {
            'date': '2018-02-01',
        },
        {
            'date': '2015-02-01',
        },
        {
            'date': '2015-03-01',
        },
    ]
    ranged = iterables.DateRangeable(iterable, date_field='date')
    assert ranged.get_range(to_date='2015-02-10') == [
        {
            'date': '2015-01-01',
        },
        {
            'date': '2015-02-01',
        },
    ]


@pytest.mark.parametrize('dates, marker, expected', [
    (
        [
            '2015-01-01',
            '2015-02-01',
            '2015-03-01',
        ],
        '2015-02-05',
        '2015-02-01',
    ),
    (
        [],
        '2015-02-05',
        None,
    ),
    (
        [
            '2015-01-01',
            '2015-02-01',
            '2015-03-01',
        ],
        '2015-03-05',
        '2015-03-01',
    ),
    (
        [
            '2015-01-01',
            '2015-02-01',
            '2015-03-01',
        ],
        '2014-03-05',
        None,
    ),
])
def test_date_rangeable_get_closest_before(dates, marker, expected):
    ranged = iterables.DateRangeable(
        [{'date': date} for date in dates],
        date_field='date',
    )
    closest_item = ranged.get_closest_before(marker)
    closest_date = closest_item and closest_item['date']
    assert closest_date == expected


def test_get_range_values_sum():
    iterable = [
        {
            'date': '2015-01-01',
            'value': 10,
        },
        {
            'date': '2015-02-01',
            'value': 5,
        },
        {
            'date': '2015-02-01',
            'value': 5,
        },
        {
            'date': '2015-03-01',
            'value': 10,
        },
    ]
    ranged = iterables.DateRangeable(
        iterable,
        date_field='date',
        value_field='value',
    )
    values_sum = ranged.get_range_values_sum(
        from_date='2015-02-01',
        to_date='2015-03-01',
    )
    assert values_sum == 10


def test_get_range_values_sum_with_nulls():
    iterable = [
        {
            'date': '2015-01-01',
            'value': None,
        },
        {
            'date': '2015-02-01',
            'value': 5,
        },
    ]
    ranged = iterables.DateRangeable(
        iterable,
        date_field='date',
        value_field='value',
    )
    values_sum = ranged.get_range_values_sum()
    assert values_sum == 5


def test_empty_dates_should_be_skipped():
    iterable = [
        {
            'date': None,
        },
        {
            'date': '2018-02-01',
        },
    ]
    ranged = iterables.DateRangeable(iterable, date_field='date')
    assert len(ranged) == 1
    assert ranged.latest() == {'date': '2018-02-01'}


def test_sorting():
    iterable = [
        {
            'date': '2018-01-01',
        },
        {
            'date': '2017-01-01',
        },
    ]
    ranged = iterables.DateRangeable(iterable, date_field='date')
    assert ranged[0] == {'date': '2017-01-01'}
    assert ranged[1] == {'date': '2018-01-01'}


def test_wrapper():
    iterable = [
        {
            'date': '2015-02-01',
            'value': 5.5,
        },
    ]
    ranged = iterables.DateRangeable(
        iterable,
        value_wrapper=Decimal,
        value_field='value',
        date_field='date',
    )
    wrapped_value = ranged.get_value_from_item(ranged[0])
    assert wrapped_value == Decimal('5.5')
