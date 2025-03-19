import pytest

from cloud.mdb.cli.dbaas.internal.common import expand_key_pattern, update_key, KeyException


@pytest.mark.parametrize(
    ids=lambda data: data['id'],
    argnames='data',
    argvalues=[
        {
            'id': 'key exists',
            'object': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                },
            },
            'key': 'a:b1',
            'value': 10,
            'result': {
                'a': {
                    'b1': 10,
                    'b2': 2,
                },
            },
        },
        {
            'id': 'key is missing in object',
            'object': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                },
            },
            'key': 'a:b3',
            'value': 10,
            'result': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                    'b3': 10,
                },
            },
        },
        {
            'id': 'key is missing in object, none in the path',
            'object': {
                'a': {
                    'b': None,
                },
            },
            'key': 'a:b:c',
            'value': 10,
            'result': {
                'a': {
                    'b': {
                        'c': 10,
                    },
                },
            },
        },
        {
            'id': 'multiple key elements are missing in object',
            'object': {
                'a': {},
            },
            'key': 'a:b:c',
            'value': 10,
            'result': {
                'a': {
                    'b': {
                        'c': 10,
                    },
                },
            },
        },
        {
            'id': 'key with array index',
            'object': {
                'a': [
                    {
                        'b1': 1,
                        'b2': 2,
                    }
                ],
            },
            'key': 'a:0:b1',
            'value': 10,
            'result': {
                'a': [
                    {
                        'b1': 10,
                        'b2': 2,
                    }
                ],
            },
        },
        {
            'id': 'key with array index of inserting element',
            'object': {
                'a': [
                    {
                        'b1': 1,
                    }
                ],
            },
            'key': 'a:1',
            'value': {
                'b2': 10,
            },
            'result': {
                'a': [
                    {
                        'b1': 1,
                    },
                    {
                        'b2': 10,
                    },
                ],
            },
        },
    ],
)
def test_update_key(data):
    object = data['object']
    update_key(object, data['key'], data['value'])
    assert object == data['result']


@pytest.mark.parametrize(
    ids=lambda data: data['id'],
    argnames='data',
    argvalues=[
        {
            'id': 'incorrect value of array index in key',
            'object': {
                'a': [
                    {
                        'b': 1,
                    }
                ],
            },
            'key': 'a:b',
            'value': 10,
        },
        {
            'id': 'invalid array index in key for leaf element (index > array length)',
            'object': {
                'a': [],
            },
            'key': 'a:1',
            'value': {
                'b': 10,
            },
        },
        {
            'id': 'invalid array index in key for non-leaf element (index >= array length)',
            'object': {
                'a': [],
            },
            'key': 'a:0:b',
            'value': 10,
        },
    ],
)
def test_update_key_failed(data):
    with pytest.raises(KeyException):
        update_key(data['object'], data['key'], data['value'])


@pytest.mark.parametrize(
    ids=lambda data: data['id'],
    argnames='data',
    argvalues=[
        {
            'id': 'existing key',
            'object': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                },
            },
            'key': 'a:b1',
            'result': [
                'a:b1',
            ],
        },
        {
            'id': 'non-existing key',
            'object': {
                'a': {},
            },
            'key': 'a:b1',
            'result': [
                'a:b1',
            ],
        },
        {
            'id': 'key pattern with dict globing',
            'object': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                },
            },
            'key': 'a:*',
            'result': [
                'a:b1',
                'a:b2',
            ],
        },
        {
            'id': 'key pattern with dict globing 2',
            'object': {
                'a': {
                    'b1': {'c': 1},
                    'b2': {'c': 2},
                },
            },
            'key': 'a:*:c',
            'result': [
                'a:b1:c',
                'a:b2:c',
            ],
        },
        {
            'id': 'key pattern with list globing',
            'object': {
                'a': [
                    'b1',
                    'b2',
                ],
            },
            'key': 'a:*',
            'result': [
                'a:0',
                'a:1',
            ],
        },
        {
            'id': 'key pattern with scalar globing',
            'object': {
                'a': {
                    'b1': 1,
                    'b2': 2,
                },
            },
            'key': 'a:b1:*',
            'result': [],
        },
        {
            'id': 'none in the middle',
            'object': {
                'a': {
                    'b': None,
                },
            },
            'key': 'a:b:c',
            'result': [
                'a:b:c',
            ],
        },
    ],
)
def test_expand_key_pattern(data):
    assert expand_key_pattern(data['object'], data['key']) == data['result']
