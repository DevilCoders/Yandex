import datetime as dt
import re

import marshmallow
import pytest

from dbaas_internal_api.apis.schemas.fields import BooleanMappedToInt, Environment
from dbaas_internal_api.apis.schemas.fields import Filter as FilterField
from dbaas_internal_api.apis.schemas.fields import (
    GrpcInt,
    GrpcStr,
    Labels,
    MappedEnum,
    UpperToLowerCaseEnum,
    XmlEscapedStr,
)
from dbaas_internal_api.utils.filters_parser import Filter as FilterObj
from dbaas_internal_api.utils.filters_parser import Operator

# pylint: disable=missing-docstring, invalid-name


class TestMappedEnum:
    class Schema(marshmallow.Schema):
        status = MappedEnum({'ONLINE': 'online', 'OFFLINE': 'offline'})

    def test_load_succeeded(self):
        data, _errors = self.Schema().load({'status': 'ONLINE'})
        assert data == {'status': 'online'}

    def test_load_failed(self):
        _data, errors = self.Schema().load({'status': 'INVALID'})
        _assert_mapped_enum_errors(errors, 'INVALID', ['ONLINE', 'OFFLINE'])

    def test_dump_succeeded(self):
        data, _errors = self.Schema().dump({'status': 'online'})
        assert data == {'status': 'ONLINE'}

    def test_dump_failed(self):
        _data, errors = self.Schema().dump({'status': 'INVALID'})
        _assert_mapped_enum_errors(errors, 'INVALID', ['online', 'offline'])


class TestEnvironment:
    class Schema(marshmallow.Schema):
        env = Environment(
            lambda: {
                'qa': 'prestable',
                'prod': 'production',
            }
        )

    def test_load_succeeded(self):
        data, _errors = self.Schema().load({'env': 'PRODUCTION'})
        assert data == {'env': 'prod'}

    def test_load_failed(self):
        _data, errors = self.Schema().load({'env': 'INVALID'})
        _assert_env_field_errors(errors, 'INVALID', ['PRESTABLE', 'PRODUCTION'])

    def test_dump_succeeded(self):
        data, _errors = self.Schema().dump({'env': 'qa'})
        assert data == {'env': 'PRESTABLE'}

    def test_dump_failed(self):
        _data, errors = self.Schema().dump({'env': 'INVALID'})
        _assert_env_field_errors(errors, 'INVALID', ['qa', 'prod'])


class UpperToLowerCaseEnumTest:
    class Schema(marshmallow.Schema):
        logLevel = UpperToLowerCaseEnum(('DEBUG', 'INFO'), attribute='log_level')

    def test_load_succeeded(self):
        data, _errors = self.Schema().load({'logLevel': 'INFO'})
        assert data == {'log_level': 'info'}

    def test_load_failed(self):
        _data, errors = self.Schema().load({'logLevel': 'INVALID'})
        _assert_log_level_errors(errors, 'INVALID', ['DEBUG', 'INFO'])

    def test_dump_succeeded(self):
        data, _errors = self.Schema().dump({'log_level': 'debug'})
        assert data == {'logLevel': 'DEBUG'}

    def test_dump_failed(self):
        _data, errors = self.Schema().dump({'log_level': 'INVALID'})
        _assert_log_level_errors(errors, 'INVALID', ['debug', 'info'])


def _assert_mapped_enum_errors(
    errors, invalid_value, allowed_values, error_field='status', error_re='Invalid value \'(.*)\', allowed values: (.*)'
):
    assert len(errors) == 1

    status_errors = errors[error_field]
    assert len(status_errors) == 1

    error = status_errors[0]
    match = re.fullmatch(error_re, error)
    assert match
    assert match.group(1) == invalid_value
    assert sorted(match.group(2).split(', ')) == sorted(allowed_values)


def _assert_env_field_errors(errors, invalid_value, allowed_values):
    _assert_mapped_enum_errors(
        errors,
        invalid_value,
        allowed_values,
        error_field='env',
        error_re='Invalid environment \'(.*)\', allowed values: (.*)',
    )


