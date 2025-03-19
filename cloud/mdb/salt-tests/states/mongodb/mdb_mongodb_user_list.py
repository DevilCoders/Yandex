#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb


PILLAR_TEST_DATA = [
    (
        {
            'admin': {
                'dbs': {
                    'admin': ['root', 'dbOwner'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                },
                'internal': True,
                'password': 'PASSWORD',
                'services': ['mongod', 'mongos', 'mongocfg'],
            },
            'user1': {
                'dbs': {
                    'admin': ['mdbMonitor'],
                    'db1': ['readWrite'],
                },
                'internal': False,
                'password': 'PASSWORD1',
                'services': ['mongod', 'mongos'],
            },
        },
        {
            'admin': {
                'password': '***',
                'roles': {
                    'admin': ['dbOwner', 'root'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                },
            },
            'user1': {
                'password': '***',
                'roles': {
                    'admin': ['mdbMonitor'],
                    'db1': ['readWrite'],
                },
            },
        },
    ),
]


@pytest.mark.parametrize('pillar, expected', PILLAR_TEST_DATA)
def test_MDBMongoUsersList_read_pillar(pillar, expected):
    users = s_mongodb.MDBMongoPillarReader.read_users(pillar)

    users_repr = users.get_changes_repr()
    assert users_repr == expected


MONGODB_TEST_DATA = [
    (
        [
            {
                "_id": "admin.admin",
                "user": "admin",
                "db": "admin",
                "credentials": {},
                "roles": [
                    {"role": "dbOwner", "db": "admin"},
                    {"role": "root", "db": "admin"},
                    {"role": "dbOwner", "db": "config"},
                    {"role": "dbOwner", "db": "local"},
                ],
            },
            {
                "_id": "user1.admin",
                "user": "user1",
                "db": "admin",
                "credentials": {},
                "roles": [
                    {"role": "mdbMonitor", "db": "admin"},
                ],
            },
            {
                "_id": "user1.db1",
                "user": "user1",
                "db": "db1",
                "credentials": {},
                "roles": [
                    {"role": "readWrite", "db": "db1"},
                ],
            },
        ],
        {
            'admin': {
                'password': '***',
                'roles': {
                    'admin': ['dbOwner', 'root'],
                    'config': ['dbOwner'],
                    'local': ['dbOwner'],
                },
            },
            'user1': {
                'password': '***',
                'roles': {
                    'admin': ['mdbMonitor'],
                    'db1': ['readWrite'],
                },
            },
        },
    ),
]


@pytest.mark.parametrize('data, expected', MONGODB_TEST_DATA)
def test_MDBMongoUsersList_read_from_mongo(data, expected):
    users = s_mongodb.MDBMongoMongodbReader.read_users(data)

    users_repr = users.get_changes_repr()
    assert users_repr == expected
