import json

from unittest.mock import patch
import io
import pytest
from log_parser import LogParser


@pytest.mark.parametrize(
    ['data', 'expected_lines'], [['{"a": 1, "b": 2}', 1], ['{"a": 1, "b": 2}\n{"c": 3, "d": 4}', 2]]
)
def test_read_chunks_with_chunk_by_line(data: str, expected_lines: int):  # multiline chunks tests in other parsers
    parser = LogParser()
    parser.initialize([])
    chunks = parser.read_chunks(stream=io.StringIO(data))
    result = list(chunks)

    assert len(result) == expected_lines
    assert '\n'.join(result) == data


@pytest.mark.parametrize(
    ['data', 'expected'],
    [
        ['', None],
        ['{"a": 1, "b": 2}', None],
        ['x;y;z;{}', {'datetime': None, 'message': '{}', LogParser.PIPE_PREFIX: 'x;y;z;'}],
        [
            'x;y;z;{"datetime": 1, "b": 2}',
            {LogParser.PIPE_PREFIX: 'x;y;z;', 'datetime': 1, 'message': '{"datetime": 1, "b": 2}'},
        ],
    ],
)
def test_parse_record_from_json(data, expected):
    parser = LogParser()
    parser.initialize(['--input-format', 'json'])

    result = parser.parse_record(data)

    assert result == expected


def test_parse_record_raise_exeptions_on_invalid_argument_or_state():

    # test None input
    with pytest.raises(AttributeError):
        parser = LogParser()
        parser.initialize(['--input-format', 'json'])
        parser.parse_record(None)

    # test undefined input format
    with pytest.raises(ValueError) as e:
        parser = LogParser()
        parser.parse_record('x;y;z;{"a": 1, "b": 2}')
    assert str(e.value) == 'You should override parse_record method or set specific parser'

    # test invalid json
    with pytest.raises(json.decoder.JSONDecodeError):
        parser = LogParser()
        parser.initialize(['--input-format', 'json'])
        parser.parse_record('x;y;z;a=1\tb=2')

    # test empty line
    with pytest.raises(json.decoder.JSONDecodeError):
        parser = LogParser()
        parser.initialize(['--input-format', 'json'])
        parser.parse_record('x;y;z;')

    # test multiline input
    with pytest.raises(json.decoder.JSONDecodeError):
        parser = LogParser()
        parser.initialize(['--input-format', 'json'])
        parser.parse_record('x;y;z;{"a": 1, "b": 2}\na;b;c;{"a": 1, "b": 2}')


@pytest.mark.parametrize(
    ['record', 'expected'], [[{"a": 1, "b": 2, LogParser.PIPE_PREFIX: 'x;y;z;'}, 'x;y;z;{"a": 1, "b": 2}']]
)
def test_print_record_as_json(record, expected):
    parser = LogParser()
    parser.initialize(['--output-format', 'json'])

    with patch('sys.stdout', new_callable=io.StringIO) as mock_stdout:
        parser.print_record(record)
    result = mock_stdout.getvalue()

    assert result == expected + '\n'


@pytest.mark.parametrize(
    ['record', 'expected'], [[{"a": 1, "b": 2, LogParser.PIPE_PREFIX: 'x;y;z;'}, 'x;y;z;tskv\ta=1\tb=2']]
)
def test_print_record_as_tskv(record, expected):
    parser = LogParser()
    parser.initialize(['--output-format', 'tskv'])

    with patch('sys.stdout', new_callable=io.StringIO) as mock_stdout:
        parser.print_record(record)
    result = mock_stdout.getvalue()

    assert result == expected + '\n'


@pytest.mark.parametrize(
    ['record', 'exception', 'message'],
    [
        [None, AttributeError, "'NoneType' object has no attribute 'pop'"],
        [{}, KeyError, "'" + LogParser.PIPE_PREFIX + "'"],
        [{"a": 1, "b": 2}, KeyError, "'" + LogParser.PIPE_PREFIX + "'"],
    ],
)
def test_print_record_raise_exceptions_on_invalid_input(record: dict, exception: Exception, message: str):
    with pytest.raises(exception) as e:
        parser = LogParser()
        parser.initialize(['--output-format', 'json'])
        parser.print_record(record)
    assert str(e.value) == message


@pytest.mark.parametrize(
    ['dt', 'expected'],
    [
        [None, None],
        ["2022-03-11T00:01:07.041+03:00", 1646946067.041],
    ],
)
def test_parse_timestamp(dt, expected):
    parser = LogParser()
    parser.initialize([])  # to set default dt_key

    result = parser.parse_timestamp(dt)

    if result is not None:
        assert abs(result - expected) < 10**-4
    else:
        assert result == expected


@pytest.mark.parametrize(
    ['record', 'expected'],
    [
        [
            {"a": 1, "b": 2, LogParser.REQUIRED_DT_KEY: "2022-03-11T00:01:07.041+03:00"},
            {
                'a': 1,
                'b': 2,
                LogParser.REQUIRED_DT_KEY: '2022-03-11T00:01:07.041+03:00',
                'timestamp': 1646946067,
                'ms': '40',
            },
        ],
    ],
)
def test_add_time_data(record, expected):
    parser = LogParser()

    result = parser.add_time_data(record)

    assert result is None
    assert record == expected


@pytest.mark.parametrize(
    ['record', 'exception', 'message'],
    [
        [None, TypeError, "'NoneType' object is not subscriptable"],
        [
            {'datetime': None},
            TypeError,
            "int() argument must be a string, a bytes-like object or a real number, not 'NoneType'",
        ],
    ],
)
def test_add_time_data_raise_exception(record: dict, exception: Exception, message: str):
    with pytest.raises(exception) as e:
        parser = LogParser()
        parser.initialize([])
        parser.add_time_data(record)
    assert str(e.value) == message


@pytest.mark.parametrize(
    ['chunk', 'expected'],
    [
        ['', None],
        ['{"a": 1, "b": 2}', None],
        ['x;y;z;{}', None],  # empty record parsed to None
        [
            'x;y;z;{"a": 1, "b": 2, "datetime": "2022-03-11T00:01:07.041+03:00"}',
            {
                LogParser.PIPE_PREFIX: 'x;y;z;',
                'message': '{"a": 1, "b": 2, "datetime": "2022-03-11T00:01:07.041+03:00"}',
                'datetime': '2022-03-11T00:01:07.041+03:00',
                'timestamp': 1646946067,
                'ms': '40',
                'cluster': '',
                'origin': None,
                'log_format': 'dbaas_int_log',
            },
        ],
    ],
)
def test_parse_chunk_with_defaults(chunk, expected):
    parser = LogParser()
    parser.initialize(['--input-format', 'json'])

    result = parser.parse_chunk(chunk)

    if result:
        result_hostname = result.pop('hostname')
        assert result_hostname is not None
    assert result == expected
