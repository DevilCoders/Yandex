# -*- coding: utf-8 -*-
'''
Module to provide MongoDB functionality to Salt

:configuration: This module uses PyMongo, and accepts configuration details as
    parameters as well as configuration settings::

        mongodb.host: 'localhost'
        mongodb.port: 27017
        mongodb.user: ''
        mongodb.password: ''

    This data can also be passed into pillar. Options passed into opts will
    overwrite options passed into pillar.
'''
from __future__ import absolute_import, print_function, unicode_literals

# Import python libs
import logging
import os.path
import re
import subprocess
import time

# Import salt libs
import json
from collections import defaultdict
from salt.exceptions import get_error_message as _get_error_message


# Import third party libs
from salt.ext import six

try:
    import pymongo
    import bson
    import ssl

    HAS_MONGODB = True
except ImportError:
    HAS_MONGODB = False

try:
    from salt.utils.stringutils import to_str
except ImportError:
    # salt library is not available in tests
    from cloud.mdb.salt_tests.common.arc_utils import to_str

log = logging.getLogger(__name__)

MEMBER_STR_STATES = {
    'STARTUP': 0,
    'PRIMARY': 1,
    'SECONDARY': 2,
    'RECOVERING': 3,
    'STARTUP2': 5,
    'UNKNOWN': 6,
    'ARBITER': 7,
    'DOWN': 8,
    'ROLLBACK': 9,
    'REMOVED': 10,
}

MEMBER_INT_STATES = dict((v, k) for k, v in MEMBER_STR_STATES.items())


MEMBER_ERROR_STATES = {
    'UNKNOWN': 6,
    'DOWN': 8,
    'ROLLBACK': 9,
    'REMOVED': 10,
}

ROLE_ATTRS_MAP = {
    'PRIMARY': {
        'ismaster': True,
        'secondary': False,
    },
    'SECONDARY': {
        'ismaster': False,
        'secondary': True,
    },
}

RS_CHANGE_WAIT_SECS = 10
SECONDARY_CATCH_UP_PERIOD_SECS = 10
DEFAULT_TIMEOUT_SECS = 5
DEFAULT_RETRIES_COUNT = 5

DEFAULT_RS_ADD_TIMEOUT = 120
DEFAULT_RS_ADD_RETRY_WAIT = 5

DEFAULT_SHARD_DRAIN_TIMEOUT = 10 * 60 * 60  # 10 hours
DEFAULT_DRAIN_RETRY_WAIT = 30

BALANCER_WINDOW_REGEX = re.compile('^[0-9]{2}:[0-9]{2}$')

RESETUP_ID_FILE = "/tmp/mdb-mongodb-resetup-id"
RESETUP_STARTED_FILE = "/tmp/mdb-mongodb-resetup-started"
MDB_MONGOD_RESETUP = "/usr/bin/mdb-mongod-resetup"
STEPDOWN_ID_FILE = "/tmp/mdb-mongodb-stepdown-id"

# For linters, salt will populate it in runtime
__opts__ = {}
__salt__ = {}
__pillar__ = {}


def __virtual__():
    '''
    Only load this module if pymongo is installed
    '''
    if not HAS_MONGODB:
        return (
            False,
            'The mongodb execution module cannot be loaded: the pymongo library is not available.',
        )

    return 'mongodb'


def _pillar(*args, **kwargs):
    return __salt__['pillar.get'](*args, **kwargs)


def _grains(*args, **kwargs):
    return __salt__['grains.get'](*args, **kwargs)


def _prepare_ssl_options(options, host=None):
    options['ssl'] = True
    if 'ssl_ca_certs' not in options:
        options['ssl_ca_certs'] = '/etc/mongodb/ssl/allCAs.pem'

    if host is None or host == 'localhost' or host == '127.0.0.1':
        options['ssl_cert_reqs'] = ssl.CERT_NONE

    try:
        with open(options['ssl_ca_certs']):
            # File is readable, do nothing
            pass
    except IOError:
        # If there is no CA.pem file or it isn't readable
        # Then just skip SSL validation
        # As we are connecting (in 99,(9)% of cases)
        # To local mongodb, there should not be any MItM
        options['ssl_cert_reqs'] = ssl.CERT_NONE
        options.pop('ssl_ca_certs', None)


def _connect(
    user=None,
    password=None,
    host=None,
    port=None,
    database=None,
    authdb=None,
    options=None,
    use_ssl=True,
):
    '''
    Returns a tuple of (user, host, port) with config, pillar, or default
    values assigned to missing values.
    '''
    if host is None:
        host = 'localhost'
    if port is None:
        port = 27018
    if database is None:
        database = 'admin'
    if authdb is None:
        authdb = database
    if options is None:
        options = {}

    if use_ssl:
        _prepare_ssl_options(options, host)

    if 'appname' not in options:
        options['appname'] = 'mdb_salt_module'

    try:
        log.debug('Trying to connect: %s:%s %s', host, port, options)
        conn = pymongo.MongoClient(host=host, port=int(port), **options)
        mdb = pymongo.database.Database(conn, database)
        if user and password:
            log.debug('Auth is requested. Trying to authenticate as %s@%s at %s:%s', user, authdb, host, port)
            mdb.authenticate(to_str(user), to_str(password), source=authdb)
    except pymongo.errors.PyMongoError as err:
        log.exception('Error connecting to database %s with error: %s', database, err)
        raise

    return conn


def get_timeout_options(timeout=None):
    if timeout is None:
        timeout = DEFAULT_TIMEOUT_SECS
    timeout_ms = timeout * 1000
    return dict((opt, timeout_ms) for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))


def _conn_is_alive(conn, dbname):
    try:
        if isinstance(conn, pymongo.MongoClient):
            return conn[dbname].command('ping')['ok'] == 1
    except pymongo.errors.PyMongoError:
        pass

    return False


def _exec_command(
    action,
    name,
    args=None,
    user=None,
    password=None,
    database=None,
    authdb='admin',
    host=None,
    port=None,
    conn_options=None,
):
    conn = _connect(user, password, host, port, authdb=authdb, options=conn_options)
    if database is None:
        # Salt can handle TypeError natively
        raise TypeError('database must be set')
    if args is None:
        args = {}
    mdb = pymongo.database.Database(conn, database)
    log.debug('exec admin command %s on %s: %s', action, database, name)
    # Check=True forces to throw an exception if something goes wrong.
    ret = mdb.command(action, name, check=True, **args)
    log.debug('execution returned: %s', ret)
    return True


def _to_dict(objects):
    '''
    Potentially interprets a string as JSON for usage with mongo
    '''
    try:
        if isinstance(objects, six.string_types):
            objects = json.loads(objects)
    except ValueError as err:
        log.error("Could not parse objects: %s", err)
        raise err

    return objects


def list_roles(role=None, user=None, password=None, host=None, port=None, authdb='admin'):
    """
    Get a list of custom roles.
    """
    conn = _connect(user, password, host, port, authdb=authdb)
    mdb = pymongo.database.Database(conn, 'admin')
    roles = list(mdb.system.roles.find({'role': role} if role else {}, {'_id': 0}))
    log.debug('roles found: %s', roles)
    if not roles:
        return []
    return roles


