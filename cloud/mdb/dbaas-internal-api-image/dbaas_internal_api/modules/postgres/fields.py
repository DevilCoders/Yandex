# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL marshmallow custom fields
"""

import humanfriendly
from apispec.ext.marshmallow.openapi import DEFAULT_FIELD_MAPPING
from marshmallow.fields import Field
from marshmallow.validate import Range, Validator


class UnitMinMaxValidator(Validator):
    """
    This is simple "non-validating" validator.
    It has min and max property as Range validator (this is used in openapi spec generation).
    """

    def __init__(self, field):
        self._field = field

    def __call__(self, value):
        return value

    @property
    def min(self):
        """
        Return min for field2range renderer
        """
        return self._field.validator.min

    @property
    def max(self):
        """
        Return max for field2range renderer
        """
        return self._field.validator.max


class PostgresqlUnitField(Field):
    """
    Field for PostgreSQL setting with units
    """

    unit_suffix = ''
    examples = ['1', '2', '3']
    default_error_messages = {'invalid': 'Not a valid integer.'}

    # pylint: disable=redefined-builtin
    def __init__(self, min=None, max=None, **kwargs):
        super().__init__(**kwargs)
        self.validator = Range(min=min, max=max)
        # This actually does nothing but helps openapi converter in min/max extraction
        self.validators = [UnitMinMaxValidator(self)]

    def _to_string(self, value):
        """
        Returns value as is if it is <= 0.
        Otherwise return value with units.
        """
        return '{value}{unit}'.format(value=int(value), unit=self.unit_suffix)

    def _get_int(self, value):
        """
        Returns int or None
        """
        try:
            int_value = int(value)
        except (ValueError, TypeError):
            int_value = None
        return int_value

    def _parse_int_value(self, value):
        """
        Function is overwritten
        """
        return value

    def fail(self, key='invalid', **kwargs):
        super().fail(key, unit_suffix=self.unit_suffix, examples=', '.join(self.examples), **kwargs)

    def _deserialize(self, value, attr, data):
        # Int to str units(ms,Kb)
        int_value = self._get_int(value)
        # Possible to have 0,-1 and it should be without postfix
        if int_value is not None and int_value <= 0:
            self.validator(int_value)
            return int_value
        res = self._parse_int_value(value)
        return res

    def _serialize(self, value, attr=None, obj=None):
        # Str Units(ms,Kb) to Int
        int_value = self._get_int(value)
        # Possible to have 0,-1 and it should be without postfix
        if int_value is not None and int_value <= 0:
            return int_value
        return self._unit_to_int(value)

    def _unit_to_int(self, value):
        # Int to str units(ms,Kb)
        return value


class PostgresqlTimespanMs(PostgresqlUnitField):
    """
    Field for PostgreSQL time settings like `lock_timeout`.
    Stores value in milliseconds.
    """

    unit_suffix = 'ms'

    def _parse_int_value(self, value):
        """
        Returns value with units if it is number.
        Otherwise returns None.
        """
        # Try cast to int if value specified without suffix
        # (in units)
        int_value = self._get_int(value)
        if int_value is None:
            self.fail()
        self.validator(int_value)
        return self._to_string(int_value)

    def _unit_to_int(self, value):
        """
        Parses value from human readable format.
        """
        try:
            # humanfriendly returns value in seconds.
            # We need to convert it to milliseconds.
            return int(humanfriendly.parse_timespan(str(value)) * 1000)
        except humanfriendly.InvalidTimespan:
            self.fail()


class PostgresqlSizeKB(PostgresqlUnitField):
    """
    Field for PostgreSQL size settings like `temp_file_limit`.
    Stores value in kilobytes.
    """

    unit_suffix = 'kB'
    store_multiplier = 1024
    default_error_messages = {
        'invalid': 'Not a valid integer.' ' Should multiple by {}'.format(store_multiplier),
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def _parse_int_value(self, value):
        """
        Returns value with units if it is number.
        Otherwise returns None.
        """
        # Try cast to int if value specified without suffix
        # (in units)
        int_value = self._get_int(value)
        if int_value is None or int_value % 1024 != 0:
            self.fail()
        self.validator(int_value)
        res = int_value / self.store_multiplier
        return self._to_string(res)

    def _unit_to_int(self, value):
        """
        Parses value from human readable format.
        """
        try:
            # Store in Kb need to convert to bytes
            return int(humanfriendly.parse_size(value, binary=True))
        except humanfriendly.InvalidSize:
            self.fail()


class PostgresqlSizeMB(PostgresqlSizeKB):
    """
    Field for PostgreSQL size settings like `temp_file_limit`.
    Stores value in megabytes.
    """

    unit_suffix = 'MB'
    store_multiplier = 1024 * 1024


# We use adding to DEFAULT_FIELD_MAPPING instead of OpenAPIConverter decorator
# because no instance of spec is available
DEFAULT_FIELD_MAPPING[PostgresqlUnitField] = ("integer", "int32")
