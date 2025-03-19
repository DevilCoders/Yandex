#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals

# import pytest
import sys
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb
import mock_mongodb


def test_ensure_users_as_in_infra_mongodb_4_0_single():
    # Copy of infrastructure test
    mocked_salt = mock_mongodb.MongoSaltMock({'users': {}})
    __salt__ = mocked_salt
    s_mongodb.__salt__ = mocked_salt
    s_mongodb.__opts__ = {'test': False}

    pillar = {
        'admin': {
            'password': 'very_difficult_password_for_admin',
            'internal': True,
            'dbs': {'admin': ['root', 'dbOwner', 'mdbMonitor'], 'local': ['dbOwner'], 'config': ['dbOwner']},
            'services': ['mongod', 'mongocfg', 'mongos'],
        },
        'monitor': {
            'password': 'very_difficult_password_for_monitor',
            'internal': True,
            'dbs': {
                'admin': ['mdbMonitor'],
            },
            'services': ['mongod', 'mongos', 'mongocfg'],
        },
        'test_user': {
            'password': 'mysupercooltestpassword11111',
            'internal': False,
            'dbs': {
                'testdb1': ['read'],
            },
            'services': ['mongod', 'mongos'],
        },
        'another_test_user': {
            'password': 'mysupercooltestpassword11111',
            'internal': False,
            'dbs': {'testdb1': ['readWrite'], 'testdb2': ['readWrite']},
            'services': ['mongod', 'mongos'],
        },
        'and_yet_another_test_user': {
            'password': 'mysupercooltestpassword11111',
            'internal': False,
            'dbs': {},
            'services': ['mongod', 'mongos'],
        },
    }
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(
        name='mongodb',
        service='mongod',
    )
    assert ret['result'] is True

    pillar['petya'] = {
        'internal': False,
        'password': 'password-password-password-password',
        'dbs': {'testdb4': ['read']},
        'services': ['mongod', 'mongos'],
    }
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    if ret['result'] is not True:
        print(mocked_salt.get_data_copy(), file=sys.stderr)
    assert ret['result'] is True

    assert __salt__['mongodb.user_auth'](user='petya', password='password-password-password', authdb='testdb4') == (
        False,
        None,
    )
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password-password-password-password', authdb='testdb4'
    ) == (True, None)

    pillar['test_user']['dbs']['testdb4'] = ['readWrite']
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    assert ret['result'] is True
    assert __salt__['mongodb.user_auth'](
        user='test_user', password='mysupercooltestpassword11111', authdb='testdb4'
    ) == (True, None)

    pillar['test_user']['dbs'].pop('testdb4', None)
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    assert ret['result'] is True
    assert __salt__['mongodb.user_auth'](
        user='test_user', password='mysupercooltestpassword11111', authdb='testdb4'
    ) == (False, None)
    assert __salt__['mongodb.user_auth'](
        user='test_user', password='mysupercooltestpassword11111', authdb='testdb1'
    ) == (True, None)

    pillar['petya']['password'] = 'password_changed-password_changed-password_changed'
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    assert ret['result'] is True
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password-password-password-password', authdb='testdb4'
    ) == (False, None)
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password_changed-password_changed-password_changed', authdb='testdb4'
    ) == (True, None)

    pillar['petya']['dbs'] = {'testdb1': ['readWrite']}
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    assert ret['result'] is True
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password_changed-password_changed-password_changed', authdb='testdb4'
    ) == (False, None)
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password_changed-password_changed-password_changed', authdb='testdb1'
    ) == (True, None)

    pillar.pop('petya', None)
    mocked_salt.pillar_set({'data': {'mongodb': {'users': pillar}}})
    ret = s_mongodb.ensure_users(name='mongodb', service='mongod')
    assert ret['result'] is True
    assert __salt__['mongodb.user_auth'](
        user='petya', password='password_changed-password_changed-password_changed', authdb='testdb1'
    ) == (False, None)
