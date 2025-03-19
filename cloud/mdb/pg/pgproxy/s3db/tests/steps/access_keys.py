# coding: utf-8

from behave import then, when
import yaml

import logging
log = logging.getLogger('access_keys')


@then(u'we have the following access keys')
def step_list_access_keys(context):
    access_keys = context.connect.get_func('v1_code.list_access_keys')
    if access_keys.errcode is not None:
        raise RuntimeError('failed to list access keys: {}'.format(access_keys.errcode))

    def access_key_id(access_key):
        return access_key['key_id']

    actual_access_keys = sorted(
        (
            dict((heading, str(record[heading])) for heading in context.table.headings)
            for record in access_keys.records
        ),
        key=access_key_id,
    )
    expected_access_keys = sorted(
        (
            dict((heading, str(row[heading])) for heading in context.table.headings)
            for row in context.table.rows
        ),
        key=access_key_id,
    )

    assert actual_access_keys == expected_access_keys, (
        'Actual access keys list differs from the expected one: actual="{}", expected="{}"'.format(
            actual_access_keys, expected_access_keys))


def add_access_key(context):
    args = yaml.safe_load(context.text)

    context.access_key_result = context.connect.get_func(
        'v1_code.add_access_key',
        i_service_id=int(args['service_id']),
        i_user_id=int(args['user_id']),
        i_role=str(args['role']),
        i_key_id=str(args['key_id']),
        i_secret_token=str(args.get('secret_token', 'SECRET')),
        i_key_version=int(args.get('key_version', 1)),
    )


@when(u'we add access key')
def step_add_access_key(context):
    add_access_key(context)


@then(u'the access key request fails with "{pgcode}" error')
def step_check_access_key_request(context, pgcode):
    if not hasattr(context, 'access_key_result'):
        raise RuntimeError('there is no access key request to check')

    log.debug(context.access_key_result)
    assert pgcode == context.access_key_result.errcode, (
        'access key request failed with different error code')


@when(u'we successfully added access key')
def step_successfully_add_access_key(context):
    add_access_key(context)

    log.debug(context.access_key_result)
    assert context.access_key_result.errcode is None, 'add access key request failed'


def delete_access_key(context):
    args = yaml.safe_load(context.text)

    context.access_key_result = context.connect.get_func(
        'v1_code.delete_access_key',
        i_service_id=int(args['service_id']),
        i_user_id=int(args['user_id']),
        i_role=str(args['role']),
        i_key_id=str(args['key_id']),
    )


@when(u'we delete access key')
def step_delete_access_key(context):
    delete_access_key(context)


@when(u'we successfully deleted access key')
def step_successfully_deleted_access_key(context):
    delete_access_key(context)

    log.debug(context.access_key_result)
    assert context.access_key_result.errcode is None, 'delete access key request failed'
