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
import time
import traceback
import yaml
import sys

from copy import deepcopy
from functools import wraps
from sys import getsizeof

try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc

try:
    try:
        import pymongo
        from pymongo.max_key import MaxKey
        from pymongo.min_key import MinKey
    except ImportError:
        from bson.max_key import MaxKey
        from bson.min_key import MinKey
    HAS_MINMAXKEY = True
except ImportError:
    HAS_MINMAXKEY = False

try:
    import pymongo.auth as mongo_auth
    import hmac
    from hashlib import sha1, sha256, pbkdf2_hmac
    from base64 import standard_b64decode, standard_b64encode

    HAS_MONGOAUTH = True
except ImportError:
    HAS_MONGOAUTH = False


try:
    import six
except ImportError:
    from salt.ext import six

# Define the module's virtual name
__virtualname__ = 'mdb_mongodb'

PILLAR_PATH_USERS = 'data:mongodb:users'
PILLAR_PATH_DATABASES = 'data:mongodb:databases'
PILLAR_PATH_SUBCLUSTERS = 'data:dbaas:cluster:subclusters'

MONGODB_ERROR_USER_NOT_FOUND = 11

INTERNAL_DBS = ['mdb_internal', 'config', 'local', 'admin']
FORBIDDEN_LOGIN_DBS = ['config', 'local']


# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}
__pillar__ = {}


log = logging.getLogger(__name__)


def _wrap_log(msg_type, repr_it, *args):
    assert msg_type in ('log', 'info', 'debug', 'error')
    level = None
    LIMIT = 10 * 2**10  # 10Kb, why not
    if len(args) >= 3:
        level = args[0]
        msg = args[1]
        log_args = args[2:]
    else:
        msg = args[0]
        log_args = args[1:]
    logger = getattr(log, msg_type)

    if sum(getsizeof(arg) for arg in args) > LIMIT:
        log.debug('Received more than %d bytes to log in %s, skipping', LIMIT, msg)
        return

    try:
        if repr_it:
            log_args = map(repr, log_args)
        if level is None:
            logger(msg, *log_args)
        else:
            logger(level, msg, *log_args)
    except MemoryError:
        log.error('Caught MemoryError executing %s', msg)


def wrap_log(msg_type, *args):
    _wrap_log(msg_type, False, *args)


def wrap_log_repr(msg_type, *args):
    _wrap_log(msg_type, True, *args)


def __virtual__():
    if 'mongodb.user_exists' not in __salt__:
        return False, 'Unable to load mongodb module.'
    return __virtualname__


def return_false_on_exception(func):
    '''In case of Exception, return {result: False} with exception message as comment'''

    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception as exc:
            log.error(traceback.format_exc())
            name = kwargs.get('name', None)
            return {
                'name': name,
                'changes': {},
                'result': False,
                'comment': str(exc),
                'trace': traceback.format_exc(),
            }

    return wrapper


def exec_on_master_or_wait(timeout):
    '''
    Exec state if on master of wait timeout seconds for master to apply state
    '''

    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if __opts__['test']:
                return func(*args, **kwargs)

            conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
            mongodb = MDBMongoMongodbInterface(conn_opts)
            stop_ts = time.time() + timeout
            while time.time() < stop_ts:
                if mongodb.is_master():
                    return func(*args, **kwargs)
                else:
                    # Check if master already did changes
                    __opts__['test'] = True
                    ret = {}
                    try:
                        ret = func(*args, **kwargs)
                    except Exception:
                        ret = {'result': False}

                    __opts__['test'] = False
                    if ret['result']:
                        return ret
                time.sleep(1)

            raise Exception('Not Master and master didn\'t completed request for timeout')

        return wrapper

    return decorator


def retry_on_fail(soft_tries_limit=5, soft_tries_timeout=300, hard_tries_limit=10):
    '''
    Retry if state fails
    '''

    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if __opts__['test']:
                return func(*args, **kwargs)

            soft_tries = 0
            hard_tries = 0
            last_ts = time.time()
            last_exc = None
            ret = {}
            while soft_tries < soft_tries_limit and hard_tries < hard_tries_limit:
                try:
                    last_exc = None
                    last_ts = time.time()
                    ret = func(*args, **kwargs)
                except Exception as exc:
                    log.error(exc, exc_info=True)
                    last_exc = exc
                    ret = {'result': False}

                if ret.get('result', False):
                    return ret

                if time.time() >= last_ts + soft_tries_timeout:
                    hard_tries += 1
                    soft_tries = 0
                else:
                    soft_tries += 1

                time.sleep(1)
            # No luck after given number of retries
            if last_exc is not None:
                raise last_exc
            else:
                return ret

        return wrapper

    return decorator


def _deep_format(struct, **kwargs):
    """
    perform .format(**kwargs) to each found string and try to preserve stucture
    """
    if isinstance(struct, six.string_types):
        return struct.format(**kwargs)
    elif isinstance(struct, dict):
        ret = {}
        for key in struct.keys():
            ret[_deep_format(key, **kwargs)] = _deep_format(struct[key], **kwargs)
        return ret
    elif isinstance(struct, collections_abc.Iterable):
        ret = []
        for val in struct:
            ret.append(_deep_format(val, **kwargs))

        if isinstance(struct, set):
            ret = set(ret)
        return ret
    else:
        return deepcopy(struct)


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
        elif isinstance(reference, collections_abc.Iterable) and not isinstance(reference, six.string_types):
            # zip() truncates the longer element, need to account for that
            if len(reference) != len(candidate):
                return False
            if isinstance(reference, tuple):
                pair = zip(reference, candidate)
            else:
                pair = zip(
                    sorted(reference, key=_ignore_unhashable_order), sorted(candidate, key=_ignore_unhashable_order)
                )
            for first, second in pair:
                if not _compare(first, second):
                    return False
            return True
        return reference == candidate

    try:
        return _compare(one, other)
    except (LookupError, TypeError, ValueError):
        return False


def _normalize_inplace(input_struct):
    # Handle OrderedDicts
    if isinstance(input_struct, dict):
        for key in input_struct:
            input_struct[key] = _normalize_inplace(input_struct[key])
        return dict(input_struct)
    if isinstance(input_struct, six.string_types):
        return input_struct
    if isinstance(input_struct, collections_abc.Iterable):
        for num, _ in enumerate(input_struct):
            input_struct[num] = _normalize_inplace(input_struct[num])
        return input_struct
    return input_struct


def _sort_list_recursive(obj, key_sort=None):
    if isinstance(obj, list):
        obj.sort(key=key_sort)
        for i in obj:
            _sort_list_recursive(i)

    elif isinstance(obj, dict):
        for k in obj.keys():
            _sort_list_recursive(obj[k])

    return


# TODO: Probably we need to move this function somewhere to common module
def _yaml_diff(one, another):
    if not (isinstance(one, dict) and isinstance(another, dict)):
        raise TypeError('both arguments must be dicts')

    one_str = yaml.safe_dump(one or '', default_flow_style=False)
    another_str = yaml.safe_dump(another or '', default_flow_style=False)
    differ = difflib.Differ()

    diff = differ.compare(one_str.splitlines(True), another_str.splitlines(True))
    return '\n'.join(diff)


class MDBMongoConnectionOptions(object):
    '''Simple class for holding MongoDB connection options'''

    def __init__(
        self,
        host='127.0.0.1',
        port=27018,
        user=None,
        password=None,
        authdb=None,
        options=None,
    ):
        try:
            self.port = int(port)
        except TypeError:
            raise TypeError('Port ({0}) is not an integer.'.format(port))

        self.host = host
        self.user = user
        self.password = password
        self.authdb = authdb
        self.options = options

    @classmethod
    def read_from_pillar(cls, **kwargs):
        args = __salt__['mdb_mongodb_helpers.get_mongo_connection_args'](**kwargs)
        return MDBMongoConnectionOptions(**args)

    def __repr__(self):
        return "MDBMongoConnectionOptions({0}, {1}, {2}, {3}, {4}, {5}".format(
            *map(
                repr,
                (
                    self.host,
                    self.port,
                    self.user,
                    self.password,
                    self.authdb,
                    self.options,
                ),
            )
        )

    def as_dict(self, return_options=False):
        ret = {
            'user': self.user,
            'password': self.password,
            'host': self.host,
            'port': self.port,
            'authdb': self.authdb,
        }
        if return_options:
            ret['options'] = self.options
        return ret

    def copy(self):
        return MDBMongoConnectionOptions(**self.as_dict(True))


