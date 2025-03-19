# -*- coding: utf-8 -*-
"""
PostgreSQL fields tests
"""
import pytest
from marshmallow.exceptions import ValidationError

from dbaas_internal_api.modules.postgres.fields import PostgresqlSizeKB, PostgresqlSizeMB, PostgresqlTimespanMs

# pylint: disable=missing-docstring, invalid-name


@pytest.mark.parametrize(
    ['field', 'value'],
    [
        (PostgresqlTimespanMs(), 'test'),
        (PostgresqlTimespanMs(), '10ss'),
        (PostgresqlTimespanMs(), '-10ms'),
        (PostgresqlTimespanMs(min=-1), '-2'),
        (PostgresqlTimespanMs(min=-1), -2),
        (PostgresqlTimespanMs(min=10), 9),
        (PostgresqlSizeKB(), 'test'),
        (PostgresqlSizeKB(), '10sdf'),
        (PostgresqlSizeKB(), '-10kB'),
        (PostgresqlSizeKB(), 1000),
        (PostgresqlSizeKB(min=-1), '-2'),
        (PostgresqlSizeKB(min=-1), -2),
        (PostgresqlSizeKB(min=10), 9),
        (PostgresqlSizeMB(), 'test'),
        (PostgresqlSizeMB(), '10sdf'),
        (PostgresqlSizeMB(), '-10MB'),
        (PostgresqlSizeKB(), 10000000),
        (PostgresqlSizeMB(min=-1), '-2'),
        (PostgresqlSizeMB(min=-1), -2),
        (PostgresqlSizeMB(min=10), 9),
        (PostgresqlSizeMB(max=100), 101),
    ],
)
def test_invalid_value(field, value):
    with pytest.raises(ValidationError):
        field.deserialize(value)


@pytest.mark.parametrize(
    ['field', 'value', 'result'],
    [
        (PostgresqlTimespanMs(), 10, '10ms'),
        (PostgresqlSizeKB(), 1024, '1kB'),
        (PostgresqlSizeKB(), 8388608, '8192kB'),
        (PostgresqlSizeKB(), 12884901888, '12582912kB'),
        (PostgresqlSizeMB(), 8388608, '8MB'),
        (PostgresqlSizeMB(), 12884901888, '12288MB'),
    ],
)
def test_valid_value_deserialize(field, value, result):
    assert field.deserialize(value) == result


@pytest.mark.parametrize(
    ['field', 'value', 'result'],
    [
        (PostgresqlTimespanMs(), '10ms', 10),
        (PostgresqlTimespanMs(), '10s', 10000),
        (PostgresqlTimespanMs(), '1h', 3600000),
        (PostgresqlSizeKB(), '1kB', 1024),
        (PostgresqlSizeMB(), '8MB', 8388608),
        (PostgresqlSizeKB(), '12GB', 12884901888),
    ],
)
def test_valid_value_serialize(field, value, result):
    assert field.serialize('value', {'value': value}) == result
