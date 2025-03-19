#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb
import mock_mongodb


def check_some_state(initial_data, pillar, expected, state, *state_args, **state_kwargs):
    '''Test ensure users on some data'''
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set(pillar)
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = getattr(s_mongodb, state)(*state_args, **state_kwargs)

    assert ret['result'] is True, 'ret[result] is not true: {}'.format(ret)
    mocked_salt.assert_data_is_equal(expected)


def check_some_state_test_true_make_no_changes(initial_data, pillar, expected, state, *state_args, **state_kwargs):
    '''Test that with test=True, ensure users won't modify data'''
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    old_expected = mocked_salt.get_data_copy()

    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set(pillar)
    s_mongodb.__opts__ = {'test': True}

    # Check that with test=True we won't make changes
    #  and they comment\result\changes sections looks fine
    need_changes = not s_mongodb._compare_structs(old_expected, expected)
    ret = getattr(s_mongodb, state)(*state_args, **state_kwargs)

    if need_changes:
        assert ret['result'] is None
        assert ret['comment'] != ''
        assert ret['changes'] != {}
    else:
        assert ret['result'] is True
        assert ret['comment'] == ''
        assert ret['changes'] == {}
    mocked_salt.assert_data_is_equal(old_expected)


def check_some_state_no_more_pending_changes(initial_data, pillar, expected, state, *state_args, **state_kwargs):
    '''Test that after performing changes we won't have any pending changes anymore'''
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)

    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set(pillar)
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = getattr(s_mongodb, state)(*state_args, **state_kwargs)

    # Check that with test=True we have no pendiong changes anymore
    s_mongodb.__opts__['test'] = True
    ret = getattr(s_mongodb, state)(*state_args, **state_kwargs)

    assert ret['result'] is True
    assert ret['comment'] == ''
    assert ret['changes'] == {}
