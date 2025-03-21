# -*- coding: utf-8 -*-
'''
Management of PostgreSQL users (roles)
======================================

The mdb_postgres_user module is fork of postgres_user

.. code-block:: yaml

    frank:
      postgres_user.present
'''
from __future__ import absolute_import, unicode_literals, print_function

# Import Python libs
import datetime
import logging
import os.path

# Import salt libs

# Salt imports
try:
    # Import salt module, but not in arcadia tests
    from salt.ext import six
except ImportError as e:
    import cloud.mdb.salt_tests.common.arc_utils as arc_utils

    arc_utils.raise_if_not_arcadia(e)

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}

log = logging.getLogger(__name__)


def __virtual__():
    '''
    Only load if the postgres module is present
    '''
    if 'postgres.user_exists' not in __salt__:
        return (False, 'Unable to load postgres module.  Make sure `postgres.bins_dir` is set.')
    return True


def check_psql(user=None, host=None, port=None, maintenance_db=None, password=None, runas=None):
    code, _, _ = __salt__['postgres.psql_exec'](
        'SELECT 1', user=user, host=host, port=port, maintenance_db=maintenance_db, password=password, runas=runas
    )
    return code == 0


def present(
    name,
    createdb=None,
    createroles=None,
    encrypted=None,
    superuser=None,
    replication=None,
    inherit=None,
    login=None,
    password=None,
    default_password=None,
    refresh_password=None,
    valid_until=None,
    groups=None,
    user=None,
    maintenance_db=None,
    db_password=None,
    db_host=None,
    db_port=None,
    db_user=None,
    pgpass_files=None,
):
    '''
    Ensure that the named user is present with the specified privileges
    Please note that the user/group notion in postgresql is just abstract, we
    have roles, where users can be seens as roles with the LOGIN privilege
    and groups the others.

    name
        The name of the system user to manage.

    createdb
        Is the user allowed to create databases?

    createroles
        Is the user allowed to create other users?

    encrypted
        Should the password be encrypted in the system catalog?

    login
        Should the group have login perm

    inherit
        Should the group inherit permissions

    superuser
        Should the new user be a "superuser"

    replication
        Should the new user be allowed to initiate streaming replication

    password
        The system user's password. It can be either a plain string or a
        md5 postgresql hashed password::

            'md5{MD5OF({password}{role}}'

        If encrypted is None or True, the password will be automatically
        encrypted to the previous
        format if it is not already done.

    default_password
        The password used only when creating the user, unless password is set.

        .. versionadded:: 2016.3.0

    refresh_password
        Password refresh flag

        Boolean attribute to specify whether to password comparison check
        should be performed.

        If refresh_password is ``True``, the password will be automatically
        updated without extra password change check.

        This behaviour makes it possible to execute in environments without
        superuser access available, e.g. Amazon RDS for PostgreSQL

    valid_until
        A date and time after which the role's password is no longer valid.

    groups
        A string of comma separated groups the user should be in

    user
        System user all operations should be performed on behalf of

        .. versionadded:: 0.17.0

    db_user
        Postgres database username, if different from config or default.

    db_password
        Postgres user's password, if any password, for a specified db_user.

    db_host
        Postgres database host, if different from config or default.

    db_port
        Postgres database port, if different from config or default.

    pgpass_files
        List of .pgpass to update password in

    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': 'User {0} is already present'.format(name)}

    # default to encrypted passwords
    if encrypted is not False:
        encrypted = __salt__['postgres.get_default_password_encryption']()
    # maybe encrypt if it's not already and necessary
    if pgpass_files:
        if __salt__['postgres.is_password_already_encrypted'](password):
            ret['result'] = False
            ret['comment'] = 'pgpass_files option can not be used with encrypted passwords'
            return ret
        else:
            raw_password = password

    password = __salt__['postgres.maybe_encrypt_password'](name, password, encrypted=encrypted)

    if default_password is not None:
        default_password = __salt__['postgres.maybe_encrypt_password'](name, default_password, encrypted=encrypted)

    db_args = {
        'maintenance_db': maintenance_db,
        'runas': user,
        'host': db_host,
        'user': db_user,
        'port': db_port,
        'password': db_password,
    }

    # check if user exists
    mode = 'create'
    user_attr = __salt__['postgres.role_get'](name, return_password=not refresh_password, **db_args)
    if user_attr is not None:
        mode = 'update'

    cret = None
    update = {}
    if mode == 'update':
        user_groups = user_attr.get('groups', [])
        if createdb is not None and user_attr['can create databases'] != createdb:
            update['createdb'] = createdb
        if inherit is not None and user_attr['inherits privileges'] != inherit:
            update['inherit'] = inherit
        if login is not None and user_attr['can login'] != login:
            update['login'] = login
        if createroles is not None and user_attr['can create roles'] != createroles:
            update['createroles'] = createroles
        if replication is not None and user_attr['replication'] != replication:
            update['replication'] = replication
        if superuser is not None and user_attr['superuser'] != superuser:
            update['superuser'] = superuser
        if password is not None and (refresh_password or user_attr['password'] != password):
            update['password'] = True
        if valid_until is not None:
            valid_until_dt = __salt__['postgres.psql_query'](
                'SELECT \'{0}\'::timestamp(0) as dt;'.format(valid_until.replace('\'', '\'\'')), **db_args
            )[0]['dt']
            try:
                valid_until_dt = datetime.datetime.strptime(valid_until_dt, '%Y-%m-%d %H:%M:%S')
            except ValueError:
                valid_until_dt = None
            if valid_until_dt != user_attr['expiry time']:
                update['valid_until'] = valid_until
        if groups is not None:
            lgroups = groups
            if isinstance(groups, (six.string_types, six.text_type)):
                lgroups = lgroups.split(',')
            if isinstance(lgroups, list):
                missing_groups = [a for a in lgroups if a not in user_groups]
                if missing_groups:
                    update['groups'] = missing_groups

    if mode == 'create' and password is None:
        password = default_password

    if mode == 'create' or (mode == 'update' and update):
        if __opts__['test']:
            if check_psql(**db_args):
                if update:
                    ret['changes'][name] = update
                ret['result'] = None
                ret['comment'] = 'User {0} is set to be {1}d'.format(name, mode)
            else:
                ret['result'] = False
                ret['comment'] = 'PostgreSQL is dead to {mode} user "{user}"'.format(mode=mode, user=name)
            return ret
        cret = __salt__['postgres.user_{0}'.format(mode)](
            username=name,
            createdb=createdb,
            createroles=createroles,
            encrypted=encrypted,
            superuser=superuser,
            login=login,
            inherit=inherit,
            replication=replication,
            rolepassword=password,
            valid_until=valid_until,
            groups=groups,
            **db_args
        )
    else:
        cret = None

    if cret:
        ret['comment'] = 'The user {0} has been {1}d'.format(name, mode)
        if update:
            ret['changes'][name] = update
        else:
            ret['changes'][name] = 'Present'
    elif cret is not None:
        ret['comment'] = 'Failed to create user {0}'.format(name)
        ret['result'] = False
    else:
        ret['result'] = True

    if pgpass_files and ret['result']:
        if isinstance(pgpass_files, str):
            pgpass_files = [pgpass_files]
        for f in pgpass_files:
            _update_pgpass_files(f, name, raw_password)

    return ret


def absent(name, user=None, maintenance_db=None, db_password=None, db_host=None, db_port=None, db_user=None):
    '''
    Ensure that the named user is absent

    name
        The username of the user to remove

    user
        System user all operations should be performed on behalf of

        .. versionadded:: 0.17.0

    db_user
        database username if different from config or default

    db_password
        user password if any password for a specified user

    db_host
        Database host if different from config or default

    db_port
        Database port if different from config or default
    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    db_args = {
        'maintenance_db': maintenance_db,
        'runas': user,
        'host': db_host,
        'user': db_user,
        'port': db_port,
        'password': db_password,
    }
    # check if user exists and remove it
    if __salt__['postgres.user_exists'](name, **db_args):
        if __opts__['test']:
            if check_psql(**db_args):
                ret['result'] = None
                ret['comment'] = 'User {0} is set to be removed'.format(name)
            else:
                ret['result'] = False
                ret['comment'] = 'PostgreSQL is dead to remove user "{user}"'.format(user=name)
            return ret
        if __salt__['postgres.user_remove'](name, **db_args):
            ret['comment'] = 'User {0} has been removed'.format(name)
            ret['changes'][name] = 'Absent'
            return ret
        else:
            ret['result'] = False
            ret['comment'] = 'User {0} failed to be removed'.format(name)
            return ret
    else:
        ret['comment'] = 'User {0} is not present, so it cannot ' 'be removed'.format(name)

    return ret


def _update_pgpass_files(file, name, password):
    if not os.path.exists(file):
        return
    with open(file, "rw+") as fh:
        changed = []
        for line in fh.readlines():
            line = line.rstrip()
            parts = line.split(':')
            if len(parts) == 5 and parts[3] == name:
                if parts[4] == password:
                    return
                parts[4] = password
                line = ':'.join(parts)
            changed.append(line)
        fh.truncate(0)
        fh.seek(0)
        fh.write("\n".join(changed) + "\n")
        fh.flush()
