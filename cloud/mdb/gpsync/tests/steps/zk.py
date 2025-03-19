#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import logging
import operator

import kazoo.exceptions
import cloud.mdb.gpsync.tests.steps.helpers as helpers
import yaml
from behave import then, when

LOG = logging.getLogger('zk')


@then('zookeeper "{name}" has holder "{holder}" for lock "{key}"')
@helpers.retry_on_assert
def step_zk_check_holder(context, name, holder, key):
    try:
        zk = helpers.get_zk(context, name)
        contender = None
        zk.start()
        lock = zk.Lock(key)
        contenders = lock.contenders()
        if contenders:
            contender = contenders[0]
    finally:
        zk.stop()
        zk.close()
    assert str(contender) == str(holder), 'lock "{key}" holder is "{holder}", expected "{exp}"'.format(
        key=key, holder=contender, exp=holder
    )


@when('we lock "{key}" in zookeeper "{name}"')
@when('we lock "{key}" in zookeeper "{name}" with value "{value}"')
def step_zk_lock(context, name, key, value=None):
    if not context.zk:
        context.zk = helpers.get_zk(context, name)
        context.zk.start()
    lock = context.zk.Lock(key, value)
    lock.acquire()
    context.zk_locks[key] = lock


@when('we release lock "{key}" in zookeeper "{name}"')
def step_zk_release_lock(context, name, key):
    # try:
    if key in context.zk_locks:
        context.zk_locks[key].release()
    # except Exception:
    #    pass


@then('zookeeper "{name}" has value "{value}" for key "{key}"')
@helpers.retry_on_assert
def step_zk_value(context, name, value, key):
    zk_value = helpers.get_zk_value(context, name, key)
    assert str(zk_value) == str(value), 'expected value "{exp}", got "{val}"'.format(exp=value, val=zk_value)


@then('zookeeper "{name}" has following values for key "{key}"')
@then('zookeeper "{name}" has following values for key "{key}" sorted by "{sort_by}"')
@helpers.retry_on_assert
def step_zk_key_values_sorted_by(context, name, key, sort_by='client_hostname'):
    exp_values = sorted(yaml.safe_load(context.text) or [], key=operator.itemgetter(sort_by))
    assert isinstance(exp_values, list), 'expected list, got {got}'.format(got=type(exp_values))
    zk_value = helpers.get_zk_value(context, name, key)
    assert zk_value is not None, 'key {key} does not exists'.format(key=key)

    LOG.info(f'zk_value = {zk_value}')
    zk_value_loaded = json.loads(zk_value)
    assert zk_value_loaded is not None, 'key {key} has null content'.format(key=key)
    actual_values = sorted(zk_value_loaded, key=operator.itemgetter(sort_by))

    equal, error = helpers.are_dicts_subsets_of(exp_values, actual_values)
    assert equal, error


def has_value_in_list(context, zk_name, key, value):
    zk_value = helpers.get_zk_value(context, zk_name, key)
    if zk_value is None or zk_value == "":
        return False

    zk_list = json.loads(zk_value)
    return value in zk_list


def has_subset_of_values(context, zk_name, key, exp_values):
    zk_value = helpers.get_zk_value(context, zk_name, key)
    if zk_value is None:
        return False

    zk_dicts = json.loads(zk_value)
    actual_values = {d['client_hostname']: d for d in zk_dicts}

    equal = helpers.is_2d_dict_subset_of(exp_values, actual_values)
    return equal


@when('we set value "{value}" for key "{key}" in zookeeper "{name}"')
def step_zk_set_value(context, name, key, value):
    try:
        zk = helpers.get_zk(context, name)
        zk.start()
        zk.ensure_path(key)
        # There is race condition, node can be deleted after ensure_path and
        # before set called. We need to catch exception and create it again.
        try:
            zk.set(key, value.encode())
        except kazoo.exceptions.NoNodeError:
            zk.create(key, value.encode())
    finally:
        zk.stop()
        zk.close()


@when('we remove key "{key}" in zookeeper "{name}"')
def step_zk_remove_key(context, name, key):
    try:
        zk = helpers.get_zk(context, name)
        zk.start()
        zk.delete(key, recursive=True)
    finally:
        zk.stop()
        zk.close()