class MDBMongoMongodbInterface(object):
    '''
    Wrapper to mongo.
    Used for real communication with mongo,
     such as get list of users (in raw format) or add/modify/delete users.
    Have quite low-level interface
    '''

    def __init__(self, conn_opts):
        if not isinstance(conn_opts, MDBMongoConnectionOptions):
            raise TypeError('conn_opts should be instance of MDBMongoConnectionOptions')
        self.conn_opts = conn_opts

    def list_users(self):
        '''
        List users in mongodb
        '''
        return __salt__['mongodb.list_users'](**self.conn_opts.as_dict())

    def _add_user(self, username, authdb, password, roles):
        if not __salt__['mongodb.user_create'](
            username, password, database=authdb, roles=roles, **self.conn_opts.as_dict()
        ):
            raise Exception('Failed to create user {0}@{1}'.format(username, authdb))

    def _add_internal_user(self, user):
        '''
        Add internal user to MongoDB
        '''
        # Internal users should be created in admin db only
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        roles = user.roles.format_roles()
        wrap_log('info', 'Adding internal user %s with roles %s', user.username, roles)

        self._add_user(user.username, user.internal_authdb, user.password, roles)

    def _add_common_user(self, user):
        '''
        Add common user to mongodb
        '''
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        user_roles = user.roles.format_roles()
        for roles in user.roles:
            db = roles.get_database()
            wrap_log('info', 'Adding user %s@%s with roles %s', user.username, db, user_roles)
            self._add_user(user.username, db, user.password, user_roles)

    def add_user(self, user):
        '''Actually add user to MongoDB, don't check if it already exists there'''
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser but {} given'.format(type(user)))
        if user.internal:
            return self._add_internal_user(user)
        else:
            return self._add_common_user(user)

    def _delete_user(self, username, authdb):
        try:
            if not __salt__['mongodb.user_remove'](username, database=authdb, **self.conn_opts.as_dict()):
                raise Exception('Failed to remove user {0}@{1}'.format(username, authdb))
        except pymongo.errors.OperationFailure as exc:
            log.error("Error during deleteing user: %s", exc)
            if exc.code() == MONGODB_ERROR_USER_NOT_FOUND:
                log.error("User not found, not need to delete")
            else:
                raise

    def _delete_internal_user(self, user):
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')
        # Internal users should be deleted from admin db only
        log.info('Deleteing internal user %s', user.username)
        self._delete_user(user.username, user.internal_authdb)

    def _delete_common_user(self, user):
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')
        for roles in user.roles:
            db = roles.get_database()
            log.info('Deleteing user %s@%s', user.username, db)
            self._delete_user(user.username, db)

    def delete_user(self, user):
        '''Actually delete user from all DB's'''
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        if user.internal:
            self._delete_internal_user(user)
        else:
            self._delete_common_user(user)

    def _update_user(self, username, authdb, password=None, roles=None):
        if not __salt__['mongodb.user_update'](
            username, database=authdb, passwd=password, roles=roles, **self.conn_opts.as_dict()
        ):
            raise Exception('Failed to update user {0}@{1}'.format(username, authdb))

    def _update_internal_user(self, user, old_user):
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        roles = user.roles.format_roles()
        wrap_log('info', 'Modifying internal user %s with roles %s', user.username, roles)
        password = None
        if user.password_changed:
            password = user.password
        self._update_user(user.username, user.internal_authdb, password=password, roles=roles)

    def _update_common_user(self, user, old_user):
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        log.info('Start modifying user %s', user.username)
        add_dbs = set(user.roles - old_user.roles) - set(FORBIDDEN_LOGIN_DBS)
        del_dbs = set(old_user.roles - user.roles) - set(FORBIDDEN_LOGIN_DBS)
        # https://st.yandex-team.ru/MDB-6513 user should be able to have access
        # to all needed databases even if it wasn't used them as authDB
        # update_dbs = set(user.roles.get_diff_dbs(old_user.roles))
        # Actually, now we always update all databases even if user just wanted
        # to change password, but in other hand password change and user roles modification
        # is made by the same function, so do not need to worry, I suppose
        update_dbs = set(user.roles.get_databases()).intersection(set(old_user.roles.get_databases())) - set(
            FORBIDDEN_LOGIN_DBS
        )
        roles = user.roles.format_roles()

        for db in add_dbs:
            wrap_log('info', 'Modifying user %s, add access to db %s with roles %s', user.username, db, roles)
            self._add_user(user.username, db, password=user.password, roles=roles)

        for db in update_dbs:
            wrap_log(
                'info',
                'Modifying user %s, updating access to db %s with roles %s',
                user.username,
                db,
                roles,
            )
            password = None
            if user.password_changed:
                password = user.password
            self._update_user(user.username, db, password=password, roles=roles)

        for db in del_dbs:
            log.info('Modifying user %s, remove it from db %s', user.username, db)
            self._delete_user(user.username, db)

        # Change password if needed:
        # Todo: Not needed anymore, actually. But need to check and think if we want to change it
        if user.password_changed:
            for roles in user.roles:
                db = roles.get_database()
                if db in add_dbs or db in update_dbs:
                    # Skip password update if it was updated already
                    continue
                log.info('Changing password for user %s@%s', user.username, db)
                self._update_user(user.username, db, password=user.password)

    def update_user(self, user, old_user):
        '''
        Modify user, i.e. change password if needed and grant/revoke all needed roles
        '''
        if not isinstance(user, MDBMongoUser):
            raise TypeError('user should be instance of MDBMongoUser')

        if user.internal:
            self._update_internal_user(user, old_user)
        else:
            self._update_common_user(user, old_user)

    def auth_user(self, username, authdb, password, raise_exception=False):
        auth_ok, _ = __salt__['mongodb.check_auth'](
            username, password, self.conn_opts.host, self.conn_opts.port, authdb, raise_exception
        )
        return auth_ok

    def check_user_password(
        self,
        user,
        authdb=None,
        set_password_changed=True,
        target_credentials=None,
    ):
        '''
        Check if current password can be used to log in with this user
         and set password_changed accordingly
        '''
        # No need to recheck password if we already decided to change it
        if user.password_changed is True:
            return False

        if authdb is None:
            authdb = user.get_some_authdb()

        password_ok = None

        if all((user.credentials, target_credentials, HAS_MONGOAUTH)):
            for algo, credentials in user.credentials.items():
                if algo in target_credentials:
                    password_ok = _compare_structs(credentials, target_credentials[algo])
                    break

        if password_ok is None:
            password_ok = self.auth_user(user.username, authdb, user.password)

        if set_password_changed:
            user.password_changed = not password_ok
        return password_ok

    def is_master(self):
        ret, _ = __salt__['mongodb.is_primary'](**self.conn_opts.as_dict())
        return ret

    def list_roles(self, role=None):
        '''List custom roles from MongoDB'''
        return __salt__['mongodb.list_roles'](role=role, **self.conn_opts.as_dict())

    def delete_role(self, role, database=None):
        '''Delete custom role'''

        if isinstance(role, six.string_types):
            if database is None:
                raise ValueError('database can not be none in case of role is string')
        elif isinstance(role, MDBMongoDBRole):
            database = role.database
            role = role.name
        else:
            raise TypeError("role should be string or MDBMongoDBRole")

        return __salt__['mongodb.drop_role'](name=role, database=database, **self.conn_opts.as_dict())

    def update_role(self, role):
        '''Update custom role'''

        if not isinstance(role, MDBMongoDBRole):
            raise TypeError("role should be MDBMongoDBRole")

        return __salt__['mongodb.update_role'](
            name=role.name,
            database=role.database,
            roles=role.roles.format_roles(),
            privileges=role.privileges,
            authenticationRestrictions=role.authenticationRestrictions,
            **self.conn_opts.as_dict()
        )

    def add_role(self, role):
        '''Create custom role'''

        if not isinstance(role, MDBMongoDBRole):
            raise TypeError("role should be MDBMongoDBRole")

        return __salt__['mongodb.create_role'](
            name=role.name,
            database=role.database,
            roles=role.roles.format_roles(),
            privileges=role.privileges,
            authenticationRestrictions=role.authenticationRestrictions,
            **self.conn_opts.as_dict()
        )

    def list_shards(self):
        '''
        List shards in mongodb
        '''
        return __salt__['mongodb.list_shards'](**self.conn_opts.as_dict())['shards']

    def add_shard_to_zone(self, shard_name, zone_name):
        '''
        Add given shard to given zone
        '''
        return __salt__['mongodb.add_shard_to_zone'](shard=shard_name, zone=zone_name, **self.conn_opts.as_dict())

    def remove_shard_from_zone(self, shard_name, zone_name):
        '''
        Add given shard to given zone
        '''
        return __salt__['mongodb.remove_shard_from_zone'](shard=shard_name, zone=zone_name, **self.conn_opts.as_dict())

    def list_sharded_databases(self):
        return __salt__['mongodb.list_sharded_databases'](**self.conn_opts.as_dict())

    def list_collections(self, database):
        return __salt__['mongodb.list_collections'](database, **self.conn_opts.as_dict())

    def list_sharded_collections(self, database):
        rows = __salt__['mongodb.list_sharded_collections'](database, **self.conn_opts.as_dict())
        collections = []
        for row in rows:
            col = row['_id'].split('.', 1)[1]
            collections.append(col)

        return collections

    def get_collections_and_tags(self, database):
        tags = __salt__['mongodb.list_shard_tags'](database, **self.conn_opts.as_dict())
        cols = {}
        for row in tags:
            col = row['ns'].split('.', 1)[1]
            if col in cols:
                cols[col].append(row['tag'])
            else:
                cols[col] = [row['tag']]

        return cols

    def drop_collection(self, database, collection):
        return __salt__['mongodb.drop_collection'](database, collection, **self.conn_opts.as_dict())

    def shard_collection_to_zone(self, database, collection, shard_key, zone):
        assert HAS_MINMAXKEY, 'No MinKey/MaxKey mongo types are found'
        __salt__['mongodb.update_zone_key_range'](
            database,
            collection,
            minkey={shard_key: MinKey()},
            maxkey={shard_key: MaxKey()},
            zone=zone,
            **self.conn_opts.as_dict()
        )

        return __salt__['mongodb.shard_collection'](
            database, collection, key={shard_key: 1}, **self.conn_opts.as_dict()
        )

    def shard_database(self, database):
        return __salt__['mongodb.enable_sharding'](database, **self.conn_opts.as_dict())

    def is_capped(self, database, collection):
        return __salt__['mongodb.is_capped'](database, collection, **self.conn_opts.as_dict())

    def convert_to_capped(self, database, collection, size):
        return __salt__['mongodb.convert_to_capped'](database, collection, size, **self.conn_opts.as_dict())

    def create_ttl_index(self, database, collection, field, ttl):
        return __salt__['mongodb.create_index'](
            database, collection, field, {'expireAfterSeconds': ttl}, **self.conn_opts.as_dict()
        )

    def list_indexes(self, database, collection):
        return __salt__['mongodb.list_indexes'](database, collection, **self.conn_opts.as_dict())

    def has_ttl_index(self, database, collection, key):
        '''
        Check if collection has TTL index for given field
        '''
        indexes = self.list_indexes(database, collection)
        for index in indexes.values():
            if len(index['key']) == 1 and key == index['key'][0][0] and 'expireAfterSeconds' in index:
                return True

        return False

    def get_balancer_state(self):
        return __salt__['mongodb.get_balancer_state'](**self.conn_opts.as_dict())

    def set_balancer_state(self, started, timeout=60000):
        if started:
            return __salt__['mongodb.start_balancer'](timeout, **self.conn_opts.as_dict())
        else:
            return __salt__['mongodb.stop_balancer'](timeout, **self.conn_opts.as_dict())

    def get_balancer_active_window(self):
        return __salt__['mongodb.get_balancer_active_window'](**self.conn_opts.as_dict())

    def set_balancer_active_window(self, start, stop):
        return __salt__['mongodb.set_balancer_active_window'](start, stop, **self.conn_opts.as_dict())

    def unset_balancer_active_window(self):
        return __salt__['mongodb.unset_balancer_active_window'](**self.conn_opts.as_dict())

    def list_databases(self):
        return __salt__['mongodb.db_list'](**self.conn_opts.as_dict())

    def drop_database(self, database):
        return __salt__['mongodb.db_remove'](database, **self.conn_opts.as_dict())

    def stepdown(self):
        return __salt__['mongodb.rs_step_down'](**self.conn_opts.as_dict())

    def resetup(self, force=False, continue_flag=False):
        return __salt__['mongodb.resetup_mongod'](force=force, continue_flag=continue_flag)

    def get_last_resetup_id(self):
        return __salt__['mongodb.get_last_resetup_id']()

    def mark_resetup_done(self, resetup_id):
        return __salt__['mongodb.mark_resetup_done'](resetup_id)

    def is_resetup_started(self, resetup_id):
        return __salt__['mongodb.is_resetup_started'](resetup_id)

    def mark_resetup_started(self, resetup_id):
        return __salt__['mongodb.mark_resetup_started'](resetup_id)

    def get_oplog_maxsize(self):
        return int(__salt__['mongodb.get_oplog_maxsize'](**self.conn_opts.as_dict()))

    def set_oplog_maxsize(self, max_size):
        return __salt__['mongodb.set_oplog_maxsize'](max_size=max_size, **self.conn_opts.as_dict())

    def is_alive(self, tries, timeout):
        '''
        Ping mongodb
        '''
        return __salt__['mongodb.is_alive'](tries, timeout, **self.conn_opts.as_dict())

    def get_last_stepdown_id(self):
        return __salt__['mongodb.get_last_stepdown_id']()

    def mark_stepdown_done(self, stepdown_id):
        return __salt__['mongodb.mark_stepdown_done'](stepdown_id)

    def rs_hosts(self, service=None):
        '''
        Get list of hosts in RS
        '''
        return __salt__['mdb_mongodb_helpers.replset_hosts'](service)

    def is_in_replicaset(self, master_hostname):
        # We don't need to try auth for mongodb.replset_initiated actually
        conn_noauth = self.conn_opts.copy()
        conn_noauth.user = None
        conn_noauth.password = None
        conn_noauth.authdb = None
        is_in_rs = __salt__['mongodb.replset_initiated'](**conn_noauth.as_dict())
        if not is_in_rs:
            return False
        if master_hostname is None:
            return True

        # Check that we in nneded replicaset and not in some other one
        return master_hostname in __salt__['mongodb.get_rs_members'](**self.conn_opts.as_dict())

    def add_to_replicaset(self, master_hostname, isArbiter, force):
        master_conn = self.conn_opts.copy()
        master_conn.host = master_hostname
        return __salt__['mongodb.replset_add'](
            '{}:{}'.format(self.conn_opts.host, self.conn_opts.port), isArbiter, force, **master_conn.as_dict()
        )