def update_role(
    name,
    roles=None,
    privileges=None,
    authenticationRestrictions=None,
    user=None,
    password=None,
    database=None,
    host=None,
    port=None,
    authdb='admin',
):
    """
    Updates role. Current semantics replaces the whole role with the spec provided.
    update is performed using `updateRole` admin command.
    """
    role_args = {
        'roles': roles or [],
        'privileges': privileges or [],
        'authenticationRestrictions': authenticationRestrictions or [],
    }
    return _exec_command(
        'updateRole',
        name,
        args=role_args,
        user=user,
        password=password,
        database=database,
        host=host,
        port=port,
        authdb=authdb,
    )


def create_role(
    name,
    roles=None,
    privileges=None,
    authenticationRestrictions=None,
    user=None,
    password=None,
    database=None,
    host=None,
    port=None,
    authdb='admin',
):
    """
    Creates custom role.
    """
    role_args = {
        'roles': roles or [],
        'privileges': privileges or [],
        'authenticationRestrictions': authenticationRestrictions or [],
    }
    return _exec_command(
        'createRole',
        name,
        args=role_args,
        user=user,
        password=password,
        database=database,
        host=host,
        port=port,
        authdb=authdb,
    )


def drop_role(name, user=None, password=None, database=None, host=None, port=None, authdb='admin'):
    """
    Removes specified role.
    """
    return _exec_command(
        'dropRole',
        name,
        user=user,
        password=password,
        database=database,
        host=host,
        port=port,
        authdb=authdb,
    )


def is_primary(user=None, password=None, host=None, port=None, authdb=None):
    """
    Check if given mongodb instance is primary
    """
    try:
        conn = _connect(user, password, host, port, authdb=authdb, options=_get_conn_opts_dict())
        doc = conn.admin.command('isMaster')
        log.debug('Checking host %s:%s: %s', host, port, doc)
        return doc.get('ismaster'), doc
    except pymongo.errors.PyMongoError as err:
        log.error(err)
        return None, err


def find_master(hosts, port=27018, timeout=60, host_only=True):
    """
    Find master within hosts
    (if hosts is not list we assume it is pillar path)
    """
    if isinstance(hosts, list):
        host_list = hosts
    else:
        host_list = __salt__['pillar.get'](hosts)

    if not host_list:
        return

    conn_options = _get_conn_opts_dict()
    deadline = time.time() + timeout
    while time.time() < deadline:
        for host in host_list:
            conn = _connect(host=host, port=port, options=conn_options)
            try:
                primary = conn.admin.command('isMaster').get('primary')
                if primary is not None:
                    return primary.split(':')[0] if host_only else primary
            except pymongo.errors.PyMongoError:
                pass
        time.sleep(1)


def get_conn_spec(rsname, hosts, port):
    hosts_port = ['{0}:{1}'.format(host, port) for host in sorted(hosts)]
    return '{rsname}/{hosts}'.format(rsname=rsname, hosts=','.join(hosts_port))


def get_cluster_item(role, shard_id=None):
    subclusters = __salt__['pillar.get']('data:dbaas:cluster:subclusters')
    ret = None
    roles = role
    if not isinstance(roles, list):
        roles = [role]
    for subname, sub in subclusters.items():
        for _role in roles:
            if _role in sub['roles']:
                _ret = sub['shards'][shard_id] if shard_id else sub
                if ret is not None:
                    ret['hosts'].update(_ret['hosts'])
                    ret['roles'] += _ret['roles']
                else:
                    ret = _ret
    if ret:
        return ret
    raise Exception('Can not retrieve cluster item')


def get_subcluster_conn_spec(role, port, rsname=None, default_rsname=None):
    subcluster = get_cluster_item(role)
    rsname = rsname if rsname else subcluster.get('name', default_rsname)
    return get_conn_spec(rsname, subcluster['hosts'], port)


def get_shard_conn_spec(role, port, shard_id, default_rsname=None):
    shard = get_cluster_item(role, shard_id)
    rsname = shard.get('name', default_rsname)
    return get_conn_spec(rsname, shard['hosts'], port)


def get_replset_config(conn):
    if conn.local.system.replset.count() > 1:
        raise RuntimeError('local.system.replset has unexpected contents')

    rs_conf = conn.local.system.replset.find_one()
    if not rs_conf:
        raise RuntimeError('No config object retrievable from local.system.replset')
    return rs_conf


def find_rs_primary(
    secondary_catch_up_period_secs=None,
    timeout=None,
    user=None,
    password=None,
    host=None,
    port=None,
    authdb='admin',
    host_only=True,
):
    """
    Find primary within replicaset
    """

    if timeout is None:
        timeout = DEFAULT_TIMEOUT_SECS
    timeout_ms = timeout * 1000

    if secondary_catch_up_period_secs is None:
        secondary_catch_up_period_secs = SECONDARY_CATCH_UP_PERIOD_SECS

    options = dict((opt, timeout_ms) for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))
    try:
        conn = _connect(user, password, host, port, options=options)
        server_selection_timeout = conn.server_selection_timeout
        heartbeat_timeout_secs = conn.admin.command({'replSetGetConfig': 1})['config']['settings'][
            'heartbeatTimeoutSecs'
        ]
    except pymongo.errors.PyMongoError as exc:
        log.error('Can not get replSetGetConfig: %s', exc, exc_info=True)
        return None

    # https://docs.mongodb.com/manual/reference/method/rs.stepDown/#behavior
    deadline = time.time() + secondary_catch_up_period_secs + server_selection_timeout + heartbeat_timeout_secs

    while time.time() < deadline:
        try:
            if not _conn_is_alive(conn, 'admin'):
                conn = _connect(user, password, host, port, authdb, options=options)
            replset_members = conn.admin.command('replSetGetStatus')['members']
            primaries = [node for node in replset_members if node['stateStr'] == 'PRIMARY']
            if len(primaries) == 1:
                primary = primaries.pop()
                logging.debug('Primary was found: %s', primary)
                return primary['name'].split(':')[0] if host_only else primary['name']
            elif len(primaries) > 1:
                raise RuntimeError('Several primaries were found: {0}'.format(', '.join(primaries)))
            log.debug('Waiting for primary')
        except Exception as err:
            log.warning('Failed while waiting for primary with error: %s', err, exc_info=True)

        time.sleep(timeout)
    log.critical('Primary was not found')


def get_rs_members(user=None, password=None, host=None, port=None, authdb=None):
    """
    Get list of replicaSet members
    """
    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        status = conn.admin.command('replSetGetStatus')
        return [x['name'].split(':')[0] for x in status['members']]
    except pymongo.errors.PyMongoError as err:
        log.error(err)
        return []


def db_list(user=None, password=None, host=None, port=None, authdb=None):
    '''
    List all Mongodb databases

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.db_list <user> <password> <host> <port>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    log.info('Listing databases')
    return conn.database_names()


def db_exists(name, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Checks if a database exists in Mongodb

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.db_exists <name> <user> <password> <host> <port>
    '''
    dbs = db_list(user, password, host, port, authdb=authdb)

    if isinstance(dbs, six.string_types):
        return False

    return name in dbs


