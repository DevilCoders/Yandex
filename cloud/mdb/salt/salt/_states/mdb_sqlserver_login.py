# -*- coding: utf-8 -*-

'''
Management of Microsoft SQLServer Logins
========================================

The mdb_sqlserver_login module is used to create
and manage SQL Server Logins

.. code-block:: yaml

    frank:
      mdb_sqlserver_login.present
        - name: login
'''
from __future__ import absolute_import, print_function, unicode_literals
import collections
import salt.utils.odict


def __virtual__():
    """
    Only load if the mdb_sqlserver module is present
    """
    return 'mdb_sqlserver.user_role_list' in __salt__


def present(name, password=None, domain=None, server_roles=None, options=None, **kwargs):
    """
    Checks existance of the named login.
    If not present, creates the login with the specified roles and options.

    name
        The name of the login to manage
    password
        Creates a SQL Server authentication login
        Since hashed passwords are varbinary values, if the
        new_login_password is 'long', it will be considered
        to be HASHED.
    domain
        Creates a Windows authentication login.
        Needs to be NetBIOS domain or hostname
    server_roles
        Add this login to all the server roles in the list
    options
        Can be a list of strings, a dictionary, or a list of dictionaries
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    login_existed = False
    login_created = False
    password_match = True
    password_set = False
    SQLLogin = (False,)
    roles_changes = (False,)

    if domain:
        principal_name = '{0}\\{1}'.format(domain, name)
    else:
        principal_name = name

    if bool(password) == bool(domain):
        ret['result'] = False
        ret['comment'] = 'One and only one of password and domain should be specified'
        return ret

    login_existed = __salt__['mssql.login_exists'](name, domain=domain, **kwargs)
    if login_existed:
        ret['comment'] = 'Login {0} is already present.'.format(name)
        SQLLogin = __salt__['mdb_sqlserver.login_property_get'](name, 'IsSQLLogin', **kwargs)
        if SQLLogin:
            password_match = __salt__['mdb_sqlserver.login_password_match'](name, password, **kwargs)
            if not password_match:
                __salt__['mdb_sqlserver.login_password_set'](name, password, **kwargs)
                password_set = True
                ret['changes'][name] = 'Password set.'

    else:
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = 'Login {0} is set to be added'.format(name)
            ret['changes'][name] = 'Present'
        else:
            login_created = __salt__['mdb_sqlserver.login_create'](
                name,
                new_login_password=password,
                new_login_domain=domain,
                new_login_roles=server_roles,
                new_login_options=options,
                **kwargs
            )

            if not login_created:
                ret['result'] = False
                ret['comment'] = 'Login {0} failed to be added: {1}'.format(name, login_created)
                return ret
            else:
                ret['comment'] = 'Login {0} has been added. '.format(name)
                ret['changes'][name] = 'Present'

    roles_present = []
    roles_p = __salt__['mdb_sqlserver.login_role_list'](name, **kwargs)
    roles_present += roles_p

    if not server_roles:
        server_roles = []
    roles_missing = list(set(server_roles) - set(roles_present))
    roles_todrop = list(set(roles_present) - set(server_roles))
    if name == 'sa':
        roles_todrop = []
        roles_missing = []
    comment = ''
    if roles_missing or roles_todrop:
        ret['comment'] += '; Login {0} roles is set to be modified'.format(name)
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] += '; Login {0} roles is set to be modified'.format(name)
            if login_created:
                ret['changes'][name] = 'Present, roles altered'
            else:
                ret['changes'][name] = 'Roles altered'
            return ret

        roles_changed = __salt__['mdb_sqlserver.principal_role_mod'](
            principal=principal_name, is_login=1, roles_add=roles_missing, roles_drop=roles_todrop, **kwargs
        )
        if not roles_changed:
            ret['comment'] += 'Roles modification failed: {e}'.format(e=roles_changed)
            return ret
        if roles_changed:
            if roles_missing:
                comment += '; Login {0}: +roles: {1}'.format(principal_name, ','.join(roles_missing))
            if roles_todrop:
                comment += '; Login {0}: -roles: {1}'.format(principal_name, ','.join(roles_todrop))
        ret['comment'] += comment
        if login_created:
            ret['changes'][name] = 'Present, roles altered'
        else:
            if password_set:
                ret['changes'][name] = 'Password set, roles altered'
            else:
                ret['changes'][name] = 'Roles altered'
    return ret


def absent(name, **kwargs):
    """
    Ensure that the named login is absent

    name
        The name of the login to remove
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if not __salt__['mssql.login_exists'](name):
        ret['comment'] = 'Login {0} is not present'.format(name)
        return ret
    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = 'Login {0} is set to be removed'.format(name)
        return ret
    if __salt__['mssql.login_remove'](name, **kwargs):
        ret['comment'] = 'Login {0} has been removed'.format(name)
        ret['changes'][name] = 'Absent'
        return ret

    ret['result'] = False
    ret['comment'] = 'Login {0} failed to be removed'.format(name)
    return ret
