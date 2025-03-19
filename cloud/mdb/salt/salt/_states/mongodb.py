# -*- coding: utf-8 -*-
'''
Management of Mongodb users and databases
=========================================

.. note::
    This module requires PyMongo to be installed.
'''

# Import Python libs
from __future__ import absolute_import, print_function, unicode_literals
import difflib
import logging
import json
from functools import partial

try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc

# Define the module's virtual name
__virtualname__ = 'mongodb'
import yaml

try:
    import six
except ImportError:
    from salt.ext import six


# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


log = logging.getLogger(__name__)


def __virtual__():
    if 'mongodb.user_exists' not in __salt__:
        return False
    return __virtualname__


def _yaml_diff(one, another):
    if not (isinstance(one, dict) and isinstance(another, dict)):
        raise TypeError('both arguments must be dicts')

    one_str = yaml.safe_dump(one or '', default_flow_style=False)
    another_str = yaml.safe_dump(another or '', default_flow_style=False)
    differ = difflib.Differ()

    diff = differ.compare(one_str.splitlines(True), another_str.splitlines(True))
    return '\n'.join(diff)


def _normalize_inplace(input_struct):
    # Handle OrderedDicts
    if isinstance(input_struct, dict):
        for key in input_struct:
            input_struct[key] = _normalize_inplace(input_struct[key])
        return dict(input_struct)
    if type(input_struct) in (six.text_type, str, bytes):
        return input_struct
    if isinstance(input_struct, collections_abc.Iterable):
        for num, _ in enumerate(input_struct):
            input_struct[num] = _normalize_inplace(input_struct[num])
        return input_struct
    return input_struct


def _compare_structs(one, other):
    """
    Compare two nested structures for values, ignoring their order
    """

    def _ignore_unhashable_order(item):
        # https://docs.python.org/3/howto/sorting.html#sort-stability-and-complex-sorts
        # Basically preserve original order if dicts are presented
        if isinstance(item, collections_abc.Hashable):
            return item
        return 1

    def _compare(reference, candidate):
        if isinstance(reference, dict):
            if len(reference.keys()) != len(candidate.keys()):
                return False
            for key in reference:
                if not _compare(reference[key], candidate[key]):
                    return False
            return True
        elif isinstance(reference, collections_abc.Iterable) and not isinstance(reference, (six.text_type, str)):
            # zip() truncates the longer element, need to account for that
            if len(reference) != len(candidate):
                return False
            pair = zip(sorted(reference, key=_ignore_unhashable_order), sorted(candidate, key=_ignore_unhashable_order))
            for first, second in pair:
                if not _compare(first, second):
                    return False
            return True
        return reference == candidate

    try:
        return _compare(one, other)
    except (LookupError, TypeError, ValueError):
        return False


def role_present(name, roles=None, privileges=None, user=None, password=None, database=None, host=None, port=None):
    """
    Ensure that custom role exists and corresponds to specifications.
    1. Get a list of roles available.
    2. If role is not found, create it and show its full spec as a diff.
    4. If role exists, and has same options, do nothing.
    3. If role exists, but has different options, calculate diff,
       and (optionally) perform actual update.
    """
    opts = dict(user=user, password=password, host=host, port=port)
    list_roles = partial(__salt__['mongodb.list_roles'], **opts)
    create_role = partial(__salt__['mongodb.create_role'], database=database, **opts)
    update_role = partial(__salt__['mongodb.update_role'], database=database, **opts)
    dry_run = __opts__['test']

    role_spec = {
        'role': name,
        'roles': _normalize_inplace(roles) or [],
        'privileges': _normalize_inplace(privileges) or [],
        'db': database,
    }
    # NOTE: roles list returns a normalized view of role. For example:
    # requested specification: {u'db': u'admin', u'privileges': [], u'role': u'cloudMonitor', u'roles': [u'clusterMonitor']}
    # becomes
    # [{u'db': u'admin', u'privileges': [], u'role': u'cloudMonitor', u'roles': [{u'db': u'admin', u'role': u'clusterMonitor'}]}]
    # And thus a direct comparison becomes impossible and state will fail on 2nd and further runs.
    cur_roles = list_roles(role=name)
    role = cur_roles[0] if cur_roles else None

    log.debug('current roles: %s', cur_roles)
    log.debug('requested specification: %s', role_spec)

    # Create if not exists
    if not role:
        # Create either creates a role or drops an exception.
        changes = role_spec
        if not dry_run:
            create_role(name, roles=roles, privileges=privileges)
            changes = list_roles(role=name)[0]
        return {
            'name': name,
            'changes': {name: 'Present'},
            'result': True,
            # Diff with empty because role didnt exist
            'comment': _yaml_diff({}, changes),
        }
    # Check if properties match.
    if not _compare_structs(role, role_spec):
        changes = role_spec
        if not dry_run:
            update_role(name, roles=roles, privileges=privileges)
            changes = list_roles(role=name)[0]
        return {
            'name': name,
            'changes': {name: 'Present'},
            'result': True,
            # Diff with empty because role didnt exist
            'comment': _yaml_diff(role, changes),
        }
    return {
        'name': name,
        'result': True,
        'changes': {},
        'comment': {},
    }