def db_remove(name, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Remove a Mongodb database

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.db_remove <name> <user> <password> <host> <port>
    '''
    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        log.info('Removing database %s', name)
        conn.drop_database(name)
    except pymongo.errors.PyMongoError as err:
        log.error('Removing database %s failed with error: %s', name, err)
        return False

    return True


def _version(mdb):
    return mdb.command('buildInfo')['version']


def version(user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Get MongoDB instance version

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.version <user> <password> <host> <port> <database>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    if not conn:
        err_msg = "Failed to connect to MongoDB database {0}:{1}".format(host, port)
        log.error(err_msg)
        return (False, err_msg)

    try:
        mdb = pymongo.database.Database(conn, database)
        return _version(mdb)
    except pymongo.errors.PyMongoError as err:
        log.error('Retrieving users failed with error: %s', err)
        raise


def version_array(user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Get MongoDB instance version_array

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.version_array <user> <password> <host> <port> <database>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    if not conn:
        err_msg = "Failed to connect to MongoDB database {0}:{1}".format(host, port)
        log.error(err_msg)
        return False, err_msg

    try:
        mdb = pymongo.database.Database(conn, database)
        return mdb.command('buildInfo')['versionArray']
    except pymongo.errors.PyMongoError as err:
        log.error('Retrieving users failed with error: %s', err)
        raise


def user_find(name, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Get single user from MongoDB

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_find <name> <user> <password> <host> <port> <database> <authdb>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    if not conn:
        err_msg = "Failed to connect to MongoDB database {0}:{1}".format(host, port)
        log.error(err_msg)
        return (False, err_msg)

    mdb = pymongo.database.Database(conn, database)
    try:
        return mdb.command("usersInfo", name)["users"]
    except pymongo.errors.PyMongoError as err:
        log.error('Listing users failed with error: %s', err)
        return (False, six.text_type(err))


def user_auth(user=None, password=None, host=None, port=None, authdb=None, raise_exception=False):
    '''
    Get single user from MongoDB

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_auth <user> <password> <host> <port> <authdb>
    '''

    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        mdb = pymongo.database.Database(conn, authdb)
        result = (True, mdb.command("usersInfo", user)["users"])
        conn.close()
        return result
    except pymongo.errors.PyMongoError as err:
        if raise_exception:
            raise
        details = None
        if isinstance(err, pymongo.errors.OperationFailure):
            details = err.details

        log.error('Cant get user info: %s, %s', err.__class__.__name__, details)
        return (False, six.text_type(err))


def check_auth(user=None, password=None, host=None, port=None, authdb=None, raise_exception=False):
    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        conn.close()
        return (True, None)
    except pymongo.errors.PyMongoError as err:
        if raise_exception:
            raise
        details = None
        if isinstance(err, pymongo.errors.OperationFailure):
            details = err.details

        log.error('Cant auth: %s, %s', err.__class__.__name__, details)
        return (False, six.text_type(err))


def user_list(user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    List users of a Mongodb database

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_list <user> <password> <host> <port> <database>
    '''

    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        log.info('Listing users')
        mdb = pymongo.database.Database(conn, database)

        output = []

        for user in mdb.command('usersInfo')['users']:
            output.append({'user': user['user'], 'roles': user['roles']})
        return output

    except pymongo.errors.PyMongoError as err:
        log.error('Listing users failed with error: %s', err)
        raise


def list_users(user=None, password=None, host=None, port=None, authdb=None):
    '''
    List users of a Mongodb and return in raw format, i.e:
    > db.system.users.find().pretty().limit(1)
    {
        "_id" : "admin.admin",
        "user" : "admin",
        "db" : "admin",
        "credentials" : {
            "SCRAM-SHA-1" : {
                "iterationCount" : 10000,
                "salt" : "vDjc6Nz1cmt7ehwIl21dzQ==",
                "storedKey" : "Fg1+kXitrmJMpB7EncruyNJ6o0c=",
                "serverKey" : "AaRniIvEgeTIzJ0RBgX4kg7LQYA="
            },
            "SCRAM-SHA-256" : {
                "iterationCount" : 15000,
                "salt" : "DioTsZ5mMhHyAcqwdsz6Ne/Zp59JH6TM1GIOfg==",
                "storedKey" : "mGs3Mbqy+0GB+CLH1FeFsF3XEPgzorrpm7BlXMC3kyI=",
                "serverKey" : "Lkdu92HNAeI0tJ/o5TlN/Wdmd3IO8Zch70DNtZ+eE0k="
            }
        },
        "roles" : [
            {
                "role" : "dbOwner",
                "db" : "config"
            },
            {
                "role" : "dbOwner",
                "db" : "local"
            },
            {
                "role" : "dbOwner",
                "db" : "admin"
            },
            {
                "role" : "root",
                "db" : "admin"
            }
        ]
    }
    ...


    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.list_users <user> <password> <host> <port> <authdb>
    '''

    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        log.info('Listing all mongodb users')
        mdb = pymongo.database.Database(conn, authdb)

        return list(mdb.system.users.find())
    except pymongo.errors.PyMongoError as err:
        log.error('Listing users failed with error: %s', err)
        raise


def user_exists(name, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Checks if a user exists in Mongodb

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_exists <name> <user> <password> <host> <port> <database>
    '''
    for user in user_list(user, password, host, port, database, authdb):
        if name == dict(user).get('user'):
            return True

    return False


def user_create(
    name, passwd, user=None, password=None, host=None, port=None, database='admin', authdb=None, roles=None
):
    '''
    Create a Mongodb user

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_create <user_name> <user_password> <roles> <user> <password> <host> <port> <database>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)

    if not roles:
        roles = []

    try:
        log.info('Creating user %s', name)
        mdb = pymongo.database.Database(conn, database)
        mdb.command('createUser', name, pwd=passwd, roles=roles)
    except pymongo.errors.PyMongoError as err:
        log.error('Creating user %s failed with error: %s', name, err)
        raise
    return True


def user_update(
    name, passwd=None, user=None, password=None, host=None, port=None, database='admin', authdb=None, roles=None
):
    '''
    Update a Mongodb user

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_updae <user_name> <user_password> <roles> <user> <password> <host> <port> <database>
    '''

    kwargs = {}
    if passwd is not None:
        kwargs['pwd'] = passwd
    if roles is not None:
        kwargs['roles'] = roles

    conn = _connect(user, password, host, port, authdb=authdb)

    try:
        log.info('Updating user %s', name)
        mdb = pymongo.database.Database(conn, database)
        mdb.command('updateUser', name, **kwargs)
    except pymongo.errors.PyMongoError as err:
        log.error('Updating user %s failed with error: %s', name, err)
        raise
    return True


def user_remove(name, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Remove a Mongodb user

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.user_remove <name> <user> <password> <host> <port> <database>
    '''
    try:
        conn = _connect(user, password, host, port)
        log.info('Removing user %s', name)
        mdb = pymongo.database.Database(conn, database)
        mdb.remove_user(name)
    except pymongo.errors.PyMongoError as err:
        log.error('Creating database %s failed with error: %s', name, err)
        raise

    return True


def get_roles(role=None, user=None, password=None, host=None, port=None):
    """
    Ensures a role with specified name exists and
    has properties described by called parameters.
    """
    con = _connect(user=user, password=password, host=host, port=port, authdb='admin')
    mdb = pymongo.database.Database(con, 'admin')
    return mdb.system.roles.find({'role': role})


def user_roles_exists(name, roles, database, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Checks if a user of a Mongodb database has specified roles

    CLI Examples:

    .. code-block:: bash

        salt '*' mongodb.user_roles_exists johndoe '["readWrite"]' dbname admin adminpwd localhost 27017

    .. code-block:: bash

        salt '*' mongodb.user_roles_exists johndoe '[{"role": "readWrite", "db": "dbname" }, {"role": "read", "db": "otherdb"}]' dbname admin adminpwd localhost 27017
    '''
    try:
        roles = _to_dict(roles)
    except Exception:
        raise ValueError('Roles provided in wrong format')

    users = user_list(user, password, host, port, database, authdb)

    for user in users:
        if name == dict(user).get('user'):
            for role in roles:
                # if the role was provided in the shortened form, we convert it to a long form
                if not isinstance(role, dict):
                    role = {'role': role, 'db': database}
                if role not in dict(user).get('roles', []):
                    return False
            return True

    return False


def user_grant_roles(name, roles, database, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Grant one or many roles to a Mongodb user

    CLI Examples:

    .. code-block:: bash

        salt '*' mongodb.user_grant_roles johndoe '["readWrite"]' dbname admin adminpwd localhost 27017

    .. code-block:: bash

        salt '*' mongodb.user_grant_roles janedoe '[{"role": "readWrite", "db": "dbname" }, {"role": "read", "db": "otherdb"}]' dbname admin adminpwd localhost 27017
    '''
    try:
        roles = _to_dict(roles)
    except Exception:
        raise ValueError('Roles provided in wrong format')

    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        log.info('Granting roles %s to user %s', roles, name)
        mdb = pymongo.database.Database(conn, database)
        mdb.command("grantRolesToUser", name, roles=roles)
    except pymongo.errors.PyMongoError as err:
        log.error('Granting roles %s to user %s failed with error: %s', roles, name, err)
        raise

    return True


def user_revoke_roles(name, roles, database, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Revoke one or many roles to a Mongodb user

    CLI Examples:

    .. code-block:: bash

        salt '*' mongodb.user_revoke_roles johndoe '["readWrite"]' dbname admin adminpwd localhost 27017

    .. code-block:: bash

        salt '*' mongodb.user_revoke_roles janedoe '[{"role": "readWrite", "db": "dbname" }, {"role": "read", "db": "otherdb"}]' dbname admin adminpwd localhost 27017
    '''
    try:
        roles = _to_dict(roles)
    except Exception:
        raise ValueError('Roles provided in wrong format')

    try:
        conn = _connect(user, password, host, port, authdb=authdb)
        log.info('Revoking roles %s from user %s', roles, name)
        mdb = pymongo.database.Database(conn, database)
        mdb.command("revokeRolesFromUser", name, roles=roles)
    except pymongo.errors.PyMongoError as err:
        log.error('Revoking roles %s from user %s failed with error: %s', roles, name, err)
        raise

    return True


def insert(objects, collection, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Insert an object or list of objects into a collection

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.insert '[{"foo": "FOO", "bar": "BAR"}, {"foo": "BAZ", "bar": "BAM"}]' mycollection <user> <password> <host> <port> <database>

    '''
    try:
        objects = _to_dict(objects)
    except Exception as err:
        raise ValueError('Objects provided in wrong format %s' % err)

    try:
        conn = _connect(user, password, host, port, database, authdb)
        log.info("Inserting %r into %s.%s", objects, database, collection)
        mdb = pymongo.database.Database(conn, database)
        col = getattr(mdb, collection)
        ids = col.insert(objects)
        return ids
    except pymongo.errors.PyMongoError as err:
        log.error("Inserting objects %r failed with error %s", objects, err)
        raise


def update_one(objects, collection, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Update an object into a collection
    http://api.mongodb.com/python/current/api/pymongo/collection.html#pymongo.collection.Collection.update_one

    .. versionadded:: 2016.11.0

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.update_one '{"_id": "my_minion"} {"bar": "BAR"}' mycollection <user> <password> <host> <port> <database>

    '''
    conn = _connect(user, password, host, port, database, authdb)

    objects = six.text_type(objects)
    objs = re.split(r'}\s+{', objects)

    if len(objs) != 2:
        return "Your request does not contain a valid " + "'{_\"id\": \"my_id\"} {\"my_doc\": \"my_val\"}'"

    objs[0] = objs[0] + '}'
    objs[1] = '{' + objs[1]

    document = []

    for obj in objs:
        obj = _to_dict(obj)
        document.append(obj)

    _id_field = document[0]
    _update_doc = document[1]

    # need a string to perform the test, so using objs[0]
    test_f = find(collection, objs[0], user, password, host, port, database, authdb)
    if not isinstance(test_f, list):
        raise ValueError('The find result is not well formatted. An error appears; cannot update.')
    elif len(test_f) < 1:
        raise ValueError('Did not find any result. You should try an insert before.')
    elif len(test_f) > 1:
        raise ValueError('Too many results. Please try to be more specific.')
    else:
        try:
            log.info("Updating %r into %s.%s", _id_field, database, collection)
            mdb = pymongo.database.Database(conn, database)
            col = getattr(mdb, collection)
            ids = col.update_one(_id_field, {'$set': _update_doc})
            nb_mod = ids.modified_count
            return "{0} objects updated".format(nb_mod)
        except pymongo.errors.PyMongoError as err:
            log.error('Updating object %s failed with error %s', objects, err)
            raise


def find(collection, query=None, user=None, password=None, host=None, port=None, database='admin', authdb=None):
    '''
    Find an object or list of objects in a collection

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.find mycollection '[{"foo": "FOO", "bar": "BAR"}]' <user> <password> <host> <port> <database>

    '''
    try:
        query = _to_dict(query)
    except Exception as err:
        raise ValueError('malformed query: %s' % err)

    try:
        conn = _connect(user, password, host, port, database, authdb)
        log.info("Searching for %r in %s", query, collection)
        mdb = pymongo.database.Database(conn, database)
        col = getattr(mdb, collection)
        ret = col.find(query)
        return list(ret)
    except pymongo.errors.PyMongoError as err:
        log.error("Searching objects failed with error: %s", err)
        raise


def remove(collection, query=None, user=None, password=None, host=None, port=None, database='admin', w=1, authdb=None):
    '''
    Remove an object or list of objects into a collection

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.remove mycollection '[{"foo": "FOO", "bar": "BAR"}, {"foo": "BAZ", "bar": "BAM"}]' <user> <password> <host> <port> <database>

    '''
    try:
        query = _to_dict(query)
    except Exception as err:
        raise ValueError('malformed query: %s' % err)

    try:
        conn = _connect(user, password, host, port, database, authdb)
        log.info("Removing %r from %s", query, collection)
        mdb = pymongo.database.Database(conn, database)
        col = getattr(mdb, collection)
        ret = col.remove(query, w=w)
        return "{0} objects removed".format(ret['n'])
    except pymongo.errors.PyMongoError as err:
        log.error("Removing objects failed with error: %s", _get_error_message(err))
        raise


def replset_add(
    hostport=None,
    arbiter=None,
    force=None,
    user=None,
    password=None,
    host=None,
    port=None,
    authdb=None,
    add_timeout=None,
    add_retry_wait=None,
):

    if add_timeout is None:
        add_timeout = DEFAULT_RS_ADD_TIMEOUT
    if add_retry_wait is None:
        add_retry_wait = DEFAULT_RS_ADD_RETRY_WAIT

    add_eta = time.time() + add_timeout
    retries_left = DEFAULT_RETRIES_COUNT

    if not isinstance(hostport, six.string_types):
        raise ValueError('Expected a host-and-port string, but got {0}'.format(json.loads(hostport)))

    conn = _connect(user, password, host, port, authdb)
    while add_eta > time.time():
        rs_conf = get_replset_config(conn)
        log.debug('replset config: %s', rs_conf)
        rs_conf['version'] += 1

        max_id = 0
        for rs_member in rs_conf['members']:
            if rs_member['_id'] > max_id:
                max_id = rs_member['_id']

        new_member_cfg = {'_id': max_id + 1, 'host': hostport}
        if arbiter is True:
            new_member_cfg['arbiteriterOnly'] = True

        if '_id' not in new_member_cfg:
            new_member_cfg['_id'] = max_id + 1

        try:
            rs_conf['members'].append(new_member_cfg)
            ret = conn.admin.command('replSetReconfig', rs_conf, force=force)
            log.debug('Node added: %s', ret)
            time.sleep(RS_CHANGE_WAIT_SECS)
            return ret['ok'] == 1 or ret

        except pymongo.errors.AutoReconnect:
            if not retries_left:
                raise
            conn = _connect(user, password, host, port, authdb=authdb)
            retries_left -= 1

        except pymongo.errors.PyMongoError as err:
            err_str = str(err)
            retried_errors = [
                'replSetReconfig is already in progress',
                'node is currently updating its configuration',
                'replSetReconfig because the node is currently updating',
                'field to member config during reconfig',
            ]
            if not [e for e in retried_errors if e in err_str]:
                log.error('Replicaset config update failed with error: %s', _get_error_message(err))
                raise

        time.sleep(add_retry_wait)

    raise RuntimeError('Host add timeout exceeded')


def replset_initiated(user=None, password=None, host=None, port=None, authdb=None, timeout=None):
    conn = _connect(user, password, host, port, authdb, options=get_timeout_options(timeout))
    doc = conn.admin.command('isMaster')
    if 'setName' in doc:
        return True
    if not doc.get('isreplicaset'):
        raise RuntimeError('Instance is running in standalone mode')
    return False


def any_host_initiated(hosts, port=None, user=None, password=None, authdb=None, timeout=None, strict=False):
    for host in hosts:
        is_initiated = None

        try:
            is_initiated = replset_initiated(host=host, port=port, timeout=timeout)
            log.debug('Host %s:%s was initiated or is running in standalone mode: %s', host, port, is_initiated)
            if is_initiated:
                return True
        except pymongo.errors.ServerSelectionTimeoutError as err:
            log.debug('Can not get replset status for %s:%s: %s', host, port, err)
            is_initiated = None

        except pymongo.errors.OperationFailure as err:
            log.error('Unexpected OperationFailure while checking %s:%s: %s', host, port, err)
            raise

        if is_initiated is None:
            log.error('Could not check host %s:%s', host, port)
            if strict:
                log.error('Assume hosts were initiated because of strict mode')
                return True

    return False


def replset_initiate(user=None, password=None, host=None, port=None, authdb=None):
    conn = _connect(user, password, host, port, authdb)

    try:
        conn.admin.command('replSetGetStatus')
    except pymongo.errors.PyMongoError as err:
        if 'no replset config has been received' not in str(err):
            return _get_error_message(err)

    try:
        ret = conn.admin.command('replSetInitiate', {})
        log.debug('Replicaset initiated: %s', ret)
        time.sleep(RS_CHANGE_WAIT_SECS)
        return ret['ok'] == 1 or ret
    except pymongo.errors.PyMongoError as err:
        log.error('Replicaset config update failed with error: %s', _get_error_message(err))
        return _get_error_message(err)


def replset_remove(hostport, force=None, user=None, password=None, host=None, port=None, authdb=None):
    if not isinstance(hostport, six.string_types):
        raise ValueError('Expected a host-and-port string of arbiter, but got {0}'.format(json.loads(hostport)))

    conn = _connect(user, password, host, port, authdb)

    rs_conf = get_replset_config(conn)
    log.debug('replset config: %s', rs_conf)
    rs_conf['version'] += 1

    try:
        for i, rs_member in enumerate(rs_conf['members']):
            log.debug('%s', rs_member)
            if rs_member['host'] == hostport:
                rs_conf['members'].pop(i)
                break

        log.debug('Applying rs_conf: %s', rs_conf)
        ret = conn.admin.command('replSetReconfig', rs_conf, force=force)
        log.debug('rs_conf apply result: %s', ret)
        return ret['ok'] == 1 or ret
    except pymongo.errors.PyMongoError as err:
        log.error('Replicaset config update failed with error: %s', _get_error_message(err))
        raise


def is_alive(tries=None, timeout=None, user=None, password=None, host=None, port=None, authdb=None):
    if tries is None:
        tries = 1

    if timeout is None:
        timeout = 5
    timeout_ms = timeout * 1000

    if authdb is None:
        authdb = 'admin'

    options = dict((opt, timeout_ms) for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))
    for _ in range(tries):
        conn = _connect(user, password, host, port, authdb, options=options)
        ts = time.time()
        try:
            return conn[authdb].command('ping')['ok'] == 1
        except pymongo.errors.PyMongoError as err:
            log.warning('ping command failed with error: %s', _get_error_message(err))
            sleep_seconds = timeout - (time.time() - ts)
            if sleep_seconds > 0:
                time.sleep(sleep_seconds)
    return False


def rs_step_down(
    step_down_secs=None,
    secondary_catch_up_period_secs=None,
    ensure_new_primary=True,
    timeout=None,
    user=None,
    password=None,
    host=None,
    port=None,
    check_on_host=None,
    authdb='admin',
):
    if step_down_secs is None:
        step_down_secs = 60
    if secondary_catch_up_period_secs is None:
        secondary_catch_up_period_secs = SECONDARY_CATCH_UP_PERIOD_SECS

    conn_options = _get_conn_opts_dict(timeout)
    try:
        cmd = bson.son.SON(
            [('replSetStepDown', step_down_secs), ('secondaryCatchUpPeriodSecs', secondary_catch_up_period_secs)]
        )
        _exec_command(
            action=cmd,
            name=None,
            user=user,
            password=password,
            host=host,
            port=port,
            database='admin',
            conn_options=conn_options,
        )
    except pymongo.errors.AutoReconnect:
        pass
    # TODO: refactor + use pymongo 3.7 exceptions
    # http://api.mongodb.com/python/current/changelog.html#changes-in-version-3-7-0
    except pymongo.errors.OperationFailure as err:
        err_str = str(err)
        if 'not primary so can\'t step down' in err_str:
            log.debug('Not a primary, will not stepdown')
        elif 'Our replica set config is invalid or we are not a member of it' in err_str:
            log.debug('Not a member of replicaset, will not stepdown')
        else:
            log.critical('Failed stepping down with error: %s', str(err))
            return False

    if not ensure_new_primary:
        rs_freeze_instance(
            freeze_secs=step_down_secs, user=user, password=password, host=host, port=port, authdb='admin'
        )
        return True

    primary_host_port = find_rs_primary(
        secondary_catch_up_period_secs=secondary_catch_up_period_secs,
        timeout=timeout,
        user=user,
        password=password,
        host=check_on_host,
        port=port,
        authdb=authdb,
        host_only=False,
    )

    log.debug('Current PRIMARY is %s', primary_host_port)
    host_port = '{}:{}'.format(host, port)

    return host_port != primary_host_port and primary_host_port is not None


def _get_conn_opts_dict(timeout=None):
    if timeout is None:
        timeout = DEFAULT_TIMEOUT_SECS
    timeout_ms = timeout * 1000
    return dict((opt, timeout_ms) for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))


def rs_freeze_instance(freeze_secs, timeout=None, user=None, password=None, host=None, port=None, authdb='admin'):
    conn_options = _get_conn_opts_dict(timeout)

    try:
        cmd = bson.son.SON([('replSetFreeze', freeze_secs)])
        _exec_command(
            action=cmd,
            name=None,
            user=user,
            password=password,
            host=host,
            port=port,
            database='admin',
            conn_options=conn_options,
        )
        return True
    except pymongo.errors.OperationFailure as err:
        log.critical('Failed freezing with error: %s', str(err))
        return False


def get_rs_host_role(wait_secs=None, conn_timeout=None, host=None, port=None, authdb='admin'):
    if wait_secs is None:
        wait_secs = 20

    conn_options = get_timeout_options(conn_timeout)
    conn = None
    deadline = int(time.time()) + wait_secs
    while time.time() < deadline:
        try:
            if not _conn_is_alive(conn, authdb):
                conn = _connect(host=host, port=port, authdb=authdb, options=conn_options)

            report = conn.admin.command('isMaster')
            for role, attrs in ROLE_ATTRS_MAP.items():
                if all(report[k] == attrs[k] for k in attrs):
                    return role

        except Exception as err:
            log.warning('Failed while waiting for role with error:  %s', err, exc_info=True)

        time.sleep(5)
    return False


def wait_for_rs_host_role(role, wait_secs=None, conn_timeout=None, host=None, port=None, authdb='admin'):
    if wait_secs is None:
        wait_secs = 20

    role_attrs = ROLE_ATTRS_MAP[role]
    conn_options = get_timeout_options(conn_timeout)
    conn = None
    deadline = int(time.time()) + wait_secs
    while time.time() < deadline:
        try:
            if not _conn_is_alive(conn, authdb):
                conn = _connect(host=host, port=port, authdb=authdb, options=conn_options)

            report = conn.admin.command('isMaster')
            if all(report[k] == role_attrs[k] for k in role_attrs):
                return True

            log.debug('Waiting for become: %s', role)
        except Exception as err:
            log.warning('Failed while waiting for role with error:  %s', err, exc_info=True)

        time.sleep(5)
    return False


def freeze_rs_electable_secondaries(
    freeze_seconds, strict=False, timeout=None, user=None, password=None, host=None, port=None, authdb='admin'
):
    """
    TODO
    """
    options = _get_conn_opts_dict(timeout)

    try:

        conn = _connect(user, password, host, port, authdb, options=options)
        rs_conf = get_replset_config(conn)
        log.debug('replset config: %s', rs_conf)
        rs_conf_members = rs_conf['members']
        rs_status_members = conn.admin.command('replSetGetStatus')['members']
    except Exception as err:
        log.warning('Failed to get replSetGetStatus: %s', err, exc_info=True)
        raise

    rs_el_secondaries = [m['name'] for m in rs_status_members if m['stateStr'] != 'PRIMARY']

    for node in rs_conf_members:
        try:
            if node['host'] not in rs_el_secondaries:
                continue
            if node['priority'] < 1:
                logging.debug('Host "%s" will not be freezed due to priority "%s"', node['host'], node['priority'])
                continue
            node_host, node_port = node['host'].split(':')
            cmd = bson.son.SON([('replSetFreeze', freeze_seconds)])
            _exec_command(
                action=cmd,
                name=None,
                user=user,
                password=password,
                host=node_host,
                port=int(node_port),
                database='admin',
                conn_options=options,
            )
        except Exception as err:
            log.warning('Failed to freeze host %s: %s', host, err, exc_info=True)
            if strict:
                raise err

    return True


def wait_for_rs_joined(deadline=None, timeout=None, user=None, password=None, host=None, port=None, authdb='admin'):
    if timeout is None:
        timeout = 5
    timeout_ms = timeout * 1000

    if deadline is None:
        deadline = 10

    options = dict((opt, timeout_ms) for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))
    conn = None
    deadline += int(time.time())
    while time.time() < deadline:
        try:
            if not _conn_is_alive(conn, authdb):
                conn = _connect(user, password, host, port, authdb, options=options)

            my_state = conn.admin.command('replSetGetStatus')['myState']
            if my_state not in MEMBER_ERROR_STATES.values():
                return True
            log.debug('Waiting for normal state')

        except Exception as err:
            log.warning('Failed while waiting for role with error:' ' %s', err, exc_info=True)

        time.sleep(5)
    return False


def list_shards(user=None, password=None, host=None, port=None, authdb=None):
    '''
    List all shards

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.list_shards <user> <password> <host> <port>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    log.info('Listing shards')
    try:
        shards = conn.admin.command('listShards')
        log.debug('Retrieved shards list: %s', shards)
        return shards
    except pymongo.errors.PyMongoError as err:
        log.error('Listing shards failed with error: %s', _get_error_message(err))
        raise


def list_shard_ids(user=None, password=None, host=None, port=None, authdb=None):
    '''
    List all shard ids

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.list_shard_ids <user> <password> <host> <port> <authdb>
    '''
    shards = list_shards(user, password, host, port, authdb)
    return [s['_id'] for s in shards['shards']]


def add_shard(shard_name, shard_url, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Add given shard to cluster

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.add_shard <shard_name> <shard_url> <user> <password> <host> <port>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    cmd_kwargs = dict(command=bson.son.SON([('addShard', shard_url), ('name', shard_name)]), check=True)
    log.info('Running command: kwargs=%s', cmd_kwargs)
    try:
        cmd = conn.admin.command(**cmd_kwargs)
        log.debug('addShard returns: %s', cmd)
        return cmd['ok'] == 1
    except pymongo.errors.PyMongoError as err:
        log.error('Add shard failed with error: %s', _get_error_message(err))
        raise


def shard_remove(
    shard_name, user=None, password=None, host=None, port=None, authdb=None, drain_timeout=None, drain_retry_wait=None
):
    '''
    Add given shard to cluster

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.shard_remove <shard_name> <user> <password> <host> <port>
    '''
    if drain_timeout is None:
        drain_timeout = DEFAULT_SHARD_DRAIN_TIMEOUT
    if drain_retry_wait is None:
        drain_retry_wait = DEFAULT_DRAIN_RETRY_WAIT

    drain_eta = time.time() + drain_timeout
    retries_left = DEFAULT_RETRIES_COUNT

    conn = _connect(user, password, host, port, authdb=authdb)
    cmd_kwargs = dict(command=bson.son.SON([('removeShard', shard_name)]), check=True)

    while drain_eta > time.time():
        log.info('Running command: kwargs=%s', cmd_kwargs)
        try:
            cmd = conn.admin.command(**cmd_kwargs)
            log.debug('removeShard returns: %s', cmd)
            if cmd['ok'] != 1:
                raise RuntimeError('Command "removeShard" returns error')

            if cmd['state'] == 'completed':
                log.info('Shard "%s" draining is completed', cmd['shard'])
                return True

            if cmd['state'] == 'started':
                log.debug('Shard draining was started')

            if cmd['state'] == 'ongoing':
                chunks_left = cmd['remaining']['chunks']
                primary_dbs = cmd.get('dbsToMove', [])
                log.debug('Shard draining is in progress: {0} chunks left'.format(chunks_left))
                if chunks_left == 0 and primary_dbs > 0:
                    raise RuntimeError('Primary databases were found: {0}'.format(','.join(primary_dbs)))

        except pymongo.errors.AutoReconnect:
            if not retries_left:
                raise
            conn = _connect(user, password, host, port, authdb=authdb)
            retries_left -= 1

        except pymongo.errors.PyMongoError as err:
            log.error('Remove shard failed with error: %s', _get_error_message(err))
            raise

        time.sleep(drain_retry_wait)

    raise RuntimeError('Drain timeout exceeded')


def get_shards_primary_dbs(user=None, password=None, host=None, port=None, authdb=None):
    '''
    Get 'shard' -> 'primary dbs list' map

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.get_shards_primary_dbs <user> <password> <host> <port>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    config_db = conn.get_database('config')
    if 'databases' not in config_db.collection_names():
        return {}
    db_shard_map = defaultdict(list)
    for doc in config_db['databases'].find():
        db_shard_map[doc['primary']].append(doc['_id'])
    return db_shard_map


def get_oplog_maxsize(host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Get current oplog maxsize

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.get_oplog_maxsize <host> <port> <user> <password> <authdb>
    '''
    conn = _connect(user, password, host, port, 'local', authdb)

    log.debug('getting oplog size')
    maxSize = conn.local.command('collstats', 'oplog.rs')['maxSize'] / 1024.0 / 1024.0

    return maxSize


def set_oplog_maxsize(max_size, host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Set current oplog maxsize

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.set_oplog_maxsize <max_size> <host> <port> <user> <password> <authdb>
    '''
    if max_size < 990:
        raise ValueError('oplog max size should be 990 or greater')
    conn = _connect(user, password, host, port, 'admin', authdb)

    log.debug('setting oplog size to %s', max_size)
    try:
        conn.admin.command({'replSetResizeOplog': 1, 'size': float(max_size)})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while setting max size of oplog for %s:%s: %s', host, port, err)
        raise
    return True


def get_feature_compatibility_version(host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Get feature compatibility version

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.get_feature_compatibility_version <host> <port> <user> <password> <authdb>
    '''
    conn = _connect(user, password, host, port, 'admin', authdb)

    log.debug('getting featureCompatibilityVersion')
    ret = conn.admin.command({'getParameter': 1, 'featureCompatibilityVersion': 1})
    return ret['featureCompatibilityVersion']['version']


def set_feature_compatibility_version(fcv, host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Set feature compatibility version

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.set_feature_compatibility_version <fcv> <host> <port> <user> <password> <authdb>
    '''
    conn = _connect(user, password, host, port, 'admin', authdb)

    log.debug('setting featureCompatibilityVersion to %s', fcv)
    try:
        conn.admin.command({'setFeatureCompatibilityVersion': fcv})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while setting featureCompatibilityVersion: %s', err, exc_info=True)
        return False
    return True


def get_free_monitoring_state(host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Get free monitoring state

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.get_free_monitoring_state <host> <port> <user> <password> <authdb>
    '''
    conn = _connect(user, password, host, port, 'admin', authdb)

    log.debug('running getFreeMonitoringStatus')
    ret = conn.admin.command({'getFreeMonitoringStatus': 1})
    return ret['state']


def set_free_monitoring_state(action, host=None, port=None, user=None, password=None, authdb='admin'):
    '''
    Set free monitoring state

    CLI Example:

    .. code-block:: bash

        salt '*' mongodb.set_free_monitoring_state <state> <host> <port> <user> <password> <authdb>
    '''
    conn = _connect(user, password, host, port, 'admin', authdb)

    log.debug('setting FreeMonitoring state to %s', action)
    try:
        cmd_kwargs = dict(command=bson.son.SON([('setFreeMonitoring', 1), ('action', action)]), check=True)
        conn.admin.command(**cmd_kwargs)
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while setting FreeMonitoring state: %s', err, exc_info=True)
        return False
    return True


def add_shard_to_zone(shard, zone, **kwargs):
    '''
    Add given shard to given zone
    '''
    conn = _connect(**kwargs)

    log.debug('Adding shard %s to zone %s', shard, zone)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('addShardToZone', shard),
                    ('zone', zone),
                ]
            )
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while adding shard to zone: %s', err, exc_info=True)
        raise
    return True


def remove_shard_from_zone(shard, zone, **kwargs):
    '''
    Add given shard to given zone
    '''
    conn = _connect(**kwargs)

    log.debug('Removing shard %s from zone %s', shard, zone)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('removeShardFromZone', shard),
                    ('zone', zone),
                ]
            )
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while removing shard from zone: %s', err, exc_info=True)
        raise
    return True


def list_databases(**kwargs):
    '''
    Get list of databases
    '''
    return _connect(**kwargs).list_database_names()


def list_sharded_databases(**kwargs):
    '''
    Get list of sharded databases (i.e. databases, we did sh.enableSharding(ad) for)
    '''
    conn = _connect(**kwargs)
    mdb = pymongo.database.Database(conn, 'config')
    databases = mdb.databases.find({"partitioned": True})
    if not databases:
        return []
    return [row['_id'] for row in databases]


def list_collections(database, **kwargs):
    '''
    Get list of collection names in given db
    '''
    conn = _connect(**kwargs)

    log.debug('Get list of collections of database %s', database)
    try:
        return conn[database].list_collection_names()
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while listing collections: %s', err, exc_info=True)
        raise


def list_sharded_collections(database, **kwargs):
    '''
    Get list of sharded collections. Each element is fow from config.collections where _id is database.collection
    '''
    conn = _connect(**kwargs)
    mdb = pymongo.database.Database(conn, 'config')
    collections = list(
        mdb.collections.find(
            {
                '_id': re.compile("^{}\\.".format(database)),
                '$or': [
                    {'dropped': False},
                    {'dropped': {'$exists': False}},
                ],
            }
        )
    )
    if not collections:
        return []
    return collections


def list_shard_tags(database, **kwargs):
    '''
    list tags (== zones) info for all collections of given DB
    '''
    conn = _connect(**kwargs)
    mdb = pymongo.database.Database(conn, 'config')
    collections = list(mdb.tags.find({'ns': re.compile("^{}\\.".format(database))}))
    if not collections:
        return []
    return collections


def drop_collection(database, collection, **kwargs):
    '''
    Drop given collection
    '''
    conn = _connect(**kwargs)

    log.debug('Trying to drop collection %s.%s', database, collection)
    try:
        conn[database].command({'drop': collection})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure while drop collection: %s', err, exc_info=True)
        raise
    return True


def update_zone_key_range(database, collection, minkey, maxkey, zone, **kwargs):
    '''
    Update zone key range for given collection, read MongoDB dos for updateZoneKeyRange
    for more info
    '''
    conn = _connect(**kwargs)

    log.debug('Updating zone range of collection %s', collection)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('updateZoneKeyRange', '{}.{}'.format(database, collection)),
                    ('min', minkey),
                    ('max', maxkey),
                    ('zone', zone),
                ]
            )
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during updateZoneKeyRange: %s', err, exc_info=True)
        raise
    return True


def shard_collection(database, collection, key, **kwargs):
    '''
    Shard given collection (exec  {shardCollection: fullName, key: key})
    '''
    conn = _connect(**kwargs)

    log.debug('Sharding collection %s', collection)
    try:
        conn.admin.command({'shardCollection': '{}.{}'.format(database, collection), 'key': key})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during shardCollection: %s', err, exc_info=True)
        raise
    return True


def enable_sharding(database, **kwargs):
    '''
    Enable sharding for given database (exec {enableSharding: dbname})
    '''
    conn = _connect(**kwargs)

    log.debug('Enabling sharding of database %s', database)
    try:
        conn.admin.command({'enableSharding': database})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during enableSharding: %s', err, exc_info=True)
        raise
    return True


def is_capped(database, collection, **kwargs):
    '''
    Check if collection is capped
    '''
    conn = _connect(**kwargs)

    log.debug('Checking if collection is capped: %s.%s', database, collection)
    try:
        res = conn[database].command({'listCollections': 1, 'filter': {'name': collection}})
        if len(res['cursor']['firstBatch']) == 0:
            return False

        return res['cursor']['firstBatch'][0]['options'].get('capped', False)
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during checking is collection is capped : %s', err, exc_info=True)
        raise
    return True


def convert_to_capped(database, collection, size, **kwargs):
    '''
    Convert given collection to capped
    '''
    conn = _connect(**kwargs)

    log.debug('Capping collection %s.%s', database, collection)
    try:
        conn[database].command({'convertToCapped': collection, 'size': size})
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during convertToCapped: %s', err, exc_info=True)
        raise
    return True


def create_index(database, collection, key, options, **kwargs):
    '''
    Create index on given collection with given key and options
    '''
    conn = _connect(**kwargs)

    log.debug('Creating index %s on collection: %s.%s', key, database, collection)
    try:
        conn[database][collection].create_index(key, **options)
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during index creation: %s', err, exc_info=True)
        raise
    return True


def list_indexes(database, collection, **kwargs):
    '''
    Get list of indexes for given collection
    '''
    conn = _connect(**kwargs)

    log.debug('List indexes of collection: %s.%s', database, collection)
    try:
        return conn[database][collection].index_information()
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during list collection indexes: %s', err, exc_info=True)
        raise


def get_balancer_state(**kwargs):
    '''
        Get state of balancer (like sh.getBalancerState())

        CLI Example:
    от пар
        .. code-block:: bash

            salt '*' mongodb.get_balancer_state user=<user> password=<password> host=<host> port=<port> authdb=<authdb>
    '''
    conn = _connect(**kwargs)
    balancer = []
    try:
        mdb = pymongo.database.Database(conn, 'config')
        balancer = list(mdb.settings.find({'_id': 'balancer'}))
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during checking balancer state: %s', err, exc_info=True)
        raise

    if not balancer or len(balancer) < 1:
        return True

    return not balancer[0].get('stopped', False)


def start_balancer(timeout, **kwargs):
    '''
    Start mongos balancer
    '''
    conn = _connect(**kwargs)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('balancerStart', 1),
                    ('maxTimeMS', timeout),
                ]
            )
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during balancerStart(): %s', err, exc_info=True)
        raise
    return True


def stop_balancer(timeout, **kwargs):
    '''
    Start mongos balancer
    '''
    conn = _connect(**kwargs)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('balancerStop', 1),
                    ('maxTimeMS', timeout),
                ]
            )
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during balancerStop(): %s', err, exc_info=True)
        raise
    return True


def get_balancer_active_window(user=None, password=None, host=None, port=None, authdb=None):
    '''
    Get balancer active window

    CLI Example:
    .. code-block:: bash

        salt '*' mongodb.get_balancer_active_window <user> <password> <host> <port> <authdb>
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    config_db = conn.get_database('config')
    balancer = config_db.settings.find_one({'_id': 'balancer'})
    return balancer.get('activeWindow', {}) if balancer is not None else {}


def set_balancer_active_window(start, stop, user=None, password=None, host=None, port=None, authdb=None):
    '''
    set balancer active window

    CLI Example:
    .. code-block:: bash

        salt '*' mongodb.set_balancer_active_window start stop <user> <password> <host> <port> <authdb>
    '''

    assert BALANCER_WINDOW_REGEX.match(start), 'start time does not match regex "{}"'.format(
        BALANCER_WINDOW_REGEX.pattern
    )
    assert BALANCER_WINDOW_REGEX.match(stop), 'stop time does not match regex "{}"'.format(
        BALANCER_WINDOW_REGEX.pattern
    )

    conn = _connect(user, password, host, port, authdb=authdb)
    config_db = conn.get_database('config')
    config_db.settings.update_one(
        {'_id': 'balancer'}, {'$set': {'activeWindow': {'start': start, 'stop': stop}}}, upsert=True
    )


def unset_balancer_active_window(user=None, password=None, host=None, port=None, authdb=None):
    '''
    unset balancer active window

    CLI Example:
    .. code-block:: bash

        salt '*' mongodb.unset_balancer_active_window start stop <user> <password> <host> <port> <authdb>
    '''

    conn = _connect(user, password, host, port, authdb=authdb)
    config_db = conn.get_database('config')
    config_db.settings.update_one({'_id': 'balancer'}, {'$unset': {'activeWindow': True}})


def get_last_resetup_id():
    '''
    Helper function for getting ID of last resetup if any
    '''
    if not os.path.isfile(RESETUP_ID_FILE):
        # No such file
        return None

    with open(RESETUP_ID_FILE, 'r') as f:
        return f.read().strip()


def mark_resetup_done(resetup_id):
    '''
    Helper function for storing resetup id for future calls
    and clear resetup-started flag
    '''
    if resetup_id is not None:
        with open(RESETUP_ID_FILE, 'w') as f:
            f.write(resetup_id)

    # Resetup Completed, remove mark, that it is started
    if os.path.isfile(RESETUP_STARTED_FILE):
        os.unlink(RESETUP_STARTED_FILE)


def is_resetup_started(resetup_id=None):
    '''
    Helper function for getting ID of last resetup if any
    '''
    if not os.path.isfile(RESETUP_STARTED_FILE):
        # No such file
        return False
    return True


def mark_resetup_started(resetup_id):
    '''
    Helper function for storing resetup id for future calls
    '''
    with open(RESETUP_STARTED_FILE, 'w') as f:
        f.write(resetup_id)


def get_last_stepdown_id():
    '''
    Helper function for getting ID of last stepdown if any
    '''
    if not os.path.isfile(STEPDOWN_ID_FILE):
        # No such file
        return None

    with open(STEPDOWN_ID_FILE, 'r') as f:
        return f.read().strip()


def mark_stepdown_done(stepdown_id):
    '''
    Helper function for storing stepdown id for future calls
    '''
    if stepdown_id is not None:
        with open(STEPDOWN_ID_FILE, 'w') as f:
            f.write(stepdown_id)


def resetup_mongod(force=False, continue_flag=False):
    args = []
    if force:
        args.append('--force')
    if continue_flag:
        args.append('--continue')
    p = subprocess.Popen([MDB_MONGOD_RESETUP] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    ret = p.poll()
    if ret != 0:
        raise Exception(
            "mdb-mongod-resetup returned not zero code: {retcode},\n stdout: {stdout},\n stderr: {stderr}".format(
                retcode=ret, stdout=stdout, stderr=stderr
            )
        )
    return (stdout, stderr)


def set_wt_engine_config(cfg, user=None, password=None, host=None, port=None, authdb=None):
    '''
    Set mongodb WT cache size
    '''
    conn = _connect(user, password, host, port, authdb=authdb)
    try:
        conn.admin.command(
            bson.son.SON(
                [
                    ('setParameter', 1),
                    ('wiredTigerEngineRuntimeConfig', cfg),
                ]
            ),
            check=True,
        )
    except pymongo.errors.OperationFailure as err:
        log.error('OperationFailure during setting wt config "%s": %s', cfg, err, exc_info=True)
        raise
    return True
