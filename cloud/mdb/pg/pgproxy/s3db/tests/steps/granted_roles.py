# coding: utf-8

from behave import given, then, when

import logging
log = logging.getLogger('granted_roles')


SUPPORTED_ROLES = {'admin', 'owner', 'reader', 'uploader', 'writer', 'presigner', 'any'}


@given(u'an empty granted roles DB')
def step_empty_granted_roles_db(context):
    granted_roles_list = context.connect.get_func('v1_code.list_granted_roles')
    if granted_roles_list.errcode is not None:
        raise RuntimeError('failed to get all granted roles: {}'.format(
            granted_roles_list.errcode))

    for granted_role in granted_roles_list.records:
        remove_granted_role = context.connect.get_func(
            'v1_code.remove_granted_role',
            i_service_id=granted_role['service_id'],
            i_role=granted_role['role'],
            i_grantee_uid=granted_role['grantee_uid'],
        )
        if remove_granted_role.errcode is not None:
            raise RuntimeError(
                'failed to remove granted role: role="{role}", error="{error}"'.format(
                    role=granted_role,
                    error=remove_granted_role.errcode,
                ))

    tvm2_granted_roles_list = context.connect.get_func('v1_code.list_tvm2_granted_roles')
    if tvm2_granted_roles_list.errcode is not None:
        raise RuntimeError('failed to get all TVM2 granted roles: {}'.format(
            tvm2_granted_roles_list.errcode))

    for granted_role in tvm2_granted_roles_list.records:
        remove_granted_role = context.connect.get_func(
            'v1_code.remove_tvm2_granted_role',
            i_service_id=granted_role['service_id'],
            i_role=granted_role['role'],
            i_grantee_uid=granted_role['grantee_uid'],
        )
        if remove_granted_role.errcode is not None:
            raise RuntimeError(
                'failed to remove TVM2 granted role: role="{role}", error="{error}"'.format(
                    role=granted_role,
                    error=remove_granted_role.errcode,
                ))


@given(u'the account "{service_id:d}" granted role "{role}" to the user "{grantee_uid:d}"')
def step_setup_granted_role(context, service_id, role, grantee_uid):
    context.execute_steps(u'''
        When the account "{service_id}" grants role "{role}" to the user "{grantee_uid}"
        Then the grant request succeeds
    '''.format(
        service_id=service_id,
        role=role,
        grantee_uid=grantee_uid,
    ))


@given(u'the account "{service_id:d}" granted role "{role}" to the TVM2 "{grantee_uid:d}"')
def step_setup_tvm2_granted_role(context, service_id, role, grantee_uid):
    context.execute_steps(u'''
        When the account "{service_id}" grants role "{role}" to the TVM2 "{grantee_uid}"
        Then the grant request succeeds
    '''.format(
        service_id=service_id,
        role=role,
        grantee_uid=grantee_uid,
    ))


@given(u'we have the following granted roles')
def set_setup_granted_roles(context):
    headings = context.table.headings
    assert {'service_id', 'role', 'grantee_uid'} == set(headings), (
        'Unexpected granted role settings: {}'.format(headings))

    for granted_role in context.table.rows:
        context.execute_steps(u'''
            Given the account "{service_id}" granted role "{role}" to the user "{grantee_uid}"
        '''.format(
            service_id=granted_role['service_id'],
            role=granted_role['role'],
            grantee_uid=granted_role['grantee_uid'],
        ))


@given(u'we have the following TVM2 granted roles')
def set_setup_tvm2_granted_roles(context):
    headings = context.table.headings
    assert {'service_id', 'role', 'grantee_uid'} == set(headings), (
        'Unexpected granted role settings: {}'.format(headings))

    for granted_role in context.table.rows:
        context.execute_steps(u'''
            Given the account "{service_id}" granted role "{role}" to the TVM2 "{grantee_uid}"
        '''.format(
            service_id=granted_role['service_id'],
            role=granted_role['role'],
            grantee_uid=granted_role['grantee_uid'],
        ))


@when(u'the account "{service_id:d}" grants role "{role}" to the user "{grantee_uid:d}"')
def step_grant_account_role(context, service_id, role, grantee_uid):
    if role not in SUPPORTED_ROLES:
        raise ValueError('invalid account role: {}'.format(role))

    context.grant_request_result = context.connect.get_func(
        'v1_code.grant_role',
        i_service_id=service_id,
        i_role=role,
        i_grantee_uid=grantee_uid,
    )