def database_absent(name, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Ensure that the named database is absent. Note that creation doesn't make sense in MongoDB.

    name
        The name of the database to remove

    user
        The user to connect as (must be able to create the user)

    password
        The password of the user

    host
        The host to connect to

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    # check if database exists and remove it
    if __salt__['mongodb.db_exists'](name, user, password, host, port, authdb=authdb):
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = ('Database {0} is present and needs to be removed').format(name)
            return ret
        if __salt__['mongodb.db_remove'](name, user, password, host, port, authdb=authdb):
            ret['comment'] = 'Database {0} has been removed'.format(name)
            ret['changes'][name] = 'Absent'
            return ret

    # fallback
    ret['comment'] = ('User {0} is not present, so it cannot be removed').format(name)
    return ret


def user_present(
    name, passwd, database="admin", user=None, password=None, host="localhost", port=27017, authdb=None, internal=False
):
    '''
    Ensure that the user is present with the specified properties

    name
        The name of the user to manage

    passwd
        The password of the user to manage

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified with the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    database
        The database in which to create the user

        .. note::
            If the database doesn't exist, it will be created.

    authdb
        The database in which to authenticate

    internal
        If user is internal MDB user (like admin or monitor)

    Example:

    .. code-block:: yaml

        mongouser-myapp:
          mongodb.user_present:
          - name: myapp
          - passwd: password-of-myapp
          # Connect as admin:sekrit
          - user: admin
          - password: sekrit

    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': 'User {0} is already present'.format(name)}

    # Internal users should be created for "admin" database only
    if internal:
        database = "admin"

    # Check for valid port
    try:
        port = int(port)
    except TypeError:
        ret['result'] = False
        ret['comment'] = 'Port ({0}) is not an integer.'.format(port)
        return ret

    # check if user exists
    # fixed 'user_exists' -> 'user_auth'
    user_exists, user_roles = __salt__['mongodb.user_auth'](name, passwd, host, port, database)
    if user_exists is True:
        return ret

    # if the check does not return a boolean, return an error
    # this may be the case if there is a database connection error
    if not isinstance(user_exists, bool):
        ret['comment'] = user_roles
        ret['result'] = False
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = ('User {0} is not present and needs to be created').format(name)
        return ret

    user_found = __salt__['mongodb.user_exists'](name, user, password, host, port, database=database, authdb=authdb)
    func_name = 'mongodb.user_update' if user_found else 'mongodb.user_create'

    if __salt__[func_name](name, passwd, user, password, host, port, database=database, authdb=authdb):
        ret['comment'] = 'User {0} has been {1}'.format(name, 'updated' if user_found else 'created')
        ret['changes'][name] = 'Present'
    else:
        ret['comment'] = 'Failed to create user {0}'.format(name)
        ret['result'] = False

    return ret


def user_create(
    name, passwd, database="admin", user=None, password=None, host="localhost", port=27017, authdb=None, roles=None
):
    '''
    Ensure that the user is present with the specified properties

    name
        The name of the user to manage

    passwd
        The password of the user to manage

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified with the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    database
        The database in which to create the user

        .. note::
            If the database doesn't exist, it will be created.

    authdb
        The database in which to authenticate



    Example:

    .. code-block:: yaml

        mongouser-myapp:
          mongodb.user_present:
          - name: myapp
          - passwd: password-of-myapp
          # Connect as admin:sekrit
          - user: admin
          - password: sekrit

    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': 'User {0} is already present'.format(name)}

    # Check for valid port
    try:
        port = int(port)
    except TypeError:
        ret['result'] = False
        ret['comment'] = 'Port ({0}) is not an integer.'.format(port)
        return ret

    if __opts__['test']:
        ret['result'] = None
        return ret

    # The user is not present, make it!
    if __salt__['mongodb.user_create'](
        name, passwd, user, password, host, port, database=database, authdb=authdb, roles=roles
    ):
        ret['comment'] = 'User {0} has been created'.format(name)
        ret['changes'][name] = 'Present'
    else:
        ret['comment'] = 'Failed to create user {0}'.format(name)
        ret['result'] = False

    return ret


def user_absent(name, user=None, password=None, host=None, port=None, database="admin", authdb=None):
    '''
    Ensure that the named user is absent

    name
        The name of the user to remove

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified by the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    database
        The database from which to remove the user specified by the ``name``
        parameter

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    # check if user exists and remove it
    user_exists = __salt__['mongodb.user_exists'](name, user, password, host, port, database=database, authdb=authdb)
    if user_exists is True:
        if __opts__['test']:
            ret['result'] = None
            ret['comment'] = ('User {0} is present and needs to be removed').format(name)
            return ret
        if __salt__['mongodb.user_remove'](name, user, password, host, port, database=database, authdb=authdb):
            ret['comment'] = 'User {0} has been removed'.format(name)
            ret['changes'][name] = 'Absent'
            return ret

    # if the check does not return a boolean, return an error
    # this may be the case if there is a database connection error
    if not isinstance(user_exists, bool):
        ret['comment'] = user_exists
        ret['result'] = False
        return ret

    # fallback
    ret['comment'] = ('User {0} is not present, so it cannot be removed').format(name)
    return ret


def _roles_to_set(roles, database):
    ret = set()
    for r in roles:
        if isinstance(r, dict):
            if r['db'] == database:
                ret.add(r['role'])
        else:
            ret.add(r)
    return ret


def _user_roles_to_set(user_list, name, database):
    ret = set()

    for item in user_list:
        if item['user'] == name:
            ret = ret.union(_roles_to_set(item['roles'], database))
    return ret


def user_grant_roles(
    name, roles, database="admin", user=None, password=None, host="localhost", port=27017, authdb=None
):
    '''
    Ensure that the named user is granted certain roles

    name
        The name of the user to remove

    roles
        The roles to grant to the user

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified by the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    database
        The database from which to remove the user specified by the ``name``
        parameter

    authdb
        The database in which to authenticate
    '''

    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    if not isinstance(roles, (list, tuple)):
        roles = [roles]

    if not roles:
        ret['result'] = True
        ret['comment'] = "nothing to do (no roles given)"
        return ret

    # Check for valid port
    try:
        port = int(port)
    except TypeError:
        ret['result'] = False
        ret['comment'] = 'Port ({0}) is not an integer.'.format(port)
        return ret

    # check if grant exists
    user_roles_exists = __salt__['mongodb.user_roles_exists'](
        name, roles, database, user=user, password=password, host=host, port=port, authdb=authdb
    )
    if user_roles_exists is True:
        ret['result'] = True
        ret['comment'] = "Roles already assigned"
        return ret

    user_list = __salt__['mongodb.user_list'](
        database=database, user=user, password=password, host=host, port=port, authdb=authdb
    )

    user_set = _user_roles_to_set(user_list, name, database)
    roles_set = _roles_to_set(roles, database)
    diff = roles_set - user_set

    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = "Would have modified roles (missing: {0})".format(diff)
        return ret

    # The user is not present, make it!
    if __salt__['mongodb.user_grant_roles'](
        name, roles, database, user=user, password=password, host=host, port=port, authdb=authdb
    ):
        ret['comment'] = 'Granted roles to {0} on {1}'.format(name, database)
        ret['changes'][name] = ['{0} granted'.format(i) for i in diff]
        ret['result'] = True
    else:
        ret['comment'] = 'Failed to grant roles ({2}) to {0} on {1}'.format(name, database, diff)

    return ret


def user_set_roles(name, roles, database="admin", user=None, password=None, host="localhost", port=27017, authdb=None):
    '''
    Ensure that the named user has the given roles and no other roles

    name
        The name of the user to grant/revoke roles to

    roles
        The roles the given user should have

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified by the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    database
        The database from which to remove the user specified by the ``name``
        parameter

    authdb
        The database in which to authenticate
    '''

    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    if not isinstance(roles, (list, tuple)):
        roles = [roles]

    if not roles:
        ret['result'] = True
        ret['comment'] = "nothing to do (no roles given)"
        return ret

    # Check for valid port
    try:
        port = int(port)
    except TypeError:
        ret['result'] = False
        ret['comment'] = 'Port ({0}) is not an integer.'.format(port)
        return ret

    user_list = __salt__['mongodb.user_list'](
        database=database, user=user, password=password, host=host, port=port, authdb=authdb
    )

    user_set = _user_roles_to_set(user_list, name, database)
    roles_set = _roles_to_set(roles, database)
    to_grant = list(roles_set - user_set)
    to_revoke = list(user_set - roles_set)

    if not to_grant and not to_revoke:
        ret['result'] = True
        ret['comment'] = "User {0} has the appropriate roles on {1}".format(name, database)
        return ret

    if __opts__['test']:
        lsg = ', '.join(to_grant)
        lsr = ', '.join(to_revoke)
        ret['result'] = None
        ret['comment'] = "Would have modified roles (grant: {0}; revoke: {1})".format(lsg, lsr)
        return ret

    ret['changes'][name] = changes = {}

    if to_grant:
        if not __salt__['mongodb.user_grant_roles'](
            name, to_grant, database, user=user, password=password, host=host, port=port, authdb=authdb
        ):
            ret['comment'] = "failed to grant some or all of {0} to {1} on {2}".format(to_grant, name, database)
            return ret
        else:
            changes['granted'] = list(to_grant)

    if to_revoke:
        if not __salt__['mongodb.user_revoke_roles'](
            name, to_revoke, database, user=user, password=password, host=host, port=port, authdb=authdb
        ):
            ret['comment'] = "failed to revoke some or all of {0} to {1} on {2}".format(to_revoke, name, database)
            return ret
        else:
            changes['revoked'] = list(to_revoke)

    ret['result'] = True
    return ret


def _parse_roles_dict(roles):
    '''
    Parse roles dict from pillar
    '''
    ret = []
    for db, rlist in roles.items():
        for role in rlist:
            ret.append({"role": role, "db": db})
    return ret


def internal_user_ensure_roles(name, roles, user=None, password=None, host="localhost", port=27017, authdb=None):
    '''
    Ensure that the named user has the given roles and no other roles

    name
        The name of the user to grant/revoke roles to

    roles
        The roles the given user should have

    user
        MongoDB user with sufficient privilege to create the user

    password
        Password for the admin user specified by the ``user`` parameter

    host
        The hostname/IP address of the MongoDB server

    port
        The port on which MongoDB is listening

    authdb
        The database in which to authenticate
    '''

    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    def _roles_to_set_internal(roles):
        ret = set()
        for r in roles:
            # if isinstance(r, dict):
            ret.add(json.dumps({"role": r['role'], "db": r['db']}))
        return ret

    def _user_roles_to_set_internal(user_list, name):
        ret = set()

        for item in user_list:
            if item['user'] == name:
                ret = ret.union(_roles_to_set_internal(item['roles']))
        return ret

    database = 'admin'

    if not isinstance(roles, (dict)):
        ret['result'] = False
        ret['comment'] = 'Roles ({0}) is not dict.'.format(roles)
        return ret

    if not roles:
        ret['result'] = True
        ret['comment'] = "nothing to do (no roles given)"
        return ret

    # Check for valid port
    try:
        port = int(port)
    except TypeError:
        ret['result'] = False
        ret['comment'] = 'Port ({0}) is not an integer.'.format(port)
        return ret

    user_list = __salt__['mongodb.user_list'](
        database=database, user=user, password=password, host=host, port=port, authdb=authdb
    )

    user_set = _user_roles_to_set_internal(user_list, name)
    roles_set = _roles_to_set_internal(_parse_roles_dict(roles))
    to_grant = list(roles_set - user_set)
    to_revoke = list(user_set - roles_set)

    if not to_grant and not to_revoke:
        ret['result'] = True
        ret['comment'] = "User {0} has the appropriate roles on {1}".format(name, database)
        return ret

    if __opts__['test']:
        lsg = ', '.join(to_grant)
        lsr = ', '.join(to_revoke)
        ret['result'] = None
        ret['comment'] = "Would have modified roles (grant: {0}; revoke: {1})".format(lsg, lsr)
        return ret

    ret['changes'][name] = changes = {}

    if to_grant:
        if not __salt__['mongodb.user_grant_roles'](
            name,
            list(map(lambda x: json.loads(x), to_grant)),
            database,
            user=user,
            password=password,
            host=host,
            port=port,
            authdb=authdb,
        ):
            ret['comment'] = "failed to grant some or all of {0} to {1} on {2}".format(to_grant, name, database)
            return ret
        else:
            changes['granted'] = list(to_grant)

    if to_revoke:
        if not __salt__['mongodb.user_revoke_roles'](
            name,
            list(map(lambda x: json.loads(x), to_revoke)),
            database,
            user=user,
            password=password,
            host=host,
            port=port,
            authdb=authdb,
        ):
            ret['comment'] = "failed to revoke some or all of {0} to {1} on {2}".format(to_revoke, name, database)
            return ret
        else:
            changes['revoked'] = list(to_revoke)

    ret['result'] = True
    return ret


def replset_add(name, arbiter=None, force=None, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Add member to replicaset.

    name
        The hostport to add

    arbiter
        Is new member is arbiter

    force
        Force replset update

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    host
        The host to connect to

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    #  TODO: check rs is configured

    if __opts__['test']:
        ret['comment'] = 'Hostport needs to be added to replicaset: {name}'.format(name=name)
        ret['result'] = None
        return ret

    add_result = __salt__['mongodb.replset_add'](name, arbiter, force, user, password, host, port, authdb)
    if add_result is True:
        ret['comment'] = 'Hostport {0} has been added to replicaset'.format(name)
        ret['result'] = True
        return ret

    ret['comment'] = add_result
    return ret


def replset_remove(name, force=None, user=None, password=None, host=None, port=None, authdb=None):
    """
    Remove member from replicaset

    name
        The hostport to add

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    host
        The host to connect to

    port
        The port to connect to

    authdb
        The database in which to authenticate
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    if __opts__['test']:
        ret['comment'] = 'Hostport needs to be removed from replicaset: {name}'.format(name=name)
        ret['result'] = None
        return ret

    primary = __salt__['mongodb.find_rs_primary'](None, None, user, password, host, port, authdb)

    if primary is None:
        ret['comment'] = 'Can not find PRIMARY of replicaset'
        ret['result'] = False
        return ret

    log.debug('Primary is "%s", we are "%s"', primary, host)
    if primary == host:
        remove_result = __salt__['mongodb.replset_remove'](name, force, user, password, host, port, authdb)
        ret['comment'] = 'Hostport {name} has been removed from replicaset'.format(name=name)
    else:
        remove_result = True
        ret['comment'] = 'Replica. Not changing replicaset configuration.'

    if remove_result is True:
        ret['result'] = True
        return ret

    ret['comment'] = remove_result
    return ret


def replset_remove_deleted(name, replset_hosts, user=None, password=None, host=None, port=None, authdb=None):
    """
    Remove deleted hosts from replica set
    """
    ret = {
        'name': name,
        'changes': {},
        'result': False,
        'comment': '',
    }
    delete_targets = []
    for rs_host in __salt__['mongodb.get_rs_members'](user=user, password=password, host=host, port=port):
        if rs_host not in replset_hosts:
            delete_targets.append(rs_host)

    if __opts__.get('test'):
        for target in delete_targets:
            ret['changes'][target] = 'remove'
        if ret['changes']:
            ret['result'] = None
        else:
            ret['result'] = True
        return ret

    for target in delete_targets:
        primary = __salt__['mongodb.find_rs_primary'](None, None, user, password, host, port, authdb)

        if primary is None:
            ret['comment'] = 'Can not find PRIMARY of replicaset'
            return ret

        if primary == target:
            stepdown_res = rs_step_down(name=target, port=port, password=password, user=user, authdb=authdb)
            if not stepdown_res.get('result'):
                ret['comment'] = 'Stepdown of {host} failed: {res}'.format(host=target, res=stepdown_res)
                return ret
        remove_res = replset_remove(
            host=host,
            port=port,
            password=password,
            user=user,
            authdb=authdb,
            name='{host}:{port}'.format(host=target, port=port),
        )
        if not remove_res.get('result'):
            ret['comment'] = 'Removal of {host} failed: {res}'.format(host=target, res=remove_res)
            return ret
        if remove_res.get('changes'):
            ret['changes'][target] = 'remove'

    ret['result'] = True
    return ret


def replset_initiate(name, user=None, password=None, port=None, authdb=None):
    '''
    Initiate replicaset

    name
        The host to initiate on

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    #  TODO: check rs is configured

    init_result = __salt__['mongodb.replset_initiate'](user, password, name, port, authdb)
    if init_result is True:
        ret['comment'] = 'ReplicaSet has been initiated at {0}:{1}'.format(name, port)
        ret['result'] = True
        return ret

    ret['comment'] = init_result
    return ret


def alive(name, tries=None, timeout=None, user=None, password=None, port=None, authdb=None):
    '''
    Check mongodb alive

    name
        The host to check

    tries
        TODO

    timeout
        TODO

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    is_alive = __salt__['mongodb.is_alive'](tries, timeout, user, password, name, port, authdb)
    if not is_alive:
        ret['comment'] = 'MongoDB at {0}:{1} seems dead'.format(name, port)
    ret['result'] = is_alive
    return ret


def rs_host_role_wait(name, role=None, wait_secs=None, conn_timeout=None, port=None):
    '''
    Check mongodb role

    name
        The host to check

    role
        Desired member state

    wait_secs
        Wait role till given time

    conn_timeout
        pymongo timeouts

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {
        'name': name,
        'changes': {},
        'comment': '',
        'result': False,
    }

    dry_run = __opts__.get('test')
    current_role = __salt__['mongodb.get_rs_host_role'](
        wait_secs=wait_secs, conn_timeout=conn_timeout, host=name, port=port
    )

    if current_role is None:
        ret['comment'] = 'Failed to get state'
        return ret

    if current_role == role:
        ret['comment'] = 'Node is {0}'.format(role)
        ret['result'] = True
        return ret

    if dry_run:
        ret['comment'] = 'Node is {0}, desired state is {1}'.format(current_role, role)
        ret['result'] = None
        return ret

    success = __salt__['mongodb.wait_for_rs_host_role'](
        role=role, wait_secs=wait_secs, conn_timeout=conn_timeout, host=name, port=port
    )

    if not success:
        ret['comment'] = 'Node is {0}, desired state is {1}'.format(current_role, role)
        return ret

    ret['comment'] = 'Node changed state from {0} to {1}'.format(current_role, role)
    ret['result'] = success
    return ret


def wait_for_rs_joined(name, deadline=None, timeout=None, user=None, password=None, port=None, authdb=None):
    '''
    Wait for replicaset join

    name
        The host to check

    deadline
        Wait join for a given time

    timeout
        pymongo timeouts

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {
        'name': name,
        'changes': {},
        'comment': '',
        'result': False,
    }

    current_role = __salt__['mongodb.get_rs_host_role'](timeout, user, password, name, port, authdb)

    if current_role is None:
        ret['comment'] = 'Failed to get state'
        ret['result'] = False
        return ret

    if __opts__['test']:
        if current_role in ['PRIMARY', 'SECONDARY']:
            ret['comment'] = 'Node has already joined replicaset'
            ret['result'] = True
        else:
            ret['comment'] = 'Node is {0}, need to wait for join'.format(current_role)
            ret['result'] = None

        return ret

    success = __salt__['mongodb.wait_for_rs_joined'](deadline, timeout, user, password, name, port, authdb)

    if not success:
        ret['comment'] = 'Node has not joined replicaset yet'
    ret['result'] = success
    return ret


def rs_step_down(
    name,
    step_down_secs=None,
    secondary_catch_up_period_secs=None,
    ensure_new_primary=True,
    timeout=None,
    user=None,
    password=None,
    port=None,
    check_on_host=None,
    authdb=None,
):
    '''
    Perform rs.stepDown

    name
        The host to step down

    step_down_secs
        The number of seconds to step down the primary, during which time
        the stepdown member is ineligible for becoming primary

    secondary_catch_up_period_secs
        The number of seconds that the mongod will wait for an electable
        secondary to catch up to the primary

    timeout
        pymongo timeouts

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''

    stepdown_host = name
    if check_on_host is None:
        check_on_host = 'localhost'

    ret = {'name': stepdown_host, 'changes': {}, 'result': False, 'comment': ''}

    primary = __salt__['mongodb.find_rs_primary'](
        secondary_catch_up_period_secs=secondary_catch_up_period_secs,
        timeout=timeout,
        user=user,
        password=password,
        host=check_on_host,
        port=port,
        authdb=authdb,
        host_only=True,
    )

    if primary is None:
        ret['comment'] = 'Can not find PRIMARY of replicaset'
        ret['result'] = False
        return ret

    if primary != stepdown_host:
        ret['comment'] = '"{0}" is not PRIMARY, no need to step down'.format(stepdown_host)
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['comment'] = 'MongoDB is PRIMARY, need to step down'
        ret['result'] = None
        return ret

    rs_step_down_cmd = __salt__['mongodb.rs_step_down'](
        step_down_secs=step_down_secs,
        secondary_catch_up_period_secs=secondary_catch_up_period_secs,
        ensure_new_primary=ensure_new_primary,
        timeout=timeout,
        user=user,
        password=password,
        host=stepdown_host,
        port=port,
        check_on_host=check_on_host,
    )

    ret['result'] = rs_step_down_cmd

    ret['comment'] = 'MongoDB has stepped down unsuccessfully' if not rs_step_down_cmd else 'MongoDB has stepped down'
    return ret


def shard_absent(name, user=None, password=None, authdb=None, host=None, port=None):
    """
    Ensure that name does not exist in cluster
    1. Get a list of shards exist.
    2. If name is not found, add it
    """
    opts = dict(user=user, password=password, host=host, port=port)
    list_shard_ids = partial(__salt__['mongodb.list_shard_ids'], **opts)
    remove_shard = partial(__salt__['mongodb.shard_remove'], name, **opts)
    dry_run = __opts__.get('test')

    name = name.split('/')[0]
    ret = {
        'name': name,
        'changes': {},
        'result': None,
        'comment': '',
    }

    if name not in list_shard_ids():
        ret['result'] = True
        ret['comment'] = 'Shard id "{0}" is not present'.format(name)
        return ret

    if dry_run:
        ret['result'] = None
        ret['comment'] = 'Shard id "{0}" is present and needs to be deleted'.format(name)
        return ret

    try:
        result = remove_shard()
        if result:
            ret['result'] = True
            ret['changes'][name] = 'Absent'
            ret['comment'] = 'Shard "{0}" was removed successfully'.format(name)
            return ret
    except Exception as exc:
        ret['comment'] = 'Failed to remove shard "{0}": {1}'.format(name, exc)

    ret['result'] = False
    return ret


def shard_present(name, url, user=None, password=None, authdb=None, host=None, port=None):
    """
    Ensure that name exists in cluster
    1. Get a list of shards exist.
    2. If name is not found, add it
    """
    opts = dict(user=user, password=password, host=host, port=port)
    list_shard_ids = partial(__salt__['mongodb.list_shard_ids'], **opts)
    add_shard = partial(__salt__['mongodb.add_shard'], name, url, **opts)
    dry_run = __opts__.get('test')

    name = name.split('/')[0]
    ret = {
        'name': name,
        'changes': {},
        'result': None,
        'comment': '',
    }

    if name in list_shard_ids():
        ret['result'] = True
        ret['comment'] = 'Shard id "{0}" is already present'.format(name)
        return ret

    if dry_run:
        ret['result'] = None
        ret['comment'] = 'Shard id "{0}" is not present and needs to be created'.format(name)
        return ret

    try:
        result = add_shard()
        if result:
            ret['result'] = True
            ret['changes'][name] = 'Present'
            ret['comment'] = 'Shard "{0}" was added successfully'.format(name)
            return ret
    except Exception as exc:
        ret['comment'] = 'Failed to add shard "{0}": {1}'.format(name, exc)

    ret['result'] = False
    return ret


def host_wasnt_initiated(name, user=None, password=None, port=None, authdb=None, strict=None):
    '''
    Check if one of hosts was initiated

    name
        Hostlist to check

    user
        The user to connect as (must be able to manage replset)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate

    strict
        Strict mode, fail if check is unsuccessful

    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    log.debug('check hosts: %s', name)
    hosts = name.split(',')
    is_initiated = __salt__['mongodb.any_host_initiated'](
        hosts=hosts, port=port, user=user, password=password, authdb=authdb, strict=strict
    )
    if is_initiated is True:
        ret['comment'] = 'At least one of hosts has been initiated: {0} at port {1}'.format(name, port)
        return ret

    ret['result'] = True
    ret['comment'] = is_initiated
    return ret


def shard_is_not_primary(name, user=None, password=None, authdb=None, host=None, port=None):
    '''
    Check if shard is not primary for any database

    name
        Shard name to check

    user
        The user to connect as (must be able to manage sharding)

    password
        The password of the user

    host
        The host to connect to

    port
        The port to connect to

    authdb
        The database in which to authenticate

    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    shard_dbs = __salt__['mongodb.get_shards_primary_dbs'](
        host=host, port=port, user=user, password=password, authdb=authdb
    )

    if name in shard_dbs:
        ret['comment'] = 'Shard {0} is primary for databases: {1}'.format(name, ','.join(shard_dbs[name]))
        return ret

    ret['result'] = True
    ret['comment'] = 'Shard {0} is not primary for any database'.format(name)
    return ret


def check_balancer_enabled(name, user=None, password=None, authdb=None, host=None, port=None):
    '''
    Check if balancer is enabled

    name
        state name

    user
        The user to connect as (must be able to manage sharding)

    password
        The password of the user

    host
        The host to connect to

    port
        The port to connect to

    authdb
        The database in which to authenticate

    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    balancer_state = __salt__['mongodb.get_balancer_state'](
        host=host, port=port, user=user, password=password, authdb=authdb
    )

    ret['result'] = balancer_state
    ret['comment'] = 'Balancer is enabled: {}'.format(balancer_state)
    return ret


def oplog_maxsize(name, max_size, user=None, password=None, port=None, authdb=None):
    '''
    https://docs.mongodb.com/v3.6/tutorial/change-oplog-size/
    Set maximum oplog size (in megabytes)

    name
        The host to change oplog size for

    max_size
        New oplog max size in megabytes

    user
        The user to connect as (must be able to modify the local database [for example clusterManager or clusterAdmin role].)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    current_oplog_max_size = int(
        __salt__['mongodb.get_oplog_maxsize'](host=name, port=port, user=user, password=password, authdb=authdb)
    )

    if current_oplog_max_size == int(max_size):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = '{0} max oplog size would be changed to {1}'.format(name, max_size)
        ret['changes'] = {'db.oplog.rs.stats().maxSize': {'old': current_oplog_max_size, 'new': max_size}}
        return ret

    result = __salt__['mongodb.set_oplog_maxsize'](
        host=name, port=port, user=user, password=password, authdb=authdb, max_size=max_size
    )
    if result:
        ret['result'] = True
        ret['changes'] = {'db.oplog.rs.stats().maxSize': {'old': current_oplog_max_size, 'new': max_size}}
    else:
        ret['comment'] = 'Something went wrong, {0} max oplog size wasn\'t  changed to {1}'.format(name, max_size)

    return ret


def feature_compatibility_version(name, fcv, user=None, password=None, port=None, authdb=None):
    '''
    https://docs.mongodb.com/manual/reference/command/setFeatureCompatibilityVersion
    Set featureCompatibilityVersion

    name
        The host to change fcv for

    fcv
        featureCompatibilityVersion

    user
        The user to connect as (must be able to modify the local database [for example clusterManager or clusterAdmin role].)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    dry_run = __opts__.get('test')

    current_fcv = __salt__['mongodb.get_feature_compatibility_version'](
        host=name, port=port, user=user, password=password, authdb=authdb
    )

    if current_fcv == fcv:
        ret['comment'] = 'FeatureCompatibilityVersion is already "{}"'.format(fcv)
        ret['result'] = True
        return ret

    ret['changes'] = {'featureCompatibilityVersion.version': {'old': current_fcv, 'new': fcv}}

    if dry_run:
        ret['result'] = None
        ret['comment'] = 'featureCompatibilityVersion would be changed to "{0}" from "{1}"'.format(fcv, current_fcv)
        return ret

    result = __salt__['mongodb.set_feature_compatibility_version'](
        fcv=fcv, host=name, port=port, user=user, password=password, authdb=authdb
    )

    if result:
        ret['result'] = True
    else:
        ret['comment'] = 'Something went wrong, fcv was not changed'
        ret['changes'] = {}

    return ret


def free_monitoring(name, status, user=None, password=None, port=None, authdb=None):
    '''
    https://docs.mongodb.com/manual/reference/method/db.disableFreeMonitoring/#db.disableFreeMonitoring
    Set FreeMonitoring status

    name
        The host to change status for

    status
        enabled or disabled

    user
        The user to connect as (must be able to modify the local database
         [for example clusterManager or clusterAdmin role].)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''

    cmd_state_map = {
        'enabled': 'enable',
        'disabled': 'disable',
    }

    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    # assert is not convenient for tests
    if status != 'disabled':
        ret['comment'] = 'Only "disabled" value of status is allowed'
        ret['result'] = False
        return ret

    dry_run = __opts__.get('test')

    version_array = __salt__['mongodb.version_array'](host=name, port=port, user=user, password=password, authdb=authdb)

    if int(version_array[0]) < 4:
        ret['comment'] = 'Current version does not support free monitoring'
        ret['result'] = True
        return ret

    current_status = __salt__['mongodb.get_free_monitoring_state'](
        host=name, port=port, user=user, password=password, authdb=authdb
    )

    if current_status == status:
        ret['comment'] = 'FreeMonitoring status is already "{}"'.format(status)
        ret['result'] = True
        return ret

    ret['changes'] = {'FreeMonitoring status': {'old': current_status, 'new': status}}

    if dry_run:
        ret['result'] = None
        ret['comment'] = 'FreeMonitoring status would be changed to "{0}" from "{1}"'.format(status, current_status)
        return ret

    result = __salt__['mongodb.set_free_monitoring_state'](
        action=cmd_state_map[status], host=name, port=port, user=user, password=password, authdb=authdb
    )

    if result:
        ret['result'] = True
    else:
        ret['comment'] = 'Something went wrong, FreeMonitoring status was not changed'
        ret['changes'] = {}

    return ret


def electable_secondaries_freezed(name, freeze_seconds, user=None, password=None, port=None, authdb=None):
    '''
    https://docs.mongodb.com/manual/reference/method/db.disableFreeMonitoring/#db.disableFreeMonitoring
    Set freeze time for electable secondaries of replset

    name
        The host to retrieve replset config

    freeze_seconds
        The duration the member is ineligible to become primary.

    user
        The user to connect as (must be able to modify the local database
         [for example clusterManager or clusterAdmin role].)

    password
        The password of the user

    port
        The port to connect to

    authdb
        The database in which to authenticate
    '''

    host = name
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    dry_run = __opts__.get('test')

    primary = __salt__['mongodb.find_rs_primary'](None, None, user, password, host, port, authdb)

    if primary is None:
        ret['comment'] = 'Can not find PRIMARY of replicaset'
        return ret

    log.debug('Primary is "%s", we are "%s"', primary, host)
    if primary != host:
        ret['comment'] = 'Current host status is not PRIMARY'
        ret['result'] = True
        return ret

    if dry_run:
        ret['result'] = None
        ret['comment'] = 'Electable secondaries will be freezed for {0} seconds'.format(freeze_seconds)
        return ret

    result = __salt__['mongodb.freeze_rs_electable_secondaries'](
        freeze_seconds=int(freeze_seconds), host=host, port=port, user=user, password=password, authdb=authdb
    )

    if result:
        ret['result'] = True
        ret['comment'] = 'Electable secondaries are freezed for {0} seconds'.format(freeze_seconds)
    else:
        ret['comment'] = 'Something went wrong, can not freeze secondaries'

    return ret
