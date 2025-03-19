# -*- coding: utf-8 -*-

'''
Management of SQLServer users
========================================

The mdb_sqlserver_user module is used to create
and manage SQLServer users and their role membership

.. code-block:: yaml

    frank:
      mdb_sqlserver_user.present
        - domain: mydomain
'''
from __future__ import absolute_import, print_function, unicode_literals
import collections
import salt.utils.odict


def __virtual__():
    """
    Only load if the mdb_sqlserver module is present
    """
    return 'mdb_sqlserver.user_role_list' in __salt__


FIXED_ROLES = [
    'db_owner',
    'db_accessadmin',
    'db_securityadmin',
    'db_ddladmin',
    'db_backupoperator',
    'db_datareader',
    'db_datawriter',
    'db_denydatareader',
    'db_denydatawriter',
]


def escape_obj_name(var):
    var = var.replace("]", "]]")
    var = '[' + var + ']'
    return var


def present(name, database, login=None, domain=None, dbroles=None, options=None, **kwargs):
    """
    Checks existance of the named user in database with a set of roles.

    name
        The name of a user to be checked
    database
        The name of the database where the user should exist
    dbroles
        The list of roles, that the this user should be member of
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    user_created = False
    if domain:
        principal_name = '{0}\\{1}'.format(domain, name)
    else:
        principal_name = name
    if __salt__['mdb_sqlserver.user_exists'](name, domain=domain, database=database, **kwargs):
        ret['comment'] = 'User {0} is already present'.format(name)
        user_created = True
    if not user_created:
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = 'User {0} is set to be added'.format(name)
            ret['changes'][name] = 'Present'
    comment = ''
    if not user_created and not __opts__['test']:
        ret['comment'] = 'User {0} is set to be added'.format(name)
        user_created = __salt__['mdb_sqlserver.user_create'](
            new_username=name,
            new_login=name,
            new_domain=domain,
            new_database=database,
            new_roles=dbroles,
            new_options=options,
            **kwargs
        )
        if user_created is not True:
            ret['result'] = False
            ret['comment'] = 'User {0} failed to be added: {1}'.format(name, user_created)
            return ret
        else:
            ret['comment'] = 'User {0} has been added. '.format(name)
            ret['changes'][name] = 'Present'

    roles_present = []
    roles_p = __salt__['mdb_sqlserver.user_role_list'](name, database, domain, **kwargs)
    roles_present += roles_p

    if not dbroles:
        dbroles = []
    roles_missing = list(set(dbroles) - set(roles_present))
    roles_todrop = list((set(roles_present) - set(dbroles)) & set(FIXED_ROLES))

    if roles_missing or roles_todrop:
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] += '; User {0} roles is set to be modified'.format(name)
            if ret['changes'].get(name) == 'Present':
                ret['changes'][name] = 'Present, roles altered'
            else:
                ret['changes'][name] = 'Roles altered'
            return ret
        roles_changed = __salt__['mdb_sqlserver.principal_role_mod'](
            principal=principal_name,
            is_login=False,
            database=database,
            roles_add=roles_missing,
            roles_drop=roles_todrop,
            **kwargs
        )
        if roles_changed is not True:
            ret['comment'] += 'Roles modification failed: {e}'.format(e=roles_changed)
            return ret
        if len(roles_missing) > 0 and roles_changed is True:
            comment += '; User {username}:has roles:{hasroles}; +roles: {roles}'.format(
                username=name, hasroles=roles_p, roles=','.join(roles_missing)
            )
        if len(roles_todrop) > 0 and roles_changed is True:
            comment += '; User {username}: -roles: {roles}'.format(username=name, roles=','.join(roles_todrop))
        ret['comment'] += comment
        if ret['changes'].get(name) == 'Present':
            ret['changes'][name] = 'Present, roles altered'
        else:
            ret['changes'][name] = 'Roles altered'

    return ret


def absent(name, database, login=None, domain=None, **kwargs):
    """
    Checks existence of the named user in database and drops it if it exists.
    All owned schemas and roles are handed over to dbo.

    name
        The name of a user to be checked
    database
        The name of the database where the user should exist
    login
        user's login, not necessary, for future use.
    domain
        user's domain, not necessary. for future use.
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if not __salt__['mdb_sqlserver.user_exists'](name, domain=domain, database=database, **kwargs):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = {name: 'should be dropped'}
        return ret

    ok = __salt__['mdb_sqlserver.user_drop'](username=name, database=database, **kwargs)
    if not ok:
        ret['result'] = False
        ret['comment'] = 'User {0} failed to be dropped'.format(name)
        return ret

    ret['result'] = True
    ret['changes'] = {name: 'dropped'}
    return ret
