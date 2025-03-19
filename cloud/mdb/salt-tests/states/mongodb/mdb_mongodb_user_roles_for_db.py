#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb
import cloud.mdb.salt.salt._states.mongodb as s_pure_mongodb

DEFAULT_DB = 'db1'
FORMAT_TEST_DATA = [
    (('db1', ['read', 'readWrite']), [{'db': 'db1', 'role': 'read'}, {'db': 'db1', 'role': 'readWrite'}]),
    (('db1', []), []),
]


@pytest.mark.parametrize('input_data, expected', FORMAT_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_format_roles(input_data, expected):
    database = input_data[0]
    roles_list = input_data[1]
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(database, roles_list)

    format_roles = roles.format_roles()
    assert s_pure_mongodb._compare_structs(format_roles, expected) is True


REPR_TEST_DATA = [
    ([], []),
    (['r', 'root', 'w'], ['r', 'root', 'w']),
    (['w', 'root', 'r'], ['r', 'root', 'w']),
    (['r', 'r', 'r', 'w'], ['r', 'w']),
]


@pytest.mark.parametrize('data, expected', REPR_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_get_changes_repr(data, expected):
    assert s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data).get_changes_repr() == expected


@pytest.mark.parametrize('data, expected', REPR_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_equality(data, expected):
    assert s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data) == s_mongodb.MDBMongoDBUserRolesForSingleDB(
        DEFAULT_DB, expected
    )


NE_TEST_DATA = [
    ([], ['r']),
    (['r', 'root', 'w'], ['root', 'r']),
    (['r', 'r', 'r', 'w'], ['r']),
]


@pytest.mark.parametrize('data, expected', NE_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_unequality(data, expected):
    assert s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data) != s_mongodb.MDBMongoDBUserRolesForSingleDB(
        DEFAULT_DB, expected
    )


@pytest.mark.parametrize('data, expected', NE_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_equality_False(data, expected):
    assert (
        s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
        == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)
    ) is False


@pytest.mark.parametrize('data, expected', REPR_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_unequality_False(data, expected):
    assert (
        s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
        != s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)
    ) is False


ADD_TEST_DATA = [
    ([], [], []),
    (['r', 'root', 'w'], ['r', 'w', 'root'], ['w', 'root', 'r']),
    (['r', 'r', 'r', 'w'], ['w', 'root'], ['w', 'r', 'root']),
    (['r', 'w'], [], ['r', 'w']),
    ([], ['r', 'w'], ['r', 'w']),
    (['r', 'w'], ['r', 'r', 'r', 'r'], ['r', 'w']),
    (['w'], ['r', 'r', 'r', 'r'], ['r', 'w']),
]


@pytest.mark.parametrize('data, add, expected', ADD_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_iadd_list(data, add, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    roles += add

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


@pytest.mark.parametrize('data, add, expected', ADD_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_iadd_str(data, add, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    for role in add:
        roles += role

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


@pytest.mark.parametrize('data, add, expected', ADD_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_iadd_MDBMongoDBUserRolesForSingleDB(data, add, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    roles += s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, add)

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


SUB_TEST_DATA = [
    ([], [], []),
    (['r', 'w'], [], ['r', 'w']),
    (['r', 'w'], ['root', 'admin'], ['r', 'w']),
    (['r', 'w'], ['r', 'admin'], ['w']),
    ([], ['r', 'w'], []),
    (['r', 'w'], ['r', 'w'], []),
    (['r'], ['r', 'w'], []),
]


@pytest.mark.parametrize('data, sub, expected', SUB_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_isub_list(data, sub, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    roles -= sub

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


@pytest.mark.parametrize('data, sub, expected', SUB_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_isub_str(data, sub, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    for role in sub:
        roles -= role

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


@pytest.mark.parametrize('data, sub, expected', SUB_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_isub_MDBMongoDBUserRolesForSingleDB(data, sub, expected):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)

    roles -= s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, sub)

    assert roles == s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, expected)


IN_TEST_DATA = [
    (['w', 'r', 'root'], 'w'),
    (['w', 'r', 'root'], 'r'),
    (['w', 'r', 'root'], 'root'),
]


@pytest.mark.parametrize('data, elem', IN_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_in(data, elem):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
    assert elem in roles


NOT_IN_TEST_DATA = [
    (['w', 'r', 'root'], 'admin'),
    (['r', 'root'], 'w'),
    (['w', 'root'], 'r'),
    (['w', 'r'], 'root'),
]


@pytest.mark.parametrize('data, elem', NOT_IN_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_not_in(data, elem):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
    assert elem not in roles


@pytest.mark.parametrize('data, elem', NOT_IN_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_in_False(data, elem):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
    assert (elem in roles) is False


@pytest.mark.parametrize('data, elem', IN_TEST_DATA)
def test_MDBMongoDBUserRolesForSingleDB_not_in_False(data, elem):
    roles = s_mongodb.MDBMongoDBUserRolesForSingleDB(DEFAULT_DB, data)
    assert (elem not in roles) is False
