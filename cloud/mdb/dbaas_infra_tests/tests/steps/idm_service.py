"""
Steps related to IDM Service
"""
import json
from time import time

from behave import given, then, when
from hamcrest import assert_that, empty, has_item, has_key, is_not

from tests.helpers import idm_service, internal_api
from tests.helpers.metadb import (disable_ch_zk_tls, enable_ch_zk_acl, enable_ch_zk_tls)
from tests.helpers.step_helpers import step_require


@given('cluster is managed by IDM')
def step_set_sox_audit_flag(context):
    resp = internal_api.request(
        context=context,
        method='PATCH',
        handle=f'mdb/{context.cluster_type}/1.0/clusters/{context.cluster["id"]}',
        data=json.dumps({
            'configSpec': {
                'soxAudit': True,
            },
        }))
    context.operation_id = resp.json().get('id')


@when('we attempt to {action} grant "{role}" to "{login}"' ' on cluster "{cluster_name}"')
@when('we attempt to {action} grant "{role}" from "{login}"' ' on cluster "{cluster_name}"')
@step_require('folder')
def step_modify_user_grant(context, action, login, role, cluster_name):
    internal_api.load_cluster_into_context(context, cluster_name)
    method = 'add_role' if action == 'add' else 'remove_role'
    call = getattr(idm_service, method)
    call(context, login, context.cluster['id'], role)
    context.operation_id = idm_service.idm_pending_task(context)


@when('we attempt to rotate passwords on IDM managed clusters')
@step_require('folder')
def step_rotate_passwords(context):
    idm_service.rotate_passwords(context)
    context.operation_id = idm_service.idm_rotate_passwords_task(context)


@then('user "{login}" {ownership} grant "{role}" on cluster "{cluster_name}"')
@step_require('folder', 'cluster_config')
def step_user_has_grant(context, login, role, cluster_name, ownership='has'):
    must_have = ownership == 'has'
    internal_api.load_cluster_into_context(context, cluster_name)
    user_roles = idm_service.fetch_user_roles(context, login)
    assert_that(user_roles, has_item(role) if must_have else is_not(has_item(role)))

    result = idm_service.get_roles(context)
    users = [d for d in result['users'] if d['login'] == login]
    if not must_have and not users:
        return
    assert_that(users, is_not(empty()))
    roles = users[0]['roles']
    cluster_role = {'cluster': context.cluster['id'], 'grants': role}
    assert_that(roles, has_item(cluster_role) if must_have else is_not(has_item(cluster_role)))


@then('all IDM users on cluster "{cluster_name}" have new passwords')
@step_require('folder', 'cluster_config')
def step_idm_new_passwords(context, cluster_name):
    internal_api.load_cluster_into_context(context, cluster_name)
    users = idm_service.fetch_idm_users(context)
    for _, info in users.items():
        assert_that(info, has_key('last_password_update'))
        assert_that(info['last_password_update'] > time() - 1000)


@given('TLS disabled in cluster "{cluster_name}"')
def step_disable_ch_zk_tls(context, cluster_name):
    disable_ch_zk_tls(context, cluster_name)


@given('TLS enabled in cluster "{cluster_name}"')
def step_enable_ch_zk_tls(context, cluster_name):
    enable_ch_zk_tls(context, cluster_name)


@given('ACL enabled in cluster "{cluster_name}"')
def step_enable_ch_zk_acl(context, cluster_name):
    enable_ch_zk_acl(context, cluster_name)


@when("we send request to get IDM clusters")
def step_request_idm_clusters(context):
    context.response = idm_service.get_clusters(context)


@then('IDM response OK and body has "{clusters}"')
@step_require('response')
def step_response_idm_clusters(context, clusters):
    status = context.response.status_code
    assert status == 200, f'Unexpected status code {status}.'

    body = context.response.json()
    assert str(body['code']) == str(0), f'Unexpected body code [body={body}].'

    actual_clusters = {x['name'].lower() for x in body['clusters']}
    clusters = {x.strip().lower() for x in clusters.split(',')}
    assert actual_clusters == clusters, f'Got unexpected response [body={body}]'