def _assert_log_level_errors(errors, invalid_value, allowed_values):
    _assert_mapped_enum_errors(
        errors,
        invalid_value,
        allowed_values,
        error_field='logLevel',
        error_re='Invalid value \'(.*)\', allowed values: (.*)',
    )


class TestLabels:
    class Schema(marshmallow.Schema):
        labels = Labels()

    def test_load_succeeded(self):
        data, _ = self.Schema().load({'labels': {'env': 'prod'}})
        assert data == {'labels': {'env': 'prod'}}

    def test_label_with_small_name_is_ok(self):
        data, _ = self.Schema().load({'labels': {'e': 'prod'}})
        assert data == {'labels': {'e': 'prod'}}

    def test_label_with_empty_value_is_ok(self):
        data, _ = self.Schema().load({'labels': {'env': ''}})
        assert data == {'labels': {'env': ''}}

    def test_load_failed_for_invalid_key_type(self):
        _, errors = self.Schema().load({'labels': {1: 'xxx'}})
        assert errors == {'labels': ['Key must be a string.']}

    def test_load_failed_for_invalid_value_type(self):
        _, errors = self.Schema().load({'labels': {'yyy': b'binary data'}})
        assert errors == {'labels': ['Value must be a string.']}

    @pytest.mark.parametrize(
        ['label_key', 'invalid_char'],
        [
            ('fOO', 'OO'),
            ('wЫ', 'Ы'),
        ],
    )
    def test_load_failed_when_key_contains_invalid_chars(self, label_key, invalid_char):
        _, errors = self.Schema().load({'labels': {label_key: 'foo'}})
        assert errors == {'labels': ['Symbol \'%s\' not allowed in key.' % invalid_char]}

    @pytest.mark.parametrize(
        ['label_value', 'invalid_char'],
        [
            ('fOO', 'OO'),
            ('wЫ', 'Ы'),
        ],
    )
    def test_load_failed_when_value_contains_invalid_chars(self, label_value, invalid_char):
        _, errors = self.Schema().load({'labels': {'env': label_value}})
        assert errors == {'labels': ['Symbol \'%s\' not allowed in value.' % invalid_char]}

    def test_load_failed_for_key_witch_start_with_number(self):
        _, errors = self.Schema().load({'labels': {'1a': 'foo'}})
        assert errors == {'labels': ['Keys must start with a lowercase letter.']}

    def test_load_failed_for_too_many_labels(self):
        _, errors = self.Schema().load({'labels': {'lbl-%d' % i: str(i) for i in range(100)}})
        assert errors == {'labels': ['Length must be between 0 and 64.']}

    @pytest.mark.parametrize(
        'label_key',
        [
            'foo.',
            'f@b',
            'f/o/o',
            r'b-\b/-b',
            r'a\\',
            r'x_//-\\_x',
            r'v_v',
            'y.-',
        ],
    )
    def test_new_valid_chars_in_name(self, label_key):
        """
        Since CLOUD-30362 we should allow ./\\@ in labels
        """
        _, errors = self.Schema().load({'labels': {label_key: 'foo'}})
        assert not errors


def mk_flt_obj(attribute, value, filter_str, operator=Operator.equals):
    return FilterObj(
        attribute=attribute,
        operator=operator,
        value=value,
        filter_str=filter_str,
    )


