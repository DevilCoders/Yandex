#!/usr/bin/python
# coding: utf-8

import pytest
import cloud.mdb.salt.salt._states.mongodb as s_mongodb

__salt__ = {}


def make_bool_function(ret_value):
    def result(*args):
        return ret_value

    return result


ALIVE_TEST_DATA = [
    ([make_bool_function(True)], {'changes': {}, 'comment': '', 'name': 'mongodb', 'result': True}),
    (
        [make_bool_function(False)],
        {'changes': {}, 'comment': 'MongoDB at mongodb:None seems dead', 'name': 'mongodb', 'result': False},
    ),
]


@pytest.mark.parametrize('input_data,expected', ALIVE_TEST_DATA)
def test_is_alive(input_data, expected):
    global __salt__

    __salt__['mongodb.is_alive'] = input_data[0]

    s_mongodb.__salt__ = __salt__
    assert s_mongodb.alive('mongodb') == expected
