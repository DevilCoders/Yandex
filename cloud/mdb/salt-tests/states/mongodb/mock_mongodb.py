#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import copy
import sys
import difflib
import yaml
from functools import partial
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

import pymongo.auth as mongo_auth
import hmac
from hashlib import sha1, pbkdf2_hmac
from base64 import standard_b64decode, standard_b64encode

try:
    import six
except ImportError:
    from salt.ext import six


def assert_structures_are_equal(data1, data2):
    if s_mongodb._compare_structs(data1, data2) is not True:
        print("====================", file=sys.stderr)
        print(data1, file=sys.stderr)
        print('', file=sys.stderr)
        print(data2, file=sys.stderr)
        print('== diff ==', file=sys.stderr)
        print(get_yaml_diff(data1, data2), file=sys.stderr)

    assert s_mongodb._compare_structs(data1, data2) is True
    return True


def get_yaml_diff(one, another):
    if not (isinstance(one, dict) and isinstance(another, dict)):
        raise TypeError('both arguments must be dicts')

    one_str = yaml.safe_dump(one or '', default_flow_style=False)
    another_str = yaml.safe_dump(another or '', default_flow_style=False)
    differ = difflib.Differ()

    diff = differ.compare(one_str.splitlines(True), another_str.splitlines(True))
    return '\n'.join(diff)


def _generate_credentials(username, password):
    salt = 'c2FsdHNhbHRzYWx0Cg=='
    iterations = 10000

    salted_pass = pbkdf2_hmac(
        'sha1',
        ensure_binary(mongo_auth._password_digest(ensure_text(username), ensure_text(password))),
        standard_b64decode(salt),
        iterations,
    )
    client_key = hmac.HMAC(salted_pass, b"Client Key", sha1).digest()
    stored_key = sha1(client_key).digest()
    storedKey = ensure_str(standard_b64encode(stored_key))

    return {
        "SCRAM-SHA-1": {
            "iterationCount": iterations,
            "salt": salt,
            "storedKey": storedKey,
            "serverKey": salt,
        },
    }