class MDBMongoReader(object):
    '''
    Abstract class for reading mongodb-related structures from pillar/mongodb/etc...
    '''

    def __init__(self):
        raise NotImplementedError("You shouldn't try to create objects of that type")

    @classmethod
    def read_role(cls, rolename=None, data=None):
        '''
        Read role, return MDBMongoDBRole object
        '''
        raise NotImplementedError

    @classmethod
    def read_roles(cls, data):
        '''
        Read role, return MDBMongoDBRolesList object
        '''
        raise NotImplementedError

    @classmethod
    def read_user_roles(cls, data):
        '''
        Read user roles, return MDBMongoDBUserRoles object
        '''
        raise NotImplementedError

    @classmethod
    def read_user(cls, username=None, data=None):
        '''
        Read user, return MDBMongoUser object
        '''
        raise NotImplementedError

    @classmethod
    def read_users(cls, data):
        '''
        read list of users, return MDBMongoUsersList
        '''
        raise NotImplementedError

    @classmethod
    def read_shard_list(cls, data):
        '''
        read list of shard names, return dict {shard_name: {options}}
        '''
        # TODO: In future, when we'll add ability to zone shards and so on,
        #  we'll need to convert it to MDBMongoShardsList class
        raise NotImplementedError

    @classmethod
    def read_databases_list(cls, data):
        '''
        read list of databases
        '''
        raise NotImplementedError


class MDBMongoPillarReader(MDBMongoReader):
    '''
    Read mongo data from pillar
    '''

    @classmethod
    def read_role(cls, rolename=None, data=None):
        if not isinstance(rolename, six.string_types):
            raise TypeError("rolename should be string")
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict")

        return MDBMongoDBRole(
            name=rolename,
            database=data['database'],
            privileges=data.get('privileges', None),
            roles=data.get('roles', None),
            authenticationRestrictions=data.get('authenticationRestrictions', None),
        )

    @classmethod
    def read_roles(cls, data):
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict")

        ret = MDBMongoDBRolesList()

        for k, v in data.items():
            ret.add_role(cls.read_role(k, v))

        return ret

    @classmethod
    def read_user_roles(cls, data):
        '''
        Read data from pillar and return MDBMongoDBUserRoles object.
        Pillar should looks like:
            admin:
                - root
                - dbOwner
            config:
                - dbOwner
            local:
                - dbOwner
        '''
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict[str,list]")

        ret = MDBMongoDBUserRoles()

        for db, roles in data.items():
            ret.add_roles(db, roles)
        return ret

    @classmethod
    def read_user(cls, username=None, data=None):
        '''
        Read user data from pillar and return MDBMongoUser object
        Pillar looks like:
            dbs:
                ----------
                admin:
                    - root
                    - dbOwner
                config:
                    - dbOwner
                local:
                    - dbOwner
            internal:
                True
            password:
                PASSWORD
            services:
                - mongod
                - mongos
                - mongocfg
        '''
        if not isinstance(username, six.string_types):
            raise TypeError("username should be string")
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict")

        user = MDBMongoUser(
            username=username,
            password=data['password'],
            internal=data.get('internal', False),
            services=data['services'][:],
            roles=cls.read_user_roles(data.get('dbs', {})),
        )

        return user

    @classmethod
    def read_users(cls, data):
        '''
        Read users from pillar and return MDBMongoUsersList object.
        Pillar is dict of users
        '''
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict")

        users = MDBMongoUsersList()
        for username, user_data in data.items():
            wrap_log('debug', 'reading user %s from pillar', username)
            users.add_user(cls.read_user(username, user_data))

        return users

    @classmethod
    def read_shard_list(cls, data):
        '''
        read list of shard names, return dict {shard_name: {options}}
        data should be result of
        ```salt-call pillar.get data:dbaas:cluster:subclusters``` (i.e. dict)
        '''
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict but {} given".format(type(data)))

        ret = {}
        for subcluster in data.values():
            if 'mongodb_cluster.mongod' in subcluster.get('roles', []):
                for shard in subcluster.get('shards', {}).values():
                    ret[shard['name']] = {'tags': set(shard.get('tags', [shard['name']]))}

        return ret

    @classmethod
    def read_databases_list(cls, data):
        '''
        read list of databases
        '''

        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict but {} given".format(type(data)))

        return list(data.keys())