class TestFilter:
    class Schema(marshmallow.Schema):
        filter = FilterField()

    @pytest.mark.parametrize(
        ['flt_str', 'attribute', 'value'],
        [
            ('name="blah"', 'name', 'blah'),
            ('id=42', 'id', 42),
            ('ts=2021-01-01', 'ts', dt.date(2021, 1, 1)),
            (
                'ts=2021-01-01T01:01',
                'ts',
                dt.datetime(2021, 1, 1, 1, 1),
            ),
        ],
    )
    def test_load_filter_with_one_field(self, flt_str, attribute, value):
        data, _ = self.Schema().load({'filter': flt_str})
        assert data == {'filters': [mk_flt_obj(attribute, value, flt_str)]}

    def test_load_filter_with_list_value(self):
        data, _ = self.Schema().load({'filter': 'id IN (1, 2, 3)'})
        assert data == {'filters': [mk_flt_obj('id', [1, 2, 3], 'id IN (1, 2, 3)', Operator.in_)]}

    def test_load_filter_with_many_fields(self):
        data, _ = self.Schema().load(
            {
                'filter': 'name="blah" AND id=42 AND ts=2021-01-01',
            }
        )
        assert data == {
            'filters': [
                mk_flt_obj('name', 'blah', 'name="blah"'),
                mk_flt_obj('id', 42, 'id=42'),
                mk_flt_obj('ts', dt.date(2021, 1, 1), 'ts=2021-01-01'),
            ],
        }

    def test_failed_for_invalid_filter_syntax(self):
        _, error = self.Schema().load({'filter': 'id > 1 OR ts > 10'})
        assert 'Filter syntax error' in error['filter'][0]

    def test_failed_for_too_long_filter_size(self):
        _, error = self.Schema().load({'filter': 'id = 1' * 1000})
        assert error == {
            'filter': ['Length must be between 1 and 1000.'],
        }


class TestXmlEscapedStr:
    class Schema(marshmallow.Schema):
        str = XmlEscapedStr()

    def test_load_succeeded(self):
        data, _errors = self.Schema().load({'str': 'str with < and > symbols'})
        assert data == {'str': 'str with &lt; and &gt; symbols'}

    def test_load_failed(self):
        _data, errors = self.Schema().load({'str': 10})
        assert len(errors) == 1

    def test_dump_str_succeeded(self):
        data, _errors = self.Schema().dump({'str': 'str with &lt; and &gt; symbols'})
        assert data == {'str': 'str with < and > symbols'}

    def test_dump_int_succeeded(self):
        data, _errors = self.Schema().dump({'str': 10})
        assert data == {'str': '10'}


class TestGrpcInt:
    class Schema(marshmallow.Schema):
        value = GrpcInt()
        value_missing = GrpcInt(missing=42)

    def test_load_non_empty_value_succeeded(self):
        data, _errors = self.Schema().load({'value': 10, 'value_missing': 20})
        assert data == {'value': 10, 'value_missing': 20}

    @pytest.mark.parametrize(
        ['input'],
        [
            ({},),
            ({'value': 0, 'value_missing': 0},),
            ({'value': '0', 'value_missing': '0'},),
        ],
    )
    def test_load_empty_value_succeeded(self, input):
        data, errors = self.Schema().load(input)
        assert not errors
        assert data == {'value_missing': 42}


class TestGrpcStr:
    class Schema(marshmallow.Schema):
        value = GrpcStr()

    @pytest.mark.parametrize(
        ['input', 'output'],
        [({}, {}), ({'value': ''}, {'value': None}), ({'value': '123'}, {'value': '123'})],
    )
    def test_load_empty_value_succeeded(self, input, output):
        data, errors = self.Schema().load(input)
        assert not errors
        assert data == output


class TestBooleanMappedToInt:
    class Schema(marshmallow.Schema):
        value = BooleanMappedToInt(allow_none=True)

    def test_load_empty(self):
        data, errors = self.Schema().load({})
        assert data == {}
        assert not errors

    def test_dump_empty(self):
        data, errors = self.Schema().dump({})
        assert data == {}
        assert not errors

    @pytest.mark.parametrize(
        ['input', 'output'],
        [
            (False, 0),
            (True, 1),
        ],
    )
    def test_load_non_empty(self, input, output):
        data, _errors = self.Schema().load({'value': input})
        assert data == {'value': output}
        assert type(data['value']) == int

    @pytest.mark.parametrize(
        ['input', 'output'],
        [
            (0, False),
            (1, True),
        ],
    )
    def test_dump_non_empty(self, input, output):
        data, _errors = self.Schema().dump({'value': input})
        assert data == {'value': output}
        assert type(data['value']) == bool

    def test_load_none(self):
        data, errors = self.Schema().load({'value': None})
        assert data == {'value': None}

    def test_dump_none(self):
        data, errors = self.Schema().dump({'value': None})
        assert data == {'value': None}
