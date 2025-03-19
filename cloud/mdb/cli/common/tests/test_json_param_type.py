from types import SimpleNamespace

import pytest

from click import ClickException
from cloud.mdb.cli.common.parameters import JsonParamType


@pytest.mark.parametrize(
    ids=lambda data: data['id'],
    argnames='data',
    argvalues=[
        {
            'id': 'integer value',
            'value': '10',
            'result': 10,
        },
        {
            'id': 'integer value with extra spaces',
            'value': '  10  ',
            'result': 10,
        },
        {
            'id': 'float value',
            'value': '10.5',
            'result': 10.5,
        },
        {
            'id': 'string value',
            'value': '"text"',
            'result': 'text',
        },
        {
            'id': 'map value',
            'value': '{"key": "value"}',
            'result': {'key': 'value'},
        },
        {
            'id': 'multiline map value',
            'value': '''
                {
                    "key": "value"
                }
                ''',
            'result': {'key': 'value'},
        },
        {
            'id': 'list value',
            'value': '["item"]',
            'result': ['item'],
        },
        {
            'id': 'true value',
            'value': 'true',
            'result': True,
        },
        {
            'id': 'false value',
            'value': 'false',
            'result': False,
        },
        {
            'id': 'null value',
            'value': 'null',
            'result': None,
        },
        {
            'id': 'implicit string value',
            'value': 'text',
            'result': 'text',
        },
        {
            'id': 'implicit string value with leading digit',
            'value': '1d',
            'result': '1d',
        },
    ],
)
def test_convert(data):
    param = SimpleNamespace(name='test')
    ctx = SimpleNamespace()
    assert JsonParamType().convert(data['value'], param, ctx) == data['result']


@pytest.mark.parametrize(
    ids=lambda data: data['id'],
    argnames='data',
    argvalues=[
        {
            'id': 'invalid json',
            'value': '[1',
        },
    ],
)
def test_convert_failed(data):
    param = SimpleNamespace(name='test')
    ctx = SimpleNamespace()
    with pytest.raises(ClickException):
        assert JsonParamType().convert(data['value'], param, ctx)