class MDBMongoMongodbReader(MDBMongoReader):
    '''
    Read mongodb-related structures from mongo itself
    '''

    @classmethod
    def read_role(cls, rolename=None, data=None):
        if rolename is not None:
            raise ValueError("rolename should be None")
        if not isinstance(data, dict):
            raise TypeError("Pillar data should be dict")

        return MDBMongoDBRole(
            name=data['role'],
            database=data['db'],
            privileges=data.get('privileges', None),
            roles=data.get('roles', None),
            authenticationRestrictions=data.get('authenticationRestrictions', None),
        )

    @classmethod
    def read_roles(cls, data):
        if not isinstance(data, list):
            raise TypeError("MongoDB data should be list")

        ret = MDBMongoDBRolesList()

        for role in data:
            ret.add_role(cls.read_role(data=role))

        return ret

    @classmethod
    def read_user_roles(cls, data):
        pass

    @classmethod
    def read_user(cls, username=None, data=None):
        '''
        Read user data from mongodb, list is such format:
        {
            "_id" : "admin.admin",
            "user" : "admin",
            "db" : "admin",
            "credentials" : {...},
            "roles" : [
                {
                    "role" : "dbOwner",
                    "db" : "config"
                },
            ]
        }
        '''
        if username is not None:
            raise ValueError("username should be None")
        if not isinstance(data, dict):
            raise TypeError("MongoDB data should be dict")

        user = MDBMongoUser(data['user'])
        user.set_credentials(data.get('credentials', None))
        authdb = data['db']

        user.roles.add_database(authdb)

        for role in data['roles']:
            wrap_log('debug', 'adding role %s from mongodb data', role['role'])
            r = role['role']
            db = role['db']
            user.roles.add_role(db, r)

        return user

    @classmethod
    def read_users(cls, data):
        '''
        Read users list from mongodb, list is such format:
        {
            "_id" : "admin.admin",
            "user" : "admin",
            "db" : "admin",
            "credentials" : {...},
            "roles" : [
                {
                    "role" : "dbOwner",
                    "db" : "config"
                },
            ]
        }
        '''
        if not isinstance(data, list):
            raise TypeError("MongoDB data should be list[dict] but {} given".format(type(data)))

        users = MDBMongoUsersList()

        for user in data:
            wrap_log('debug', 'adding user %s from mongodb data', user['user'])
            users.add_user(cls.read_user(data=user))

        return users

    @classmethod
    def read_shard_list(cls, data):
        '''
        read list of shard names, return dict {shard_name: {options}}
        data should be result of
        mongodb.list_shards() (i.e. list of dicts)
        '''
        if not isinstance(data, list):
            raise TypeError("MongoDB data should be list")

        ret = {}
        for shard in data:
            ret[shard['_id']] = {
                'tags': set(shard.get('tags', [])),
            }

        return ret

    @classmethod
    def read_databases_list(cls, data):
        '''
        read list of databases
        '''

        if not isinstance(data, list):
            raise TypeError("MongoDB data should be list but {} given".format(type(data)))

        if len(data) < 1:
            raise ValueError("MongoDB database list can't be empty")

        return data[:]


###########################
# User Management Classes #
###########################


class MDBMongoDBUserRolesForSingleDB(object):
    '''List of roles for DB'''

    def __init__(self, database, roles=[]):
        self.database = database
        self.roles = set(roles)

    def __repr__(self):
        return "MDBMongoDBUserRolesForSingleDB({0}, {1})".format(*map(repr, (self.database, list(self.roles))))

    def get_database(self):
        '''Return database those roles are for'''
        return self.database

    def add_role(self, role):
        '''Add new role'''
        self.roles.add(role)

    def has_role(self, role):
        '''Check if we has such role for this DB'''
        return role in self.roles

    def remove_role(self, role):
        '''Remove Role'''
        if self.has_role(role):
            self.roles.remove(role)

    def is_empty(self):
        '''If we have any roles for our DB'''
        return len(self.roles) == 0

    def get_roles_list(self):
        '''Return list of roles'''
        return sorted(list(self.roles))

    def format_roles(self):
        '''Return list of roles in format MongoDB want them'''
        ret = []
        for role in self.roles:
            ret.append({'db': self.database, 'role': role})
        ret.sort(key=lambda x: (x.get('db'), x.get('role')))

        return ret

    def __iter__(self):
        '''For iteration - just iterate through roles'''
        return iter(self.roles)

    def __contains__(self, key):
        '''if X in MDBMongoDBUserRolesForSingleDB'''
        return self.has_role(key)

    def __iadd__(self, other):
        '''MDBMongoDBUserRolesForSingleDB += (Iterable|str)'''
        if isinstance(other, six.string_types):
            self.add_role(other)
        elif isinstance(other, collections_abc.Iterable):
            for role in other:
                self.add_role(role)
        else:
            raise TypeError(
                "MDBMongoDBUserRolesForSingleDB can add string or Iterable only but {0} is given".format(type(other))
            )
        return self

    def __isub__(self, other):
        '''MDBMongoDBUserRolesForSingleDB += (Iterable|str)'''
        if isinstance(other, six.string_types):
            self.remove_role(other)
        elif isinstance(other, collections_abc.Iterable):
            for role in other:
                self.remove_role(role)
        else:
            raise TypeError("MDBMongoDBUserRolesForSingleDB can sub string or Iterable only")
        return self

    def __eq__(self, other):
        '''MDBMongoDBUserRolesForSingleDB == MDBMongoDBUserRolesForSingleDB'''
        if isinstance(other, MDBMongoDBUserRolesForSingleDB):
            return self.roles == other.roles
        raise TypeError("MDBMongoDBUserRolesForSingleDB can be compared with MDBMongoDBUserRolesForSingleDB only")

    def __ne__(self, other):
        '''
        MDBMongoDBUserRolesForSingleDB != MDBMongoDBUserRolesForSingleDB
         (yes, we need separate method for it)
        '''
        return not self.__eq__(other)

    def get_changes_repr(self):
        '''Get roles representation for Salt changes'''
        return self.get_roles_list()


class MDBMongoDBUserRoles(object):
    '''List of roles of user'''

    def __init__(self, roles=None):
        self.roles = {}
        if roles is not None:
            for db, roles in roles.items():
                self.add_roles(db, roles)

    def __repr__(self):
        return "MDBMongoDBUserRoles({0})".format(repr(self.roles))

    def add_role(self, database, role):
        '''Add role to specified database to user'''
        self.add_database(database)
        self.roles[database] += role

    def add_roles(self, database, roles):
        '''Add multiple roles to specified database to user'''
        self.add_database(database)
        for role in roles:
            self.add_role(database, role)

    def remove_role(self, database, role):
        '''Remove single role for specified'''
        if database in self.roles:
            self.roles[database] -= role
            if self.roles[database].is_empty():
                # Not sure if we really need to delete database here
                self.roles.pop(database, None)

    def remove_roles(self, database, roles):
        '''Remove list of roles from specified DB'''
        for role in roles:
            self.remove_role(database, role)

        if self.roles[database].is_empty():
            # Not sure if we really need to delete database here
            self.roles.pop(database, None)

    def format_roles(self):
        ret = []
        for db in self.roles:
            ret += self.roles[db].format_roles()
        ret.sort(key=lambda x: (x.get('db'), x.get('role')))
        return ret

    def __iter__(self):
        # No need keys here as MDBMongoDBUserRolesForSingleDB contains db inside
        return iter(self.roles.values())

    def __sub__(self, other):
        '''MDBMongoDBUserRoles - MDBMongoDBUserRoles, return list of databases'''
        # We don't check if roles for specific DB are equal or not, we don't need it here
        if not isinstance(other, MDBMongoDBUserRoles):
            raise TypeError("MDBMongoDBUserRoles can sub MDBMongoDBUserRoles only")
        ret = []
        for db in self.roles:
            if db not in other.roles:
                ret.append(db)

        return ret

    def get_databases(self):
        return list(self.roles.keys())

    def add_database(self, database):
        '''Add database to roles if not exists (with no roles, but can be used as authdb)'''
        if database not in self.roles:
            self.roles[database] = MDBMongoDBUserRolesForSingleDB(database)

    def __getitem__(self, key):
        '''MDBMongoDBUserRoles[DB]'''
        return self.roles[key]

    def get_diff_dbs(self, other):
        '''
        Get list of databases,
         which are present in both collections but has diffirent set of roles
        '''
        ret = []
        for db in self.roles:
            if db in other.roles:
                if self.roles[db] != other.roles[db]:
                    ret.append(db)

        return ret

    def __eq__(self, other):
        '''MDBMongoDBUserRoles == MDBMongoDBUserRoles'''
        if isinstance(other, MDBMongoDBUserRoles):
            if set(self.roles.keys()) != set(other.roles.keys()):
                return False

            for db in self.roles:
                if self.roles[db] != other.roles[db]:
                    return False
            return True

        raise TypeError("MDBMongoDBUserRoles can be compared with MDBMongoDBUserRoles only")

    def __ne__(self, other):
        '''MDBMongoDBUserRoles != MDBMongoDBUserRoles'''
        return not self.__eq__(other)

    def __iadd__(self, other):
        '''MDBMongoDBUserRoles += MDBMongoDBUserRoles'''
        if isinstance(other, MDBMongoDBUserRoles):
            for db in other.roles:
                self.add_roles(db, other.roles[db])
        else:
            raise TypeError("MDBMongoDBUserRoles can add MDBMongoDBUserRoles only")
        return self

    def get_changes_repr(self):
        '''Get representation for Salt changes list'''
        ret = {}
        for db in self.roles:
            ret[db] = self.roles[db].get_changes_repr()
        return ret


