# coding: utf-8

from __future__ import division
from __future__ import unicode_literals

import config
from gaux.aux_decorators import decayed_memoize
from gaux.aux_utils import retry_requests_get


@decayed_memoize(120)
def resolve_abc_service(service_id, oauth_token):
    """
    Resolves ABC :param:`service_id` to list of logins.

    :type service_id: Union[str, unicode]
    :rtype: Iterable[Union[str, unicode]]
    """

    headers = {
        'Authorization': 'OAuth {}'.format(oauth_token),
        'Accept': 'application/json',
    }
    url = 'https://abc-back.yandex-team.ru/api/v4/services/members/?fields=person.login&service={}'.format(service_id)

    users = set()

    while True:
        response = retry_requests_get(5, url, headers=headers, allow_redirects=False, verify=False)
        if response.status_code == 404:  # service not found
            return []
        elif response.status_code == 200:
            service_info = response.json()
            for elem in service_info['results']:
                if 'person' in elem:
                    users.add(elem['person']['login'])

            if service_info['next'] is not None:
                url = service_info['next']
            else:
                break
        elif response.status_code == 302:  # invalid auth token
            raise Exception('Got <302> response when requesting {} (might be wrong oauth token)'.format(url))
        else:
            raise Exception('Got bad status <{}>'.format.response.status_code)

    print 'Service {} users: {}'.format(service_id, ' '.join(users))

    return users


def list_abc_services():
    from core.settings import SETTINGS

    url = '{}/services/?fields=parent,slug,id&page_size=1000'.format(SETTINGS.services.abc.rest.url)
    headers = {
        'Authorization': 'OAuth {}'.format(config.get_default_oauth()),
        'Accept': 'application/json',
    }

    result = []
    while True:
        response = retry_requests_get(5, url, headers=headers, allow_redirects=False, verify=False)
        if response.status_code == 200:
            response_json = response.json()
            result.extend(response_json['results'])

            if response_json['next'] is not None:
                url = response_json['next']
            else:
                break
        else:
            raise Exception('Request <{}> returned status code <{}>'.format(url, response.status_code))

    return result


def list_abc_services_members():
    from core.settings import SETTINGS

    url = '{}/services/members/?fields=role.scope,person.login,service.slug&page_size=500'.format(SETTINGS.services.abc.rest.url)
    headers = {
        'Authorization': 'OAuth {}'.format(config.get_default_oauth()),
        'Accept': 'application/json',
    }

    result = []
    while True:
        response = retry_requests_get(5, url, headers=headers, allow_redirects=False, verify=False)
        if response.status_code == 200:
            response_json = response.json()
            result.extend(response_json['results'])

            if response_json['next'] is not None:
                url = response_json['next']
            else:
                break
        else:
            raise Exception('Request <{}> returned status code <{}>'.format(url, response.status_code))

    return result


def parse_abc_service_role(abc_service_full_role):
    from core.db import CURDB

    # Example: role_id=123, role_name=qwerty, 123, qwerty
    if '=' in abc_service_full_role:
        name, value = abc_service_full_role.split('=')
        if name == 'role_name' and CURDB.abcgroups.has_role(value):
            return value
        elif name == 'role_id' and CURDB.abcgroups.has_role(int(value)):
            return CURDB.abcgroups.get_role(int(value)).name
    elif CURDB.abcgroups.has_role(abc_service_full_role):
        return abc_service_full_role
    return None


def parse_abc_service_name(abc_service_full_name):
    from core.db import CURDB

    sep = ':' if ':' in abc_service_full_name else '_'
    name_parts = abc_service_full_name.split(sep)
    if len(name_parts) == 1:
        return (name_parts[0], None) if CURDB.abcgroups.has(name_parts[0]) else (None, None)
    elif sep == ':' and len(name_parts) > 2:
        return None, None

    abc_service_name = '_'.join(name_parts[:-1])
    abc_service_role_name = parse_abc_service_role(name_parts[-1])

    if CURDB.abcgroups.has(abc_service_name) and abc_service_role_name is not None:
        abc_service = CURDB.abcgroups.get(abc_service_name)
        abc_service_roles_names = {CURDB.abcgroups.get_role(x).name: x for x in abc_service.roles}

        if abc_service_role_name in abc_service_roles_names:
            return abc_service_name, abc_service_roles_names[abc_service_role_name]

    if CURDB.abcgroups.has(abc_service_full_name):
        return abc_service_full_name, None

    return None, None