# Our mocked MongoDB
class MongoSaltMock(object):
    def __init__(self, initial_data):
        self.data = copy.deepcopy(initial_data)
        self.pillar = {}

        self.salt = {
            'grains.get': partial(self.mock_none, self),
        }

    def __getitem__(self, key):
        if key in self.salt:
            return self.salt[key]
        _key = key.replace('.', '_')
        return getattr(self, _key)

    @staticmethod
    def get_auth_data(**kwargs):
        ret = {
            'host': '127.0.0.1',
            'port': 28017,
            'user': 'admin',
            'password': '',
            'authdb': 'admin',
            'use_auth': False,
        }
        ret.update(kwargs)
        return ret

    @staticmethod
    def mock_none(*args, **kwargs):
        return None

    def pillar_set(self, new_pillar):
        self.pillar = copy.deepcopy(new_pillar)

    def get_data_copy(self):
        self._sort_data_inplace(self.data)
        return copy.deepcopy(self.data)

    @staticmethod
    def _sort_data_inplace(data):
        if data.get('roles', None) is not None:
            data['roles'].sort(key=lambda x: x.get('_id'))

    def pillar_get(self, path, *args):
        mpillar = self.pillar
        for k in path.split(':'):
            if k in mpillar:
                mpillar = mpillar[k]
            else:
                if len(args) > 0:
                    return args[0]
                else:
                    raise Exception('Key {} not found'.format(path))

        return copy.deepcopy(mpillar)

    def assert_data_is_equal(self, expected_data):
        _expected_data = copy.deepcopy(expected_data)
        self._sort_data_inplace(_expected_data)
        return assert_structures_are_equal(self.get_data_copy(), _expected_data)

    def mongodb_list_users(self, *args, **kwargs):
        ret = []
        for user in self.data['users']:
            for authdb in self.data['users'][user]['authdb']:
                roles = []

                for db in self.data['users'][user]['dbs']:
                    for role in self.data['users'][user]['dbs'][db]:
                        roles.append({'db': db, 'role': role})

                ret.append(
                    {
                        '_id': "{0}.{1}".format(authdb, user),
                        'user': user,
                        "db": authdb,
                        "credentials": _generate_credentials(
                            user,
                            self.data['users'][user]['password'],
                        ),
                        "roles": roles,
                    }
                )

        return ret

    def mongodb_user_grant_roles(
        self, name, roles, database, user=None, password=None, host=None, port=None, authdb=None
    ):
        for role in roles:
            r = role

            if type(r) is str:
                r = {'db': database, 'role': r}

            if self.data['users'][name]['dbs'].get(r['db'], None) is None:
                self.data['users'][name]['dbs'][r['db']] = []

            self.data['users'][name]['dbs'][r['db']] = list(set(self.data['users'][name]['dbs'][r['db']] + [r['role']]))

        return True

    def mongodb_user_revoke_roles(
        self, name, roles, database, user=None, password=None, host=None, port=None, authdb=None
    ):
        for role in roles:
            r = role

            if type(r) is str:
                r = {'db': database, 'role': r}

            self.data['users'][name]['dbs'][r['db']] = list(
                set(self.data['users'][name]['dbs'][r['db']]) - set([r['role']])
            )
            if len(self.data['users'][name]['dbs'][r['db']]) == 0:
                self.data['users'][name]['dbs'].pop(r['db'], None)

        return True

    def mongodb_user_remove(self, name, user=None, password=None, host=None, port=None, database='admin', authdb=None):
        self.data['users'][name]['dbs'].pop(database, None)
        self.data['users'][name]['authdb'] = [v for v in self.data['users'][name]['authdb'] if v != database]

        if len(self.data['users'][name]['dbs']) == 0:
            self.data['users'].pop(name)

        return True

    def mongodb_user_create(
        self, name, passwd, user=None, password=None, host=None, port=None, database='admin', authdb=None, roles=None
    ):

        if name not in self.data['users']:
            self.data['users'][name] = {'dbs': {}, 'authdb': [], 'password': passwd}

        self.data['users'][name]['dbs'][database] = []
        if database in self.data['users'][name]['authdb']:
            raise Exception("User {}@{} already exists".format(name, database))
        self.data['users'][name]['authdb'].append(database)
        if roles is not None:
            self.mongodb_user_grant_roles(
                name,
                roles,
                database,
                user=user,
                password=password,
                host=host,
                port=port,
                authdb=authdb,
            )

        return True

    def mongodb_user_update(
        self,
        name,
        passwd=None,
        user=None,
        password=None,
        host=None,
        port=None,
        database='admin',
        authdb=None,
        roles=None,
    ):
        if passwd is not None:
            self.data['users'][name]['password'] = passwd

        if self.data['users'][name]['dbs'].get(database, None) is None:
            self.data['users'][name]['dbs'][database] = []

        if roles is not None:
            self.data['users'][name]['dbs'] = {}
            self.data['users'][name]['dbs'][database] = []
            self.mongodb_user_grant_roles(
                name,
                roles,
                database,
                user=user,
                password=password,
                host=host,
                port=port,
                authdb=authdb,
            )
        return True

    def mongodb_user_auth(
        self,
        user=None,
        password=None,
        host=None,
        port=None,
        authdb=None,
        raise_exception=False,
    ):
        if (
            user in self.data['users']
            and self.data['users'][user]['password'] == password
            and authdb in self.data['users'][user]['authdb']
        ):
            return True, None
        else:
            return False, None

    def mongodb_check_auth(
        self,
        user=None,
        password=None,
        host=None,
        port=None,
        authdb=None,
        raise_exception=False,
    ):
        if (
            user in self.data['users']
            and self.data['users'][user]['password'] == password
            and authdb in self.data['users'][user]['authdb']
        ):
            return True, None
        else:
            return False, None

    def mongodb_user_exists(
        self,
        name,
        user=None,
        password=None,
        host=None,
        port=None,
        database='admin',
        authdb=None,
    ):
        if name in self.data['users'] and database in self.data['users'][name]['authdb']:
            return True
        else:
            return False

    def mongodb_is_primary(self, user=None, password=None, host=None, port=None, authdb=None):
        is_primary = self.data.get('mongodb', {}).get('is_primary', True)
        return is_primary, {'ismaster': is_primary, 'secondary': not is_primary}

    def mongodb_list_roles(self, *args, **kwargs):
        return copy.deepcopy(self.data['roles'])

    def mongodb_create_role(
        self,
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

        self.data['roles'].append(
            {
                '_id': '{}.{}'.format(database, name),
                'role': name,
                'db': database,
                'roles': copy.deepcopy(roles or []),
                'privileges': copy.deepcopy(privileges or []),
                'authenticationRestrictions': copy.deepcopy(authenticationRestrictions or []),
            }
        )
        return True

    def mongodb_drop_role(self, name, user=None, password=None, database=None, host=None, port=None, authdb='admin'):

        for i, v in enumerate(self.data['roles']):
            if v['_id'] == '{}.{}'.format(database, name):
                del self.data['roles'][i]

        return True

    def mongodb_update_role(
        self,
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

        self.mongodb_drop_role(name, user, password, database, host, port, authdb)

        return self.mongodb_create_role(
            name, roles, privileges, authenticationRestrictions, user, password, database, host, port, authdb
        )

    def mongodb_list_sharded_databases(self, **kwargs):
        # { "_id" : "asdasdas", "primary" : "rs01", "partitioned" : true,
        #  "version" : { "uuid" : UUID("e2248fd0-1dba-4c42-96e6-08fae8fb8c75"), "lastMod" : 1 } }
        res = []
        for db in self.data['dbs']:
            if self.data['dbs'][db]['sharded']:
                res.append(db)
        return res

    def mongodb_enable_sharding(self, database, **kwargs):
        if database not in self.data['dbs']:
            self.data['dbs'][database] = {
                'sharded': True,
                'collections': {},
            }
        else:
            self.data['dbs'][database]['sharded'] = True
        return True

    def mongodb_list_collections(self, database, **kwargs):
        if database in self.data['dbs']:
            return self.data['dbs'][database]['collections'].keys()
        else:
            return []

    def mongodb_list_sharded_collections(self, database, **kwargs):
        res = []

        for cname, cdata in self.data['dbs'][database]['collections'].items():
            if cdata['sharded'] is True:
                res.append(
                    {
                        '_id': '{}.{}'.format(database, cname),
                        'lastmodEpoch': None,
                        'lastmod': None,
                        'dropped': False,
                        'key': None,
                        'unique': False,
                        'uuid': None,
                    }
                )

        return res

    def mongodb_list_shard_tags(self, database, **kwargs):
        # { "_id" : { "ns" : "123.ch1", "min" : { "x" : 1 } }, "ns" : "123.ch1",
        # "min" : { "x" : 1 }, "max" : { "x" : 2 }, "tag" : "rs01" }

        res = []
        for cname, cdata in self.data['dbs'][database]['collections'].items():
            for tag in cdata['tags']:
                res.append(
                    {
                        '_id': {'ns': '{}.{}'.format(database, cname), 'min': None},
                        'ns': '{}.{}'.format(database, cname),
                        'min': None,
                        'max': None,
                        'tag': tag,
                    }
                )

        return res

    def mongodb_list_shards(self, *args, **kvargs):
        res = []
        for shard, tags in self.data['shards'].items():
            res.append({'_id': shard, 'host': None, 'state': 1, 'tags': tags[:]})

        return {'ok': 1, 'shards': res}

    def ensure_db(self, database):
        if database not in self.data['dbs']:
            self.data['dbs'][database] = {
                'sharded': False,
                'collections': {},
            }

    def ensure_collection(self, database, collection):
        self.ensure_db(database)

        if collection not in self.data['dbs'][database]['collections']:
            self.data['dbs'][database]['collections'][collection] = {
                'sharded': False,
                'tags': [],
                'capped': False,
                'indexes': [],
            }

    def mongodb_update_zone_key_range(self, database, collection, minkey, maxkey, zone, **kwargs):
        self.ensure_collection(database, collection)
        if zone not in self.data['dbs'][database]['collections'][collection]['tags']:
            self.data['dbs'][database]['collections'][collection]['tags'].append(zone)

        return True

    def mongodb_is_capped(self, database, collection, **kwargs):
        return self.data['dbs'].get(database, {}).get('collections', {}).get(collection, {}).get('capped', False)

    def mongodb_convert_to_capped(self, database, collection, size, **kwargs):
        self.ensure_collection(database, collection)
        self.data['dbs'][database]['collections'][collection]['capped'] = True

        return True

    def mongodb_drop_collection(self, database, collection, **kwargs):
        if database in self.data['dbs']:
            self.data['dbs'][database]['collections'].pop(collection, None)
        return True

    def mongodb_shard_collection(self, database, collection, key, **kwargs):
        self.ensure_collection(database, collection)
        self.data['dbs'][database]['collections'][collection]['sharded'] = True

    def mongodb_add_shard_to_zone(self, shard, zone, **kwargs):
        if shard not in self.data['shards']:
            self.data['shards'][shard] = []

        self.data['shards'][shard].append(zone)

    def mongodb_remove_shard_from_zone(self, shard, zone, **kwargs):
        if shard not in self.data['shards']:
            return

        self.data['shards'][shard] = list(set(self.data['shards'][shard]) - set([zone]))

    def mongodb_create_index(self, database, collection, key, options, **kwargs):
        self.ensure_collection(database, collection)
        if isinstance(key, bytes) or isinstance(key, six.string_types):
            key = {key: 1}
        ikey = [(x, key[x]) for x in sorted(key.keys())]

        for i, index in enumerate(self.data['dbs'][database]['collections'][collection]['indexes']):
            if index['key'] == ikey:
                if isinstance(options, dict):
                    self.data['dbs'][database]['collections'][collection]['indexes'][i].update(options)
                return True

        index = {}
        if isinstance(options, dict):
            index.update(options)
        index['key'] = ikey

        self.data['dbs'][database]['collections'][collection]['indexes'].append(index)

        return True

    def mongodb_list_indexes(self, database, collection, **kwargs):
        indexes = self.data['dbs'].get(database, {}).get('collections', {}).get(collection, {}).get('indexes', [])
        ret = {}

        for index in indexes:
            iid = '_'.join([x[0] for x in index['key']]) + '_'
            ret[iid] = {
                'v': 2,
                'ns': '{}.{}'.format(database, collection),
            }
            ret[iid].update(index)
            ret[iid]['key'] = index['key'][:]

        return ret

    def mongodb_get_balancer_state(self, **kwargs):
        return self.data.get('balancer', True)

    def mongodb_start_balancer(self, timeout, **kwargs):
        self.data['balancer'] = True
        return True

    def mongodb_stop_balancer(self, timeout, **kwargs):
        self.data['balancer'] = False
        return True

    def mongodb_db_list(self, **kwargs):
        res = []
        for db in self.data['dbs']:
            res.append(db)
        return res

    def mongodb_db_remove(self, database, **kwargs):
        self.data['dbs'].pop(database, None)

    def mongodb_insert(self, objects, database, collection, **kwargs):
        self.ensure_collection(database, collection)

    def mongodb_rs_step_down(self, **kwargs):
        is_master = self.mongodb_is_primary(**kwargs)
        if not is_master:
            return True
        nodes = self.data.get('mongodb', {}).get('nodes', 0)
        if nodes < 2:
            return False
        if 'mongodb' not in self.data:
            self.data['mongodb'] = {}
        self.data['mongodb']['is_primary'] = False
        return True

    def mongodb_get_last_resetup_id(self):
        return self.data.get('resetup_id', None)

    def mongodb_resetup_mongod(self, force=False, continue_flag=False):
        if not self.mongodb_rs_step_down():
            raise Exception('stepdown failed')
        return ("Ok", "Ok")

    def mongodb_mark_resetup_done(self, resetup_id):
        if resetup_id is not None:
            self.data['resetup_id'] = resetup_id
        return True

    def mongodb_mark_resetup_started(self, resetup_id):
        return

    def mongodb_is_resetup_started(self, resetup_id):
        return False

    def mongodb_get_last_stepdown_id(self):
        return self.data.get('stepdown_id', None)

    def mongodb_mark_stepdown_done(self, stepdown_id):
        if stepdown_id is not None:
            self.data['stepdown_id'] = stepdown_id
        return True

    def mdb_mongodb_helpers_replset_hosts(self, srv=None, **kwargs):
        nodes = self.data.get('mongodb', {}).get('nodes', 0)
        return ['nodc-{}.db.yandex.net'.format(i) for i in range(0, nodes)]

    def mongodb_replset_initiated(self, **kwargs):
        host = kwargs.get('host', None)
        return host in self.data.get('rs_hosts', [])

    def mongodb_get_rs_members(self, **kwargs):
        return self.data.get('rs_hosts', [])

    def mongodb_replset_add(self, host_port, arbiter=None, force=None, **kwargs):
        host = host_port.split(':')[0]
        if host in self.data.get('rs_hosts', []):
            if not force:
                raise Exception('Host in RS already')
            else:
                return True

        if self.data.get('rs_hosts', None) is None:
            self.data['rs_hosts'] = []

        self.data['rs_hosts'].append(host)
        return True

    def mdb_mongodb_helpers_get_mongo_connection_args(self, **kwargs):
        ret = self.get_auth_data(**kwargs)
        # Drop unneded keys
        keys_whitelist = [
            'host',
            'port',
            'user',
            'password',
            'authdb',
        ]
        for k in list(ret.keys()):
            if k not in keys_whitelist:
                ret.pop(k, None)

        return ret


def ensure_str(s, encoding='utf-8', errors='strict'):
    """Coerce *s* to six.text_type.
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


def ensure_binary(s, encoding='utf-8', errors='strict'):
    """Coerce **s** to six.binary_type.
    For Python 2:
      - `unicode` -> encoded to `str`
      - `str` -> `str`
    For Python 3:
      - `str` -> encoded to `bytes`
      - `bytes` -> `bytes`
    NOTE: The function equals to six.ensure_binary that is not present in the salt version of six module.
    """
    if isinstance(s, six.binary_type):
        return s
    if isinstance(s, six.text_type):
        return s.encode(encoding, errors)
    raise TypeError("not expecting type '%s'" % type(s))


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