class MDBMongoUser(object):
    '''Class for holding user retrieved from DB or pillar'''

    # authdb for internal users
    internal_authdb = 'admin'

    def __init__(
        self,
        username,
        password=None,
        internal=False,
        password_changed=False,
        roles=None,
        services=None,
        credentials=None,
    ):
        self.username = username
        self.password = password
        self.password_changed = password_changed
        self.internal = internal
        self.roles = MDBMongoDBUserRoles()
        if roles is not None:
            self.roles += roles
        if services is None:
            self.services = ['mongod', 'mongos']
        else:
            self.services = services[:]

        self.credentials = None
        self.set_credentials(credentials)

    def __repr__(self):
        return "MDBMongoUser({0}, {1}, {2}, {3}, {4}, {5}, {6})".format(
            *map(
                repr,
                (
                    self.username,
                    self.password,
                    self.internal,
                    self.password_changed,
                    self.roles,
                    self.services,
                    self.credentials,
                ),
            )
        )

    def get_some_authdb(self):
        '''Get some DB where user can perform authentification'''
        if self.internal:
            return self.internal_authdb
        return next(iter(self.roles)).get_database()

    def can_service(self, service):
        '''Check if user can use given service'''
        return service in self.services

    def set_credentials(self, credentials):
        '''Set user credentials'''
        self.credentials = deepcopy(credentials)
        if isinstance(self.credentials, dict):
            for algo in self.credentials.keys():
                # serverKey isn't needed for our purposes
                self.credentials[algo].pop('serverKey', None)

    def get_hashed_password(self, credentials, scram_algo='SCRAM-SHA-1'):
        '''
        Get hashed user password as it doing by MongoDB
        '''
        if not HAS_MONGOAUTH:
            return None

        # By some reason for SHA-1 mongo uses
        # md5(<user>:mongo:<password>) for generating password hash
        # but for SHA-256 it uses just
        # <password>
        # Reference: sources of pymongo and nodejs mongodb driver
        # https://github.com/mongodb/node-mongodb-native/blob/f71f09bd466f0630bbe6859d8ed074ecd5f4a51f/lib/cmap/auth/scram.js#L123
        def sha256_digest(username, password):
            return ensure_text(password)

        def sha1_digest(username, password):
            return mongo_auth._password_digest(ensure_text(username), ensure_text(password))

        algo_map = {
            'SCRAM-SHA-1': ('sha1', sha1, sha1_digest),
            'SCRAM-SHA-256': ('sha256', sha256, sha256_digest),
        }
        if scram_algo not in algo_map:
            raise ValueError("Unsupported auth algorithm: {}".format(scram_algo))

        algo, hash_fn, digest_fn = algo_map[scram_algo]

        # Stealed this code from pymongo.auth._authenticate_scram_sha1
        salted_pass = pbkdf2_hmac(
            algo,
            digest_fn(
                self.username,
                self.password,
            ).encode('utf-8'),
            standard_b64decode(credentials['salt']),
            credentials['iterationCount'],
        )
        client_key = hmac.HMAC(salted_pass, b"Client Key", hash_fn).digest()
        stored_key = hash_fn(client_key).digest()
        return ensure_str(standard_b64encode(stored_key))

    def fill_credentials(
        self,
        b64_salt,
        iterationCount,
        scram_algo='SCRAM-SHA-1',
    ):
        """
        Fill users credentials with given salt, iterations count and algo
        """
        if not HAS_MONGOAUTH:
            return None

        credentials = {
            'iterationCount': int(iterationCount),
            'salt': b64_salt,
            'storedKey': None,
        }
        credentials['storedKey'] = self.get_hashed_password(credentials, scram_algo)

        if self.credentials is None:
            self.credentials = {}
        self.credentials[scram_algo] = credentials

    def fill_credentials_from_user(self, otherUser):
        if not isinstance(otherUser.credentials, dict):
            return
        for algo, credentials in otherUser.credentials.items():
            self.fill_credentials(
                b64_salt=credentials['salt'],
                iterationCount=credentials['iterationCount'],
                scram_algo=algo,
            )

    def __eq__(self, other):
        '''MDBMongoUser == MDBMongoUser'''
        if isinstance(other, MDBMongoUser):
            return (
                self.password_changed is False
                and other.password_changed is False
                and (self.password is None or other.password is None or self.password == other.password)
                and self.roles == other.roles
                and self.username == other.username
                and _compare_structs(self.credentials, other.credentials)
            )

        raise TypeError("MDBMongoUser can be compared with MDBMongoUser only")

    def __ne__(self, other):
        '''MDBMongoUser != MDBMongoUser'''
        return not self.__eq__(other)

    def merge(self, other):
        '''Merge 2nd user to this, except of password and username'''
        if not isinstance(other, MDBMongoUser):
            raise TypeError("MDBMongoUser can be merged with MDBMongoUser only")

        if other.internal:
            self.internal = True
        if other.password_changed:
            self.password_changed = other.password_changed
        self.roles += other.roles

        if self.credentials is None:
            self.credentials = other.credentials

    def copy(self):
        '''Get copy of user'''
        ret = MDBMongoUser(self.username)
        ret.password = self.password
        ret.internal = self.internal
        ret.password_changed = self.password_changed
        ret.services = self.services[:]
        ret.roles += self.roles

        return ret

    def set_internal(self, internal):
        '''Set internal flag'''
        self.internal = internal

    def get_changes_repr(self):
        '''Get representation for Salt changes list'''
        ret = {'password': '***', 'roles': self.roles.get_changes_repr()}
        if self.password_changed:
            ret['password'] = '******'
        return ret

    def copy_internal_from(self, user):
        '''
        Copy internal flag from user
        '''
        if not isinstance(user, MDBMongoUser):
            raise TypeError("user argument should be MDBMongoUser")
        self.set_internal(user.internal)


class MDBMongoUsersList(object):
    '''Class for holding list of user retrieved from DB or pillar'''

    def __init__(self, users=None):
        self.users = {}
        if users is not None:
            for user in users.values():
                self.add_user(user, True)

    def __repr__(self):
        return "MDBMongoUsersList({0})".format(repr(self.users))

    def add_user(self, user, do_copy=False):
        '''Add or merge user'''
        wrap_log('debug', 'adding or merging user %s', user.username)
        if isinstance(user, MDBMongoUser):
            if user.username in self.users:
                self.users[user.username].merge(user)
            else:
                if do_copy:
                    self.users[user.username] = user.copy()
                else:
                    self.users[user.username] = user
        elif isinstance(user, six.string_types):
            if user not in self.users:
                self.users[user] = MDBMongoUser(user)
        else:
            raise TypeError('Added user should be string or MDBMongoUser')

    def get_user(self, key):
        if isinstance(key, MDBMongoUser):
            key = key.username
        return self.users[key]

    def __contains__(self, key):
        if isinstance(key, MDBMongoUser):
            key = key.username
        return key in self.users

    def __iter__(self):
        '''https://docs.python.org/3/reference/datamodel.html#object.__iter__'''
        return iter(self.users.values())

    def __getitem__(self, key):
        '''https://docs.python.org/3/reference/datamodel.html#object.__getitem__'''
        return self.get_user(key)

    def __setitem__(self, key, value):
        '''https://docs.python.org/3/reference/datamodel.html#object.__getitem__'''
        if not isinstance(value, MDBMongoUser):
            raise TypeError("Only MDBMongoUser is accessible as item value for MDBMongoUsersList")
        if key in self.users:
            self.users.pop(key, None)
        return self.add_user(value)

    def __len__(self):
        return len(self.users)

    def keys(self):
        return list(self.users.keys())

    def __eq__(self, other):
        '''MDBMongoUsersList == MDBMongoUsersList'''
        if not isinstance(other, MDBMongoUsersList):
            raise TypeError('MDBMongoUsersList can be compared with MDBMongoUsersList only')

        if set(self.keys()) != set(other.keys()):
            return False

        for user in self.users:
            if self[user] != other[user]:
                return False

        return True

    def __ne__(self, other):
        '''MDBMongoUsersList != MDBMongoUsersList'''
        return not self.__eq__(other)

    def __sub__(self, other):
        '''MDBMongoUsersList - MDBMongoUsersList, return MDBMongoUsersList'''
        # We don't check if specific users are equal or not, we don't need it here
        if not isinstance(other, MDBMongoUsersList):
            raise TypeError("MDBMongoUsersList can sub MDBMongoUsersList only")
        ret = MDBMongoUsersList()
        for user in self.users:
            if user not in other.users:
                ret.add_user(self.users[user])

        return ret

    def get_diff_users(self, other):
        '''
        Get list of users from our collection,
         which present in both collections but aren't equal
        '''
        ret = MDBMongoUsersList()
        for user in self.users:
            if user in other.users:
                if self.users[user] != other.users[user]:
                    ret.add_user(self.users[user])

        return ret

    def filter_service(self, service, do_copy=False):
        '''
        Return only users, who can use service `service`
        '''
        ret = MDBMongoUsersList()
        for user in self.users.values():
            if user.can_service(service):
                ret.add_user(user, do_copy)

        return ret

    def filter_users_with_roles(self, do_copy=False):
        '''
        Return only users, who have some role in DB
        '''
        ret = MDBMongoUsersList()
        for user in self.users.values():
            if len(user.roles.get_databases()) > 0:
                ret.add_user(user, do_copy)

        return ret

    def get_changes_repr(self):
        '''Get representation of users list for Salt change list'''
        ret = {}
        for user in self.users:
            ret[user] = self.users[user].get_changes_repr()
        return ret

    def copy_internal_from(self, users):
        '''
        Copy internal flag for each user
        '''
        if not isinstance(users, MDBMongoUsersList):
            raise TypeError("MDBMongoUsersList.copy_internal_from() argument `users` should be  MDBMongoUsersList")

        for user in self.users:
            if user in users.users:
                self.users[user].copy_internal_from(users.users[user])


############################
# Roles Management Classes #
############################


