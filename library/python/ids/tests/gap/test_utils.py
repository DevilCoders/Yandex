# coding: utf-8
from __future__ import unicode_literals

from datetime import date

import pytest

from ids.services.gap import utils


def test_serialize_period():
    assert utils.serialize_period(date(2022, 5, 10)) == '2022-05-10'
    assert utils.serialize_period('2022-05-10') == '2022-05-10'


def test_date_from_string():
    function = utils.date_from_gap_dt_string
    assert function('2022-05-10T00:00:00') == date(2022, 5, 10)


@pytest.mark.parametrize(
    "input,expected",
    [
        ('batman', ['batman']),
        (['batman', 'robin'], ['batman', 'robin']),
    ]
)
def test_fetch_logins(input, expected):
    assert utils.smart_fetch_logins(input) == expected
