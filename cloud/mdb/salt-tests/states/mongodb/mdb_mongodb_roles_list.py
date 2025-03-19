#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb


PILLAR_TEST_DATA = [
    (
        {
            'mdbAdmin': {
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
        },
        {
            'mdbAdmin.admin': {
                'database': 'admin',
                'name': 'mdbAdmin',
                'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
                'roles': {},
            }
        },
    ),
]


@pytest.mark.parametrize('pillar, expected', PILLAR_TEST_DATA)
def test_MDBMongoRolesList_read_pillar(pillar, expected):
    roles = s_mongodb.MDBMongoPillarReader.read_roles(pillar)

    roles_repr = roles.get_changes_repr()
    assert roles_repr == expected


MONGODB_TEST_DATA = [
    (
        [
            {
                "_id": 'admin.mdbAdmin',
                'db': 'admin',
                'role': 'mdbAdmin',
                'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
                'roles': [],
            },
        ],
        {
            'mdbAdmin.admin': {
                'database': 'admin',
                'name': 'mdbAdmin',
                'privileges': [{'actions': ['find'], 'resource': {'collection': 'system.profile', 'db': ''}}],
                'roles': {},
            }
        },
    ),
]


@pytest.mark.parametrize('data, expected', MONGODB_TEST_DATA)
def test_MDBMongoRolesList_read_from_mongo(data, expected):
    roles = s_mongodb.MDBMongoMongodbReader.read_roles(data)

    roles_repr = roles.get_changes_repr()
    assert roles_repr == expected