class MDBMongoDBRole(object):
    '''Class for holding MongoDB role'''

    def __init__(self, name, database='admin', privileges=None, roles=None, authenticationRestrictions=None):
        self.name = name
        self.database = database

        # We do not parse privileges at all, just pass it as is to MongoDB
        if privileges is not None:
            self.privileges = deepcopy(privileges)
            _normalize_inplace(self.privileges)
            _sort_list_recursive(
                self.privileges,
                lambda x: (
                    x.get('resource', {}).get('db', ''),
                    x.get('resource', {}).get('collection', ''),
                    x.get('resource', {}).get('anyResource', False),
                    x.get('actions', []).sort(),
                )
                if isinstance(x, dict)
                else x,
            )
        else:
            self.privileges = []

        if authenticationRestrictions:
            self.authenticationRestrictions = deepcopy(authenticationRestrictions)
            _normalize_inplace(self.authenticationRestrictions)
            _sort_list_recursive(
                self.authenticationRestrictions,
                lambda x: (
                    x.get('clientSource', []).sort(),
                    x.get('serverAddress', []).sort(),
                )
                if isinstance(x, dict)
                else x,
            )
        else:
            self.authenticationRestrictions = []

        self.roles = MDBMongoDBUserRoles()
        if roles is not None:
            self.set_roles(roles)

    def __repr__(self):
        return "MDBMongoDBRole({},{},{},{},{})".format(
            *map(repr, (self.name, self.database, self.privileges, self.roles, self.authenticationRestrictions))
        )

    def copy(self):
        '''Return copy of seelf'''
        return MDBMongoDBRole(
            self.name,
            self.privileges,
            self.roles,
            self.authenticationRestrictions,
        )

    def set_roles(self, roles=None):
        '''Set role roles'''

        # First, clear old roles
        self.roles = MDBMongoDBUserRoles()
        if roles is None:
            return

        if isinstance(roles, MDBMongoDBUserRoles):
            self.roles += roles
        elif isinstance(roles, list):
            for role in roles:
                if isinstance(role, six.string_types):
                    self.roles.add_role(self.database, role)
                elif isinstance(role, dict):
                    self.roles.add_role(role['db'], role['role'])
                else:
                    raise TypeError("Each element of roles list should be string or dict")
        else:
            raise TypeError('roles should be None, list or MDBMongoDBUserRoles')

    def get_changes_repr(self):
        '''Get representation for Salt changes list'''
        ret = {
            'name': self.name,
            'database': self.database,
            'privileges': self.privileges,
            'roles': self.roles.get_changes_repr(),
        }
        return ret

    def __eq__(self, other):
        '''MDBMongoDBRole == MDBMongoDBRole'''
        if not isinstance(other, MDBMongoDBRole):
            raise TypeError('MDBMongoDBRole can be compared with MDBMongoDBRole only')

        if (
            self.name != other.name
            or self.database != other.database
            or self.roles != other.roles
            or not _compare_structs(self.privileges, other.privileges)
            or not _compare_structs(self.authenticationRestrictions, other.authenticationRestrictions)
        ):
            return False

        return True

    def __ne__(self, other):
        '''MDBMongoDBRole != MDBMongoDBRole'''
        return not self.__eq__(other)


class MDBMongoDBRolesList(object):
    '''Class for holding list of MongoDB roles'''

    def __init__(self, roles=None):
        self.roles = {}
        if roles is not None:
            for role in roles.values():
                self.add_role(role, True)

    def __repr__(self):
        return "MDBMongoDBRolesList({0})".format(repr(self.roles))

    @staticmethod
    def _get_role_key(role):
        if isinstance(role, MDBMongoDBRole):
            return '{name}.{database}'.format(
                name=role.name,
                database=role.database,
            )
        else:
            return role

    def add_role(self, role, do_copy=False):
        '''Add or merge role'''
        if isinstance(role, MDBMongoDBRole):
            if do_copy:
                self.roles[self._get_role_key(role)] = role.copy()
            else:
                self.roles[self._get_role_key(role)] = role
        elif isinstance(role, six.string_types):
            if role not in self.roles:
                self.roles[self._get_role_key(role)] = MDBMongoDBRole(role)
        else:
            raise TypeError('Added role should be string or MDBMongoDBRole')

    def get_role(self, key):
        return self.roles[self._get_role_key(key)]

    def __contains__(self, key):
        return self._get_role_key(key) in self.roles

    def __iter__(self):
        return iter(self.roles.values())

    def __getitem__(self, key):
        return self.get_role(key)

    def __setitem__(self, key, value):
        if not isinstance(value, MDBMongoDBRole):
            raise TypeError("Only MDBMongoDBRole is accessible as item value for MDBMongoDBRolesList")
        if self._get_role_key(key) in self.roles:
            self.roles.pop(self._get_role_key(key), None)
        return self.add_role(value)

    def __len__(self):
        return len(self.roles)

    def keys(self):
        return list(self.roles.keys())

    def __eq__(self, other):
        '''MDBMongoDBRolesList == MDBMongoDBRolesList'''
        if not isinstance(other, MDBMongoDBRolesList):
            raise TypeError('MDBMongoDBRolesList can be compared with MDBMongoDBRolesList only')

        if set(self.keys()) != set(other.keys()):
            return False

        for role in self.roles:
            if self[role] != other[role]:
                return False

        return True

    def __ne__(self, other):
        '''MDBMongoDBRolesList != MDBMongoDBRolesList'''
        return not self.__eq__(other)

    def __sub__(self, other):
        '''MDBMongoDBRolesList - MDBMongoDBRolesList, return MDBMongoDBRolesList'''
        # We don't check if specific roles are equal or not, we don't need it here
        if not isinstance(other, MDBMongoDBRolesList):
            raise TypeError("MDBMongoDBRolesList can sub MDBMongoDBRolesList only")
        ret = MDBMongoDBRolesList()
        for role in self.roles:
            if role not in other.roles:
                ret.add_role(self.roles[role])

        return ret

    def get_diff_roles(self, other):
        '''
        Get list of roles from our collection,
         which present in both collections but aren't equal
        '''
        ret = MDBMongoDBRolesList()
        for role in self.roles:
            if role in other.roles:
                if self.roles[role] != other.roles[role]:
                    ret.add_role(self.roles[role])

        return ret

    def get_changes_repr(self):
        '''Get representation of roles list for Salt change list'''
        ret = {}
        for role in self.roles:
            ret[role] = self.roles[role].get_changes_repr()
        return ret


##########
# States #
##########


@return_false_on_exception
@exec_on_master_or_wait(120)
def ensure_users(name, **kwargs):
    '''
    Ensure that needed users (and only them) exist in MongoDB rs
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)
    service = kwargs.get('service', 'mongod')

    # Get New users and currently present users
    _pillar = __salt__['pillar.get'](PILLAR_PATH_USERS)
    users = MDBMongoPillarReader.read_users(_pillar).filter_service(service).filter_users_with_roles()

    log.log(0, 'Service: %s', service)

    _current_users = mongodb.list_users()
    wrap_log('debug', 'listing db users is over')
    current_users = MDBMongoMongodbReader.read_users(_current_users)
    current_users.copy_internal_from(users)

    wrap_log('info', 'Old List of users: %s', ', '.join(current_users.keys()))
    wrap_log('info', 'New List of users list after service filter: %s', ', '.join(users.keys()))

    wrap_log_repr('debug', "users: %s", users)
    wrap_log_repr('debug', "current_users: %s", current_users)

    # Get collection diffs, i.e. find which users we need to add/modify/remove

    remove_users = current_users - users
    add_users = users - current_users

    for user in users:
        if user in current_users:
            user.fill_credentials_from_user(current_users[user])
            mongodb.check_user_password(
                user,
                current_users[user].get_some_authdb(),
                target_credentials=current_users[user].credentials,
            )

    modify_users = users.get_diff_users(current_users)

    log.info("%d new users found: %s", len(add_users), ', '.join(add_users.keys()))
    log.info("%d users will be deleted: %s", len(remove_users), ', '.join(remove_users.keys()))
    log.info("%d users will be modified: %s", len(modify_users), ', '.join(modify_users.keys()))

    if not any((add_users, remove_users, modify_users)):
        ret['result'] = True
        return ret

    changes = {
        'New users': add_users.get_changes_repr(),
        'Deleted users': list(remove_users.keys()),
        'Modified users': modify_users.get_changes_repr(),
    }

    if __opts__['test']:
        ret['result'] = None
        ret['changes'] = changes
        ret['comment'] = '{0} users would be added, {1} deleted and {2} modified'.format(
            len(add_users), len(remove_users), len(modify_users)
        )
        return ret

    for user in add_users:
        mongodb.add_user(user)
    for user in modify_users:
        mongodb.update_user(user, current_users[user])
    for user in remove_users:
        mongodb.delete_user(user)

    ret['result'] = True
    ret['changes'] = changes
    ret['comment'] = '{0} users were added, {1} deleted and {2} modified'.format(
        len(add_users), len(remove_users), len(modify_users)
    )
    return ret


@return_false_on_exception
@exec_on_master_or_wait(120)
def ensure_roles(name, roles=None, userdb_roles=None, **kwargs):
    '''
    Ensure that needed roles (and only them) are exists in MongoDB rs
    This is internal function for ensure_roles

    where:
    * roles - statically defined foles
    * user_db_roles - roles which should be created foreach user's database,
       placeholder {db} will be raplaced to database's name
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    # Get New roles and currently present roles
    wrap_log_repr('log', 0, 'Pillar: %s', roles)
    roles = MDBMongoPillarReader.read_roles(roles)
    if userdb_roles is None:
        userdb_roles = {}
    userdb_roles = MDBMongoPillarReader.read_roles(userdb_roles)

    _current_roles = mongodb.list_roles()
    wrap_log_repr('log', 0, 'Roles list from mongo: %s', _current_roles)
    current_roles = MDBMongoMongodbReader.read_roles(_current_roles)

    db_pillar = __salt__['pillar.get'](PILLAR_PATH_DATABASES)
    target_dbs = MDBMongoPillarReader.read_databases_list(db_pillar)

    # For each role in userdb_roles, need to generate real role and put it into roles
    for role in userdb_roles:
        # For each database in pillar
        for db in target_dbs:
            format_dict = {'db': db}
            new_role_roles = MDBMongoDBUserRoles()
            # Role can have link to other roles, which are dict: {db: [role, role, ...]}
            # So we need to iterate thru each db and each role
            for role_db in role.roles.get_databases():
                new_role_db = role_db.format(**format_dict)
                for db_role in role.roles[role_db].get_roles_list():
                    new_role_roles.add_role(
                        new_role_db,
                        db_role.format(**format_dict),
                    )

            roles.add_role(
                MDBMongoDBRole(
                    name=role.name.format(**format_dict),
                    database=role.database.format(**format_dict),
                    privileges=_deep_format(role.privileges, **format_dict),
                    roles=new_role_roles,
                    authenticationRestrictions=_deep_format(role.authenticationRestrictions, **format_dict),
                )
            )

    wrap_log('info', 'Old List of roles: %s', ', '.join(current_roles.keys()))
    wrap_log('info', 'New List of roles: %s', ', '.join(roles.keys()))

    wrap_log_repr('debug', "roles: %s", roles)
    wrap_log_repr('debug', "current_roles: %s", current_roles)
    # Get collection diffs, i.e. find which roles we need to add/modify/remove

    remove_roles = current_roles - roles
    add_roles = roles - current_roles

    modify_roles = roles.get_diff_roles(current_roles)

    log.info("%d new roles found: %s", len(add_roles), ', '.join(add_roles.keys()))
    log.info("%d roles will be deleted: %s", len(remove_roles), ', '.join(remove_roles.keys()))
    log.info("%d roles will be modified: %s", len(modify_roles), ', '.join(modify_roles.keys()))

    if not any((add_roles, remove_roles, modify_roles)):
        ret['result'] = True
        return ret

    changes = {'old': current_roles.get_changes_repr(), 'new': roles.get_changes_repr()}
    ret['changes'] = {'diff': _yaml_diff(changes['old'], changes['new'])}
    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = '{0} roles would be added, {1} deleted and {2} modified'.format(
            len(add_roles), len(remove_roles), len(modify_roles)
        )
        return ret

    for role in add_roles:
        mongodb.add_role(role)
    for role in remove_roles:
        mongodb.delete_role(role)
    for role in modify_roles:
        mongodb.update_role(role)

    ret['result'] = True
    ret['comment'] = '{0} roles were added, {1} deleted and {2} modified'.format(
        len(add_roles), len(remove_roles), len(modify_roles)
    )
    return ret


