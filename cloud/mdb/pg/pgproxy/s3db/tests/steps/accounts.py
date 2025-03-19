# coding: utf-8

from behave import given, then, when

import logging
log = logging.getLogger('accounts')


@given(u'an empty accounts DB')
def step_empty_accounts_db(context):
    drop_result = context.connect.get_func('v1_impl.drop_accounts')
    if drop_result.errcode is not None:
        raise RuntimeError('failed to drop all accounts: {}'.format(drop_result.errcode))

    accounts_list = context.connect.get_func('v1_code.list_accounts')
    if accounts_list.errcode is not None:
        raise RuntimeError('failed to list all accounts: {}'.format(accounts_list.errcode))

    assert not accounts_list.records, 'Some accounts are presented after drop: {}'.format(
        accounts_list.records
    )



def step_setup_account_common(context, service_id, status, max_size=None, max_buckets=None):
    if status not in ('active', 'suspended'):
        raise ValueError('invalid account status: {}'.format(status))

    register_result = context.connect.get_func(
        'v1_code.register_account',
        i_service_id=service_id,
        i_max_size=max_size,
        i_max_buckets=max_buckets,
    )
    if register_result.errcode is not None:
        raise RuntimeError('failed to register account: {}'.format(register_result.errcode))

    if status == 'suspended':
        suspension_result = context.connect.get_func(
            'v1_code.suspend_account',
            i_service_id=service_id,
        )
        if suspension_result.errcode is not None:
            raise RuntimeError('failed to suspend account: {}'.format(suspension_result.errcode))


@given(u'the account "{service_id:d}" with status "{status}" and max_size "{max_size}" and max_buckets "{max_buckets}"')
def step_setup_account_with_max_size_and_max_buckets(context, service_id, status, max_size, max_buckets):
    step_setup_account_common(context, service_id, status, max_size, max_buckets)


@given(u'the account "{service_id:d}" with status "{status}" and max_size "{max_size}"')
def step_setup_account_with_max_size(context, service_id, status, max_size):
    step_setup_account_common(context, service_id, status, max_size)


@given(u'the account "{service_id:d}" with status "{status}"')
def step_setup_account(context, service_id, status):
    step_setup_account_common(context, service_id, status)


@given(u'we have the following accounts')
def set_setup_accounts(context):
    assert {'service_id', 'status', 'max_size', 'max_buckets'} == set(context.table.headings), (
        'Unexpected account settings: {}'.format(context.table.headings))

    for account in context.table.rows:
        if account['max_size'] and account['max_buckets']:
            context.execute_steps(u'''
                Given the account "{service_id}" with status "{status}" and max_size "{max_size}" and max_buckets "{max_buckets}"
            '''.format(
                service_id=account['service_id'],
                status=account['status'],
                max_size=account['max_size'],
                max_buckets=account['max_buckets'],
            ))
        elif account['max_size']:
            context.execute_steps(u'''
                Given the account "{service_id}" with status "{status}" and max_size "{max_size}"
            '''.format(
                service_id=account['service_id'],
                status=account['status'],
                max_size=account['max_size'],
            ))
        else:
            context.execute_steps(u'''
                Given the account "{service_id}" with status "{status}"
            '''.format(
                service_id=account['service_id'],
                status=account['status'],
            ))



def step_register_account_common(context, service_id, max_size=None, max_buckets=None):
    register_result = context.connect.get_func(
        'v1_code.register_account',
        i_service_id=service_id,
        i_max_size=max_size,
        i_max_buckets=max_buckets,
    )
    if register_result.errcode is not None:
        raise RuntimeError('failed to register account: {}'.format(register_result.errcode))


@when(u'we register the account "{service_id:d}" with max_size "{max_size}" and max_buckets "{max_buckets}"')
def step_register_account_with_max_size_and_max_buckets(context, service_id, max_size, max_buckets):
    step_register_account_common(context, service_id, max_size, max_buckets)


@when(u'we register the account "{service_id:d}" with max_size "{max_size}"')
def step_register_account_with_max_size(context, service_id, max_size):
    step_register_account_common(context, service_id, max_size)


@when(u'we register the account "{service_id:d}"')
def step_register_account(context, service_id):
    step_register_account_common(context, service_id)



@when(u'we suspend the account "{service_id:d}"')
def step_suspend_account(context, service_id):
    suspension_result = context.connect.get_func(
        'v1_code.suspend_account',
        i_service_id=service_id,
    )
    if suspension_result.errcode is not None:
        raise RuntimeError('failed to suspend account: {}'.format(suspension_result.errcode))


@when(u'we request info about the account "{service_id:d}"')
def step_request_account_info(context, service_id):
    context.account_info_request = context.connect.get_func(
        'v1_code.account_info',
        i_service_id=service_id,
    )


def step_validate_account_info(context, service_id, field_name, field_value):
    account_info = context.connect.get_func(
        'v1_code.account_info',
        i_service_id=service_id,
    )
    if account_info.errcode is not None:
        raise RuntimeError('failed to get account info: {}'.format(account_info.errcode))

    assert service_id == account_info.records[0]['service_id'], (
        'Unexpected account service_id: {}'.format(account_info.records[0]['service_id']))
    assert field_value == account_info.records[0][field_name], (
        'Unexpected account {}: {}'.format(field_name, account_info.records[0][field_name]))


@then(u'the account "{service_id:d}" has "{status}" status')
def step_validate_account_status(context, service_id, status):
    step_validate_account_info(context, service_id, 'status', status)


@then(u'the account "{service_id:d}" has "{folder_id}" folder_id')
def step_validate_account_folder_id(context, service_id, folder_id):
    step_validate_account_info(context, service_id, 'folder_id', folder_id)


@then(u'the account "{service_id:d}" has "{max_size:d}" max_size')
def step_validate_account_max_size(context, service_id, max_size):
    step_validate_account_info(context, service_id, 'max_size', max_size)


@then(u'the account "{service_id:d}" has "{max_buckets:d}" max_buckets')
def step_validate_account_max_buckets(context, service_id, max_buckets):
    step_validate_account_info(context, service_id, 'max_buckets', max_buckets)


@then(u'we have the following accounts')
def step_list_accounts(context):
    accounts = context.connect.get_func('v1_code.list_accounts')
    if accounts.errcode is not None:
        raise RuntimeError('failed to list accounts: {}'.format(accounts.errcode))

    def account_id(account):
        return account['service_id']

    actual_accounts = sorted(
        (
            dict((heading, str(record[heading])) for heading in context.table.headings)
            for record in accounts.records
        ),
        key=account_id,
    )
    expected_accounts = sorted(
        (
            dict((heading, row[heading]) for heading in context.table.headings)
            for row in context.table.rows
        ),
        key=account_id,
    )

    assert actual_accounts == expected_accounts, (
        'Actual accounts list differs from the expected one: actual="{}", expected="{}"'.format(
            actual_accounts, expected_accounts))


@then(u'the account request fails with error "{errcode}"')
def step_validate_account_info_request(context, errcode):
    assert errcode == context.account_info_request.errcode, (
        'Account info request returns with different errcode: expected="{}", actual="{}"'.format(
            errcode, context.account_info_request.errcode
        ))