@when(u'the account "{service_id:d}" grants role "{role}" to the TVM2 "{grantee_uid:d}"')
def step_tvm2_grant_account_role(context, service_id, role, grantee_uid):
    if role not in SUPPORTED_ROLES:
        raise ValueError('invalid account role: {}'.format(role))

    context.grant_request_result = context.connect.get_func(
        'v1_code.tvm2_grant_role',
        i_service_id=service_id,
        i_role=role,
        i_grantee_uid=grantee_uid,
    )


@when(u'the account "{service_id:d}" removes role "{role}" from the user "{grantee_uid:d}"')
def step_remove_granted_account_role(context, service_id, role, grantee_uid):
    if role not in SUPPORTED_ROLES:
        raise ValueError('invalid account role: {}'.format(role))

    context.grant_request_result = context.connect.get_func(
        'v1_code.remove_granted_role',
        i_service_id=service_id,
        i_role=role,
        i_grantee_uid=grantee_uid,
    )


@when(u'the account "{service_id:d}" removes role "{role}" from the TVM2 "{grantee_uid:d}"')
def step_remove_granted_account_role(context, service_id, role, grantee_uid):
    if role not in SUPPORTED_ROLES:
        raise ValueError('invalid account role: {}'.format(role))

    context.grant_request_result = context.connect.get_func(
        'v1_code.remove_tvm2_granted_role',
        i_service_id=service_id,
        i_role=role,
        i_grantee_uid=grantee_uid,
    )


@then(u'the grant request succeeds')
def step_validate_successful_grant_request(context):
    assert context.grant_request_result.errcode is None, (
        'Grant account role request failed: {}'.format(context.grant_request_result.errcode))


@then(u'the grant request fails with error "{errcode}"')
def step_validate_failed_grant_request(context, errcode):
    assert errcode == context.grant_request_result.errcode, (
        'Grant account role request fails with different errcode: expected="{}", actual="{}"'.format(
            errcode, context.grant_request_result.errcode
        ))


@then(u'we have the following granted roles')
def step_list_granted_roles(context):
    granted_roles = context.connect.get_func('v1_code.list_granted_roles')
    if granted_roles.errcode is not None:
        raise RuntimeError('failed to list granted roles: {}'.format(granted_roles.errcode))

    def granted_role_key(granted_role):
        return granted_role['service_id'], granted_role['role'], granted_role['grantee_uid']

    actual_granted_roles = sorted(
        (
            dict((heading, str(record[heading])) for heading in context.table.headings)
            for record in granted_roles.records
        ),
        key=granted_role_key,
    )
    expected_granted_roles = sorted(
        (
            dict((heading, row[heading]) for heading in context.table.headings)
            for row in context.table.rows
        ),
        key=granted_role_key,
    )

    assert actual_granted_roles == expected_granted_roles, (
        'Actual granted roles list differs from the expected one: actual="{}", expected="{}"'.format(
            actual_granted_roles, expected_granted_roles))


@then(u'we have the following TVM2 granted roles')
def step_list_tvm2_granted_roles(context):
    tvm2_granted_roles = context.connect.get_func('v1_code.list_tvm2_granted_roles')
    if tvm2_granted_roles.errcode is not None:
        raise RuntimeError('failed to list TVM2 granted roles: {}'.format(tvm2_granted_roles.errcode))

    def granted_role_key(granted_role):
        return granted_role['service_id'], granted_role['role'], granted_role['grantee_uid']

    actual_granted_roles = sorted(
        (
            dict((heading, str(record[heading])) for heading in context.table.headings)
            for record in tvm2_granted_roles.records
        ),
        key=granted_role_key,
    )
    expected_granted_roles = sorted(
        (
            dict((heading, row[heading]) for heading in context.table.headings)
            for row in context.table.rows
        ),
        key=granted_role_key,
    )

    assert actual_granted_roles == expected_granted_roles, (
        'Actual granted roles list differs from the expected one: actual="{}", expected="{}"'.format(
            actual_granted_roles, expected_granted_roles))