@return_false_on_exception
def ensure_shard_zones(name, **kwargs):
    '''
    Ensure, that shards are assigned to needed zones
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    _pillar = __salt__['pillar.get'](PILLAR_PATH_SUBCLUSTERS)
    target_shards = MDBMongoPillarReader.read_shard_list(_pillar)

    shards = MDBMongoMongodbReader.read_shard_list(mongodb.list_shards())

    tags_blacklist = set(target_shards.keys())

    shards_to_fix = {}

    # For each shard check if it is present in zone with the same name as shard name
    for shard_name, shard in shards.items():
        if shard_name not in target_shards:
            log.debug('Unexpected shard name: %s', shard_name)
            continue

        add_tags = list(target_shards[shard_name]['tags'] - shard['tags'])
        del_tags = list(shard['tags'] & (tags_blacklist - set([shard_name])))
        if add_tags or del_tags:
            # We need to delete "blacklisted" tags so our SLI collections will work properly
            shards_to_fix[shard_name] = {
                'add': add_tags,
                'del': del_tags,
            }

    if len(shards_to_fix) == 0:
        return {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    if __opts__['test']:
        return {
            'name': name,
            'changes': shards_to_fix,
            'result': None,
            'comment': 'Zones for {} shards would be fixed'.format(len(shards_to_fix)),
        }

    for shard_name, data in shards_to_fix.items():
        for tag in data['add']:
            mongodb.add_shard_to_zone(shard_name, tag)

        for tag in data['del']:
            mongodb.remove_shard_from_zone(shard_name, tag)

    return {
        'name': name,
        'changes': shards_to_fix,
        'result': True,
        'comment': 'Zones for {} shards were fixed'.format(len(shards_to_fix)),
    }


@return_false_on_exception
@retry_on_fail(15, 60, 5)
def ensure_check_collections(name, db='mdb_internal', prefix='check_', **kwargs):
    '''
    Ensure that our internal db is sharded
     and needed collections are present, sharded and properly configured
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    # First of all, we need to enable sharding of our DB
    if db not in mongodb.list_sharded_databases():
        if __opts__['test']:
            return {
                'name': name,
                'changes': db,
                'result': None,
                'comment': 'Database {} would be created and sharded'.format(db),
            }
        else:
            mongodb.shard_database(db)

    new_collections = {}
    current_collections = mongodb.list_collections(db)
    sharded_collections = mongodb.list_sharded_collections(db)
    collections_tags = mongodb.get_collections_and_tags(db)

    _pillar = __salt__['pillar.get'](PILLAR_PATH_SUBCLUSTERS)
    shards = MDBMongoPillarReader.read_shard_list(_pillar)
    collections_to_drop = []

    # For each shard check if we have needed collection
    for shard in shards.keys():
        col_name = '{}{}'.format(prefix, shard)
        # If there is no such collection, create it
        if col_name not in current_collections:
            new_collections[col_name] = {'zone': shard, 'recreate': False}
        # If there is such collection, but it is not sharded or not properly configured, recreate it
        elif (
            (col_name not in sharded_collections)
            or (col_name not in collections_tags)
            or (len(collections_tags[col_name]) != 1)
            or (shard not in collections_tags[col_name])
            or (not mongodb.has_ttl_index(db, col_name, 'd'))
        ):

            new_collections[col_name] = {'zone': shard, 'recreate': True}

    # List drop collections which starts with `prefix` and doesn't need for our purposes.
    for collection in current_collections:
        if collection.startswith(prefix):
            cname = collection[len(prefix) :]
            if cname not in shards.keys():
                collections_to_drop.append(collection)

    if len(new_collections) == 0 and len(collections_to_drop) == 0:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': '',
        }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'[re]create': list(new_collections.keys()), 'drop': collections_to_drop},
            'result': None,
            'comment': '{} collections would be created or recreated and {} dropped'.format(
                len(new_collections),
                len(collections_to_drop),
            ),
        }

    for col, data in new_collections.items():
        if data['recreate']:
            mongodb.drop_collection(db, col)

        mongodb.shard_collection_to_zone(db, col, 'ts', data['zone'])
        mongodb.create_ttl_index(db, col, 'd', 60 * 60 * 24)

    for col in collections_to_drop:
        mongodb.drop_collection(db, col)

    return {
        'name': name,
        'changes': {'[re]create': list(new_collections.keys()), 'drop': collections_to_drop},
        'result': True,
        'comment': '{} collections were be created or recreated and {} dropped'.format(
            len(new_collections), len(collections_to_drop)
        ),
    }


@return_false_on_exception
def ensure_balancer_state(name, started, **kwargs):
    '''
    Ensure that mongos balancer is in needed state
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    currState = mongodb.get_balancer_state()

    if currState == started:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': '',
        }

    actionStr = 'started' if started else 'stoped'

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'balancer.state': actionStr},
            'result': None,
            'comment': 'mongos balancer would be {}'.format(actionStr),
        }

    mongodb.set_balancer_state(started)

    return {
        'name': name,
        'changes': {'balancer.state': actionStr},
        'result': True,
        'comment': 'mongos balancer was {}'.format(actionStr),
    }


@return_false_on_exception
def ensure_balancer_active_window(name, start, stop, **kwargs):
    '''
    Ensure that mongos balancer has balancing window
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)
    window = {'start': start, 'stop': stop}

    curr_window = mongodb.get_balancer_active_window()

    if curr_window == window:
        return {
            'name': name,
            'result': True,
            'changes': {},
            'comment': '',
        }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'balancer.active_window': window},
            'result': None,
            'comment': 'mongos balancer active window would be {}'.format(window),
        }

    mongodb.set_balancer_active_window(start, stop)

    return {
        'name': name,
        'changes': {'balancer.active_window': window},
        'result': True,
        'comment': 'mongos balancer active window was {}'.format(curr_window),
    }


