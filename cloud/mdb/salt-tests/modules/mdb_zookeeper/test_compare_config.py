# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import pytest

from cloud.mdb.salt.salt._modules import mdb_zookeeper


@pytest.mark.parametrize(
    ids=[
        'Config empty, all items added',
        'Single value changed',
        'Config already has some item, and new added',
        'Item removed from config',
        'Items changed, added and removed from config',
    ],
    argnames=['config_old', 'config_new', 'expected'],
    argvalues=[
        ({}, {'key1': 'value1'}, {'added': {'key1': 'value1'}}),
        ({'key1': 'value1'}, {'key1': 'value2'}, {'changed': {'key1': 'value2'}}),
        ({'key1': 'value1'}, {'key1': 'value1', 'key2': 'value2'}, {'added': {'key2': 'value2'}}),
        ({'key1': 'value1', 'key2': 'value2'}, {'key1': 'value1'}, {'removed': {'key2': 'value2'}}),
        (
            {
                'key1': 'value1',
                'key2': 'value2',
            },
            {'key1': 'value11', 'key3': 'value3'},
            {'changed': {'key1': 'value11'}, 'added': {'key3': 'value3'}, 'removed': {'key2': 'value2'}},
        ),
    ],
)
def test_compare_config(config_old, config_new, expected):
    assert mdb_zookeeper.compare_config(config_new, config_old) == expected
