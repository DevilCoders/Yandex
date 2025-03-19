#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb


PILLAR_TEST_DATA = [
    (
        'mdbAdmin',
        {
            'database': 'admin',
            'privileges': [
                {
                    'resource': {
                        'db': '',
                        'collection': 'system.profile',
                    },
                    'actions': ['find'],
                },
            ],
        },
        {
            'database': 'admin',
            'name': 'mdbAdmin',
            'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
            'roles': {},
        },
    ),
]


@pytest.mark.parametrize('name, pillar, expected', PILLAR_TEST_DATA)
def test_MDBMongoDBRole_read_pillar(name, pillar, expected):
    role = s_mongodb.MDBMongoPillarReader.read_role(name, pillar)

    role_repr = role.get_changes_repr()
    assert role_repr == expected


MONGODB_TEST_DATA = [
    (
        {
            "_id": 'admin.mdbAdmin',
            'db': 'admin',
            'role': 'mdbAdmin',
            'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
            'roles': [],
        },
        {
            'database': 'admin',
            'name': 'mdbAdmin',
            'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
            'roles': {},
        },
    ),
]


@pytest.mark.parametrize('data, expected', MONGODB_TEST_DATA)
def test_MDBMongoDBRole_read_from_mongo(data, expected):
    role = s_mongodb.MDBMongoMongodbReader.read_role(None, data)

    role_repr = role.get_changes_repr()
    assert role_repr == expected


EQ_TEST_DATA = [
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1'],
            [],
        ],
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1'],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1'],
            [],
        ],
        [
            'role1',
            'db1',
            [
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
            ],
            ['db1', 'root'],
            [],
        ],
    ),
]


@pytest.mark.parametrize('data1, data2', EQ_TEST_DATA)
def test_MDBMongoDBRole_eq(data1, data2):
    role1 = s_mongodb.MDBMongoDBRole(*data1)
    role2 = s_mongodb.MDBMongoDBRole(*data2)

    assert role1 == role2


NE_TEST_DATA = [
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role2',
            'db1',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role1',
            'db2',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            ['1'],
            [],
            [],
        ],
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role1',
            'db1',
            ['2'],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            ['root'],
            [],
        ],
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role1',
            'db1',
            [],
            ['root'],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            [],
            ['some'],
        ],
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [],
            [],
            [],
        ],
        [
            'role1',
            'db1',
            [],
            [],
            ['some'],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": "test"}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1'],
            [],
        ],
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1'],
            [],
        ],
    ),
    (
        [
            'role1',
            'db1',
            [
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
            ],
            ['root', 'db1', 'db2'],
            [],
        ],
        [
            'role1',
            'db1',
            [
                {"resource": {"anyResource": True}, "actions": ["enableSharding", "flushRouterConfig", "getShardMap"]},
                {"resource": {"db": "admin", "collection": ""}, "actions": ["viewRole"]},
            ],
            ['root', 'db1'],
            [],
        ],
    ),
]


@pytest.mark.parametrize('data1, data2', NE_TEST_DATA)
def test_MDBMongoDBRole_ne(data1, data2):
    role1 = s_mongodb.MDBMongoDBRole(*data1)
    role2 = s_mongodb.MDBMongoDBRole(*data2)

    assert role1 != role2


@pytest.mark.parametrize('data1, data2', NE_TEST_DATA)
def test_MDBMongoDBRole_not_eq(data1, data2):
    role1 = s_mongodb.MDBMongoDBRole(*data1)
    role2 = s_mongodb.MDBMongoDBRole(*data2)

    assert (role1 == role2) is False


@pytest.mark.parametrize('data1, data2', EQ_TEST_DATA)
def test_MDBMongoDBRole_not_ne(data1, data2):
    role1 = s_mongodb.MDBMongoDBRole(*data1)
    role2 = s_mongodb.MDBMongoDBRole(*data2)

    assert (role1 != role2) is False