@return_false_on_exception
def ensure_balancer_active_window_disabled(name, **kwargs):
    '''
    Ensure mongos balancer balancing window is not set
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    curr_window = mongodb.get_balancer_active_window()

    if not curr_window:
        return {
            'name': name,
            'result': True,
            'changes': {},
            'comment': '',
        }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'balancer.active_window': 'unset'},
            'result': None,
            'comment': 'mongos balancer active window would be unset',
        }

    mongodb.unset_balancer_active_window()

    return {
        'name': name,
        'changes': {'balancer.active_window': 'unset'},
        'result': True,
        'comment': 'mongos balancer active window was {}'.format(curr_window),
    }


@return_false_on_exception
@exec_on_master_or_wait(120)
def ensure_databases(name, **kwargs):
    '''
    Ensure, that all needed databases exists and only them
    '''

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    curr_dbs = MDBMongoMongodbReader.read_databases_list(mongodb.list_databases())
    _pillar = __salt__['pillar.get'](PILLAR_PATH_DATABASES)
    target_dbs = MDBMongoPillarReader.read_databases_list(_pillar)

    res = {
        'name': name,
        'changes': {},
        'result': False,
        'comment': '',
    }

    curr_user_dbs = [db for db in curr_dbs if db not in INTERNAL_DBS]

    if (len(target_dbs) < 1) and (len(curr_user_dbs) >= 2):
        raise ValueError(
            "You are trying to delete all user databases, but there is more than one user database present, this should not happend. Manual actions needed."
        )

    del_dbs = [db for db in curr_user_dbs if db not in target_dbs]

    if len(del_dbs) == 0:
        res['result'] = True
        return res

    if len(del_dbs) > 1:
        res[
            'comment'
        ] = 'Trying to delete more than one database: {}, this should not happend. Manual actions needed.'.format(
            repr(del_dbs)
        )
        return res

    target_database = __salt__['pillar.get']('target-database', '')
    if del_dbs[0] != target_database:
        res[
            'comment'
        ] = 'would delete database {}, but {} was requested, this should not happend. Manual actions needed.'.format(
            del_dbs[0], target_database
        )
        return res

    if __opts__['test']:
        res.update(
            {
                'changes': {'drop': del_dbs},
                'result': None,
                'comment': '{} databases would be dropped'.format(len(del_dbs)),
            }
        )
        return res

    for db in del_dbs:
        mongodb.drop_database(db)

    res.update(
        {
            'changes': {'drop': del_dbs},
            'result': True,
            'comment': '{} databases were dropped'.format(len(del_dbs)),
        }
    )
    return res


@return_false_on_exception
@retry_on_fail(5, 300, 10)
def wait_for_resetup(name, resetup_id=None, **kwargs):
    '''
    Wait for host to complete resetup/initial sync if any
    '''

    if kwargs.get('service', 'mongod') not in ['mongod', 'mongocfg']:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'No need to check resetup status for service {service}'.format(
                service=kwargs['service'],
            ),
        }

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    if __opts__['test']:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': '',
        }

    (stdout, stderr) = mongodb.resetup(continue_flag=True)
    if resetup_id is not None:
        mongodb.mark_resetup_done(resetup_id)

    return {
        'name': name,
        'changes': {},
        'result': True,
        'comment': 'Host now in readable state',
    }


@return_false_on_exception
def ensure_resetup_host(name, resetup_id=None, **kwargs):
    '''
    Resetup host.
    But if resetup_id == resetup_id of previous (successful) resetup,
     then do nothing (it could happend in case of resetup task has been restarted by some reason)
    '''

    if kwargs.get('service', 'mongod') not in ['mongod', 'mongocfg']:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'No resetup needed for service {service}'.format(service=kwargs['service']),
        }

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    if mongodb.is_resetup_started(resetup_id):
        return wait_for_resetup(name, resetup_id, **kwargs)

    # Check if last resetup had the same resetup_id
    if resetup_id is not None:
        last_resetup_id = mongodb.get_last_resetup_id()
        if last_resetup_id == resetup_id:
            return {
                'name': name,
                'changes': {},
                'result': True,
                'comment': 'Resetup was already done',
            }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'resetup': True},
            'result': None,
            'comment': 'Host would be resetupped',
        }

    res = {'resetup': True}

    # If we are primary, then need to stepdown first
    if mongodb.is_master():
        mongodb.stepdown()
        res['stepdown'] = True

    if mongodb.is_master():
        raise Exception("MongoDB is still primary after stepdown")

    mongodb.mark_resetup_started(resetup_id)
    (stdout, stderr) = mongodb.resetup(force=True)
    mongodb.mark_resetup_done(resetup_id)

    return {
        'name': name,
        'changes': res,
        'result': True,
        'comment': 'Host resetup completed successfully\n\nstdout:\n{stdout}\n\nstderr:\n{stderr}'.format(
            stdout=stdout, stderr=stderr
        ),
    }


@return_false_on_exception
def oplog_maxsize(name, max_size, **kwargs):
    '''
    https://docs.mongodb.com/v3.6/tutorial/change-oplog-size/
    Set maximum oplog size (in megabytes)

    max_size
        New oplog max size in megabytes
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    current_oplog_max_size = mongodb.get_oplog_maxsize()

    if current_oplog_max_size == int(max_size):
        ret['result'] = True
        return ret

    if __opts__['test']:
        ret['result'] = None
        ret['comment'] = '{0} max oplog size would be changed to {1}'.format(name, max_size)
        ret['changes'] = {'db.oplog.rs.stats().maxSize': {'old': current_oplog_max_size, 'new': max_size}}
        return ret

    result = mongodb.set_oplog_maxsize(max_size)
    if result:
        ret['result'] = True
        ret['changes'] = {'db.oplog.rs.stats().maxSize': {'old': current_oplog_max_size, 'new': max_size}}
    else:
        ret['comment'] = 'Something went wrong, {0} max oplog size wasn\'t  changed to {1}'.format(name, max_size)

    return ret


@return_false_on_exception
def wait_user_created(name, timeout, **kwargs):
    '''
    Wait up to `timeout` seconds for user `user` (defualt: admin) being created
     (or replicaded from primary if we are replica)
    '''
    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    ret = {'name': name, 'changes': {}, 'result': False, 'comment': 'Auth failed'}

    stop_ts = time.time() + timeout
    while time.time() < stop_ts:
        try:
            auth_ok = mongodb.auth_user(conn_opts.user, conn_opts.authdb, conn_opts.password, True)
            if auth_ok:
                ret.update({'result': True, 'comment': ''})
                return ret
        except Exception as exc:
            # Remember last error message in case we'll fail to wait for user
            ret['comment'] = str(exc)
        time.sleep(5)

    return ret


@return_false_on_exception
def alive(name, tries=None, timeout=None, **kwargs):
    '''
    Check if mongodb alive

    tries
        How many times try to ping mongodb

    timeout
        ping timeout
    '''
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    is_alive = mongodb.is_alive(tries, timeout)
    if not is_alive:
        ret['comment'] = 'MongoDB at {0}:{1} seems dead'.format(conn_opts.host, conn_opts.port)
    ret['result'] = is_alive
    return ret


@return_false_on_exception
def ensure_stepdown_host(name, stepdown_id=None, **kwargs):
    '''
    Stepdown host.
    But if stepdown_id == stepdown_id of previous (successful) stepdown,
     then do nothing (it could happend in case of stepdown task has been restarted by some reason)
    '''

    if kwargs.get('service', 'mongod') not in ['mongod', 'mongocfg']:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'No stepdown needed for service {service}'.format(service=kwargs['service']),
        }

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    if len(mongodb.rs_hosts(kwargs.get('service', 'mongod'))) <= 1:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'Only one host in replica set, stepdown is not possible',
        }

    # Check if last stepdown had the same stepdown_id
    if stepdown_id is not None:
        last_stepdown_id = mongodb.get_last_stepdown_id()
        if last_stepdown_id == stepdown_id:
            return {
                'name': name,
                'changes': {},
                'result': True,
                'comment': 'Stepdown was already done',
            }

    # If we is not primary, then we do not need to do anything
    if not mongodb.is_master():
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'Node is not PRIMARY, no stepdown needed',
        }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'stepdown': True},
            'result': None,
            'comment': 'Host would be stepdowned',
        }

    res = {'stepdown': True}

    mongodb.stepdown()

    if mongodb.is_master():
        raise Exception("MongoDB is still primary after stepdown")

    mongodb.mark_stepdown_done(stepdown_id)

    return {
        'name': name,
        'changes': res,
        'result': True,
        'comment': 'Host stepdown completed successfully',
    }


@return_false_on_exception
def ensure_member_in_replicaset(name, master_hostname=None, arbiter=False, force=False, **kwargs):
    '''
    Ensure that given memeber is in replicaset.
    master_hostname is hostname of RS master.
    '''
    if kwargs.get('service', 'mongod') not in ['mongod', 'mongocfg']:
        return {
            'name': name,
            'changes': {},
            'result': False,
            'comment': 'You should not try to add "{service}" to replicaset'.format(service=kwargs['service']),
        }

    conn_opts = MDBMongoConnectionOptions.read_from_pillar(**kwargs)
    mongodb = MDBMongoMongodbInterface(conn_opts)

    if mongodb.is_in_replicaset(master_hostname):
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'We are already in needed replicaset',
        }

    if master_hostname is None:
        return {
            'name': name,
            'changes': {},
            'result': False,
            'comment': 'Replicaset master is not found, can not add member to replicaset',
        }

    if __opts__['test']:
        return {
            'name': name,
            'changes': {'rs.add': mongodb.conn_opts.host},
            'result': None,
            'comment': 'Host would be added to replicaset',
        }

    mongodb.add_to_replicaset(master_hostname, arbiter, force)

    return {
        'name': name,
        'changes': {'rs.add': mongodb.conn_opts.host},
        'result': True,
        'comment': 'Host was added to replicaset',
    }


def ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to `str`.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    NOTE: The function equals to six.ensure_str that is not present in the salt version of six module.
    """
    if type(s) is str:
        return s
    if six.PY2 and isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    elif six.PY3 and isinstance(s, six.binary_type):
        return s.decode(encoding, errors)
    elif not isinstance(s, (six.text_type, six.binary_type)):
        raise TypeError("not expecting type '%s'" % type(s))
    return s


def ensure_text(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to six.text_type.
    For Python 2:
      - `unicode` -> `unicode`
      - `str` -> `unicode`
    For Python 3:
      - `str` -> `str`
      - `bytes` -> decoded to `str`
    """
    if sys.version_info[0] == 2:
        binary_type = str
        text_type = unicode  # noqa
    else:
        text_type = str
        binary_type = bytes
    if isinstance(s, binary_type):
        return s.decode(encoding, errors)
    elif isinstance(s, text_type):
        return s
    else:
        raise TypeError("not expecting type '%s'" % type(s))
