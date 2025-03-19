#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

DEFAULT_USER_NAME = 'user1'

PILLAR_TEST_DATA = [
    (
        'user1',
        {
            'dbs': {
                'admin': ['root', 'dbOwner'],
                'config': ['dbOwner'],
                'local': ['dbOwner'],
                'db1': [],
            },
            'internal': True,
            'password': 'PASSWORD',
            'services': ['mongod', 'mongos', 'mongocfg'],
        },
        (
            "user1",
            "PASSWORD",
            {
                'password': '***',
                'roles': {
                    'admin': ['dbOwner', 'root'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                },
            },
        ),
    ),
]


@pytest.mark.parametrize('username, pillar, expected', PILLAR_TEST_DATA)
def test_MDBMongoUser_read_pillar(username, pillar, expected):
    user = s_mongodb.MDBMongoPillarReader.read_user(username, pillar)
    user_repr = user.get_changes_repr()
    assert user.username == expected[0]
    assert user.password == expected[1]
    assert user_repr == expected[2]


MONGODB_TEST_DATA = [
    (
        {
            "_id": "admin.admin",
            "user": "admin",
            "db": "admin",
            "credentials": {},
            "roles": [
                {"role": "dbOwner", "db": "config"},
            ],
        },
        (
            "admin",
            {
                'password': '***',
                'roles': {
                    'config': ['dbOwner'],
                    'admin': [],
                },
            },
        ),
    ),
]


@pytest.mark.parametrize('data, expected', MONGODB_TEST_DATA)
def test_MDBMongoUser_read_from_mongo(data, expected):
    user = s_mongodb.MDBMongoMongodbReader.read_user(data=data)
    user_repr = user.get_changes_repr()
    assert user.username == expected[0]
    assert user_repr == expected[1]


EQ_TEST_DATA = [
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
]


@pytest.mark.parametrize('data1, data2', EQ_TEST_DATA)
def test_MDBMongoUser_eq(data1, data2):
    user1 = s_mongodb.MDBMongoUser(*data1)
    user2 = s_mongodb.MDBMongoUser(*data2)

    assert user1 == user2


NE_TEST_DATA = [
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user2',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'DIFFERENT_PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
]


@pytest.mark.parametrize('data1, data2', NE_TEST_DATA)
def test_MDBMongoUser_ne(data1, data2):
    user1 = s_mongodb.MDBMongoUser(*data1)
    user2 = s_mongodb.MDBMongoUser(*data2)

    assert user1 != user2


@pytest.mark.parametrize('data1, data2', NE_TEST_DATA)
def test_MDBMongoUser_eq_False(data1, data2):
    user1 = s_mongodb.MDBMongoUser(*data1)
    user2 = s_mongodb.MDBMongoUser(*data2)

    assert (user1 == user2) is False


@pytest.mark.parametrize('data1, data2', EQ_TEST_DATA)
def test_MDBMongoUser_ne_False(data1, data2):
    user1 = s_mongodb.MDBMongoUser(*data1)
    user2 = s_mongodb.MDBMongoUser(*data2)

    assert (user1 != user2) is False


MERGE_TEST_DATA = [
    (
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root'],
                    'local': ['dbOwner'],
                    'db0': ['dbOwner'],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
        (
            'user1',
            'PASSWORD',
            True,
            False,
            s_mongodb.MDBMongoDBUserRoles(
                {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                    'db0': ['dbOwner'],
                    'db1': [],
                }
            ),
            ['mongod', 'mongos', 'mongocfg'],
        ),
    ),
]


@pytest.mark.parametrize('data1, data2, expected', MERGE_TEST_DATA)
def test_MDBMongoUser_merge(data1, data2, expected):
    user1 = s_mongodb.MDBMongoUser(*data1)
    user2 = s_mongodb.MDBMongoUser(*data2)
    user_exp = s_mongodb.MDBMongoUser(*expected)

    user1.merge(user2)

    assert user1 == user_exp
