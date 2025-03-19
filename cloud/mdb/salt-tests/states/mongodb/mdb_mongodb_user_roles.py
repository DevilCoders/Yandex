#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

FORMAT_TEST_DATA = [
    (
        [('db1', ['read', 'readWrite'])],
        [
            {'db': 'db1', 'role': 'read'},
            {'db': 'db1', 'role': 'readWrite'},
        ],
    ),
    ([(['db1', []])], []),
    (
        [
            ('db1', ['read', 'readWrite']),
            ('db1', ['read', 'readWrite']),
            ('db2', ['read', 'read']),
            ('db3', ['read']),
            ('db3', ['write']),
        ],
        [
            {'db': 'db1', 'role': 'read'},
            {'db': 'db1', 'role': 'readWrite'},
            {'db': 'db2', 'role': 'read'},
            {'db': 'db3', 'role': 'read'},
            {'db': 'db3', 'role': 'write'},
        ],
    ),
]


@pytest.mark.parametrize('input_data,expected', FORMAT_TEST_DATA)
def test_MDBMongoDBUserRoles_format_roles(input_data, expected):
    user_roles = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in input_data:
        user_roles.add_roles(db, roles)

    roles_list = user_roles.format_roles()
    assert roles_list == expected


EQ_TEST_DATA = [
    ([('db1', ['read', 'readWrite'])], [('db1', ['read', 'readWrite'])]),
    ([('db1', ['read', 'readWrite'])], [('db1', ['readWrite', 'read'])]),
    ([('db1', ['read']), ('db1', ['readWrite'])], [('db1', ['read', 'readWrite'])]),
    (
        [
            ('db1', ['read', 'readWrite']),
            ('db1', ['read', 'readWrite']),
            ('db2', ['read', 'read']),
            ('db3', ['read']),
            ('db3', ['write']),
        ],
        [
            ('db1', ['read', 'readWrite']),
            ('db2', ['read']),
            ('db3', ['read', 'write']),
        ],
    ),
]


@pytest.mark.parametrize('data1, expected', EQ_TEST_DATA)
def test_MDBMongoDBUserRoles_eq(data1, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in data1:
        roles1.add_roles(db, roles)

    roles2 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in expected:
        roles2.add_roles(db, roles)

    assert roles1 == roles2


NE_TEST_DATA = [
    ([('db1', ['read', 'readWrite'])], [('db2', ['read', 'readWrite'])]),
    ([('db1', ['read'])], [('db1', ['readWrite', 'read'])]),
    ([('db1', ['read', 'readWrite'])], [('db1', ['readWrite'])]),
    ([('db1', ['read']), ('db1', ['readWrite'])], [('db1', ['readWrite'])]),
    (
        [
            ('db1', ['read', 'readWrite']),
            ('db2', ['read']),
            ('db3', ['read', 'write']),
        ],
        [
            ('db1', ['read', 'readWrite']),
            ('db3', ['read', 'write']),
        ],
    ),
]


@pytest.mark.parametrize('data1, expected', NE_TEST_DATA)
def test_MDBMongoDBUserRoles_ne(data1, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in data1:
        roles1.add_roles(db, roles)

    roles2 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in expected:
        roles2.add_roles(db, roles)

    assert roles1 != roles2


@pytest.mark.parametrize('data1, expected', NE_TEST_DATA)
def test_MDBMongoDBUserRoles_eq_False(data1, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in data1:
        roles1.add_roles(db, roles)

    roles2 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in expected:
        roles2.add_roles(db, roles)

    assert (roles1 == roles2) is False


@pytest.mark.parametrize('data1, expected', EQ_TEST_DATA)
def test_MDBMongoDBUserRoles_ne_False(data1, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in data1:
        roles1.add_roles(db, roles)

    roles2 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in expected:
        roles2.add_roles(db, roles)

    assert (roles1 != roles2) is False


PILLAR_TEST_DATA = [
    (
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
            'db1': [],
        },
        [
            ('admin', ['root', 'dbOwner']),
            ('config', ['dbOwner']),
            ('local', ['dbOwner']),
            ('db1', []),
        ],
    ),
]


@pytest.mark.parametrize('data, expected', PILLAR_TEST_DATA)
def test_MDBMongoDBUserRoles_read_pillar(data, expected):
    roles1 = s_mongodb.MDBMongoPillarReader.read_user_roles(data)

    roles2 = s_mongodb.MDBMongoDBUserRoles()
    for db, roles in expected:
        roles2.add_roles(db, roles)

    assert roles1 == roles2


ADD_TEST_DATA = [
    (
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        {
            'local': ['dbOwner'],
            'db1': [],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
            'db1': [],
        },
    ),
    (
        {},
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
    ),
    (
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        {},
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
    ),
    (
        {
            'admin': ['root'],
            'local': ['dbOwner'],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
    ),
    (
        {
            'admin': ['root'],
            'local': ['dbOwner'],
        },
        {
            'admin': ['dbOwner'],
            'config': ['dbOwner'],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
    ),
]


@pytest.mark.parametrize('data1, data2, expected', ADD_TEST_DATA)
def test_MDBMongoDBUserRoles_iadd(data1, data2, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles(data1)
    roles2 = s_mongodb.MDBMongoDBUserRoles(data2)
    roles_exp = s_mongodb.MDBMongoDBUserRoles(expected)

    roles1 += roles2

    assert roles1 == roles_exp


SUB_TEST_DATA = [
    (
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        {
            'local': ['dbOwner'],
            'db1': [],
        },
        ['admin', 'config'],
    ),
    (
        {},
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        [],
    ),
    (
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
            'local': ['dbOwner'],
        },
        {},
        ['admin', 'config', 'local'],
    ),
    (
        {
            'admin': ['root'],
            'local': ['dbOwner'],
        },
        {
            'admin': ['root', 'dbOwner'],
            'config': ['dbOwner'],
        },
        ['local'],
    ),
    (
        {
            'admin': ['root'],
            'local': ['dbOwner'],
        },
        {
            'config': ['dbOwner'],
        },
        ['admin', 'local'],
    ),
]


@pytest.mark.parametrize('data1, data2, expected', SUB_TEST_DATA)
def test_MDBMongoDBUserRoles_add(data1, data2, expected):
    roles1 = s_mongodb.MDBMongoDBUserRoles(data1)
    roles2 = s_mongodb.MDBMongoDBUserRoles(data2)
    dbs_exp = expected[:]
    dbs_exp.sort()
    res = roles1 - roles2
    res.sort()

    assert res == dbs_exp