def unwrap_abc_group(name):
    """Unwrap abc group name to list of users (RX-447)

    <abc:groupname> is converted to all owners of corresponding abc group
    <abc:groupname:role=role_id> is converted to owners with specified role
    <abc:groupname_role> is converted to owners with specified role
    <abc:group_name> is converted to all owners of corresponding abc group
    <abc:group_name_role> is converted to owners with specified role
    """
    assert name.startswith('abc:'), 'Not an abc group: <{}>'.format(name)

    from core.db import CURDB

    if CURDB.version < '2.2.56':
        name = name.partition(':')[2]
        abc_service_name, abc_role_id = parse_abc_service_name(name)

        if abc_service_name is None:
            return []

        abc_service = CURDB.abcgroups.get(abc_service_name)

        result = []
        if abc_role_id is None:
            for user_names in abc_service.roles.itervalues():
                result.extend(user_names)
        else:
            result.extend(abc_service.roles[abc_role_id])

        return sorted(set(result))
    else:
        service_slug, scope_slug = CURDB.abcgroups.get_service_scope_slug(name)
        return sorted(set(CURDB.abcgroups.get_service_members(service_slug, scope_slug)))


def abc_roles_to_nanny_json(group):
    """Convert group abc owners to nanny as requested in https://st.yandex-team.ru/RX-447#1526636879000 (RX-447)"""
    if group.parent.db.version < '2.2.56':
        abc_group_names = [x.partition(':')[2] for x in group.card.owners if x.startswith('abc:')]

        role_ids_by_abc_group = dict()
        for abc_group_name in abc_group_names:
            abc_group_name, _, role_info = abc_group_name.partition(':')
            abc_group = group.parent.db.abcgroups.get(abc_group_name)
            if abc_group.id not in role_ids_by_abc_group:
                role_ids_by_abc_group[abc_group.id] = []

            if role_info == '':
                role_ids_by_abc_group[abc_group.id] = None
            else:
                role_id = int(role_info.partition('=')[2])
                if role_ids_by_abc_group[abc_group.id] is not None:
                    role_ids_by_abc_group[abc_group.id].append(role_id)

        scope_ids_by_abc_service = role_ids_by_abc_group

    else:
        abc_group_names = [x for x in group.card.owners if x.startswith('abc:')]

        scope_ids_by_abc_service = {}
        for abc_group_name in abc_group_names:
            abc_service, abc_scope = group.parent.db.abcgroups.get_service_scope_slug(abc_group_name)
            abc_service = group.parent.db.abcgroups.get_service(abc_service)

            if abc_service['id'] not in scope_ids_by_abc_service:
                scope_ids_by_abc_service[abc_service['id']] = []

            if abc_scope is None:
                scope_ids_by_abc_service[abc_service['id']] = None
            else:
                scope_id = group.parent.db.abcgroups.get_scope(abc_scope)['id']
                if scope_ids_by_abc_service[abc_service['id']] is not None:
                    scope_ids_by_abc_service[abc_service['id']].append(scope_id)

    # generate result
    result = []
    for abc_service_id in sorted(scope_ids_by_abc_service):
        scope_ids = scope_ids_by_abc_service[abc_service_id]
        if scope_ids is None:
            result.append(dict(
                service_id=abc_service_id,
                role_policy='ALL',
                role_ids=[],
            ))
        else:
            result.append(dict(
                service_id=abc_service_id,
                role_policy='ROLE_IDS_LIST',
                role_ids=sorted(scope_ids)
            ))

    return result


def exists_service_scope_slug(formated_string, db=None):
    from core.db import CURDB

    db = db or CURDB
    return db.abcgroups.has_service_slug(formated_string)
