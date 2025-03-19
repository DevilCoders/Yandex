# -*- coding: utf-8 -*-
'''
Module to provide MDB-specific MongoDB functionality to Salt

'''
from __future__ import absolute_import, print_function, unicode_literals

# Import python libs
import logging
from functools import wraps

# Import salt libs


try:
    import pymongo
    import bson

    assert pymongo
    assert bson
    HAS_MONGODB = True
except ImportError:
    HAS_MONGODB = False

log = logging.getLogger(__name__)

SRV_TO_PORT = {
    'mongod': 27018,
    'mongos': 27017,
    'mongocfg': 27019,
}

MONGO_ALL_SRVS = ['mongod', 'mongos', 'mongocfg']

GBYTE = 1024**3

# For linters, salt will populate it in runtime
__opts__ = {}
__salt__ = {}
__pillar__ = {}


def __virtual__():
    return 'mdb_mongodb_helpers'


def _pillar(*args, **kwargs):
    return __salt__['pillar.get'](*args, **kwargs)


def _grains(*args, **kwargs):
    return __salt__['grains.get'](*args, **kwargs)


class PillarCache(object):
    PILLAR_CACHE_ROOT = '_mongodb_cache'

    @classmethod
    def _ensure_cache_root(cls):
        if __pillar__.get(cls.PILLAR_CACHE_ROOT, None) is None:
            __pillar__[cls.PILLAR_CACHE_ROOT] = {}

    @classmethod
    def get(cls, key, default=None):
        cls._ensure_cache_root()
        return __pillar__[cls.PILLAR_CACHE_ROOT].get(key, default)

    @classmethod
    def set(cls, key, value):
        cls._ensure_cache_root()
        __pillar__[cls.PILLAR_CACHE_ROOT][key] = value

    @classmethod
    def delete(cls, key):
        cls._ensure_cache_root()
        __pillar__[cls.PILLAR_CACHE_ROOT].pop(key, None)

    @classmethod
    def clear(cls):
        __pillar__[cls.PILLAR_CACHE_ROOT] = {}


def use_pillar_cache(func):
    '''In case of Exception, return {result: False} with exception message as comment'''

    @wraps(func)
    def wrapper(*args, **kwargs):
        use_cache = kwargs.get('use_cache', True)
        cache_key = '{}.{}.{}'.format(func.__name__, repr(args), repr(kwargs))
        if use_cache:
            ret = PillarCache.get(cache_key)
            if ret is not None:
                return ret

        ret = func(*args, **kwargs)
        PillarCache.set(cache_key, ret)
        return ret

    return wrapper


@use_pillar_cache
def services_deployed(filter_services=None):
    '''
    Get list of deployed services on current host
    if filter_services is provided, show only deployed services in this list
    '''
    deployed = []
    defaults = {
        'mongod': True,
        'mongocfg': False,
        'mongos': False,
    }
    if not isinstance(filter_services, list):
        filter_services = defaults.keys()
    for srv in MONGO_ALL_SRVS:
        if _pillar('data:mongodb:use_{}'.format(srv), defaults[srv]) and srv in filter_services:
            deployed.append(srv)

    return deployed


def all_services():
    '''
    Return list of all available MongoDB services
    '''
    return MONGO_ALL_SRVS[:]


@use_pillar_cache
def is_replica(srv, use_cache=True):
    '''
    Get host role (is_replica).
    '''
    is_replica = False

    if srv is None:
        deployed = services_deployed()
        assert len(deployed) == 1
        srv = deployed[0]

    if srv == 'mongos':
        # every mongos is master, no check needed
        return False

    # We are master by default
    grains_ismaster = True
    if _grains('mongodb:{}:replset_initialized'.format(srv), False):
        grains_ismaster = _grains('mongodb:{}:ismaster'.format(srv), False)

    if _pillar('mongodb-primary') or _pillar('replica') or not grains_ismaster:
        is_replica = True

    try:
        if not is_replica and HAS_MONGODB and 'mongodb.is_primary' in __salt__:
            is_master, doc = __salt__['mongodb.is_primary'](port=SRV_TO_PORT[srv])
            if (
                isinstance(is_master, bool)
                and isinstance(doc, dict)
                and doc.get('ismaster', None) != doc.get('secondary', None)
            ):
                # Sometimes (If RS isn't initialized yet?), we can get following answer:
                # {
                #  u'info': u'Does not have a valid replica set config',
                #  u'ismaster': False,
                #  u'maxWriteBatchSize': 100000,
                #  u'ok': 1.0,
                #  u'maxWireVersion': 7,
                #  u'readOnly': False,
                #  u'logicalSessionTimeoutMinutes': 30,
                #  u'localTime': datetime.datetime(2020, 7, 8, 19, 58, 46, 892000),
                #  u'minWireVersion': 0,
                #  u'isreplicaset': True,
                #  u'maxBsonObjectSize': 16777216,
                #  u'maxMessageSizeBytes': 48000000,
                #  u'secondary': False
                # }
                # I don't want to parse `info`, so i'll check if ismaster != secondary

                is_replica = not is_master
    except Exception as exc:
        log.error(exc, exc_info=True)

    return is_replica


def master_hostname():
    '''
    Get master hostname if any.
    '''
    if _pillar('mongodb-primary'):
        return _pillar('mongodb-primary')

    return None


def get_cluster_conn_override():
    '''
    get list of cluster's host ip addresses
    '''
    hosts = []
    for host in _pillar('data:dbaas:cluster_hosts', []):
        res = __salt__['dnsutil.AAAA'](host)
        if type(res) == list:
            for ipv6address in res:
                hosts.append(ipv6address)
        else:
            log.Error('Error while getting AAAA records of host %s: %s', host, res)
        res = __salt__['dnsutil.A'](host)
        if type(res) == list:
            for ipv4address in res:
                hosts.append(ipv4address)
        else:
            log.Error('Error while getting A records of host %s: %s', host, res)

    return hosts


def get_cluster_item(role, shard_id=None):
    subclusters = _pillar('data:dbaas:cluster:subclusters')
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


@use_pillar_cache
def replset_hosts(srv=None, **kwargs):
    '''
    Get list of replset hosts
    if srv isn't provided, use first deployed service
    '''
    # In case of some [fake] pillar was passed to function,
    # Just do pillar.update(fake_pillar) instead of deep merging.
    # We use so as it is much faster and easier and
    # it should be used only in case of
    # salt-call --local mdb_mongodb_helpers.replset_hosts <srv> 'pillar={...}'
    # I.e. when real pillar is unaccessible (i.e. empty)
    if 'pillar' in kwargs:
        __pillar__.update(kwargs['pillar'])

    if srv is None:
        srv = services_deployed()[0]

    if srv == 'mongod':
        return _pillar('data:dbaas:shard_hosts')
    elif srv == 'mongocfg':
        return list(get_cluster_item(['mongodb_cluster.mongocfg', 'mongodb_cluster.mongoinfra'])['hosts'].keys())
    elif srv == 'mongos':
        return list(get_cluster_item(['mongodb_cluster.mongos', 'mongodb_cluster.mongoinfra'])['hosts'].keys())

    raise ValueError("Wrong srv: {srv}".format(srv=srv))


def deploy_service(srv):
    '''
    Check if we need to process given service
    I.e. check it this service is deployed and it shouldn't been skiped
    '''
    return srv in services_deployed() and not _pillar('skip-{srv}'.format(srv=srv), False)


def get_mongo_ro_limit(space_limit):
    """
    How much free space needed to proper MongoDB work
    (reserve more in case of space_limit is close to max limit)
    """
    DEFAULT_FREE_MB_LIMIT = 512
    MAX_SPACE_LIMIT_GB = 600  # Actually 605, but whatever
    # In porto limit is much smaller, but in porto we always can say portoctl set <fqdn>
    # space_limit=...

    try:
        space_limit = int(space_limit)
    except ValueError as exc:
        log.error(exc, exc_info=True)
        # If we got invalid value as space limit, assume that we have no space limit
        space_limit = 0

    space_limit_gb = space_limit / GBYTE

    if space_limit_gb >= MAX_SPACE_LIMIT_GB:
        return 5 * 1024  # 5 Gb in megabytes

    return DEFAULT_FREE_MB_LIMIT


def mdb_mongo_tools_confdir():
    "Confdir for mongotools"
    return "/etc/yandex/mdb-mongo-tools"


def get_mongo_connection_args(**kwargs):
    "Return dict with mongo connection arguments"
    args = {
        'host': kwargs.get('host', None),
        'port': kwargs.get('port', None),
        'user': kwargs.get('user', None),
        'password': kwargs.get('password', None),
        'authdb': kwargs.get('authdb', None),
        'options': kwargs.get('options', None),
    }

    service = kwargs.get('service', 'mongod')
    use_auth = kwargs.get('use_auth', True)

    if args['host'] is None:
        args['host'] = __salt__['grains.get']('fqdn', '127.0.0.1')

    if args['port'] is None:
        args['port'] = SRV_TO_PORT[service]

    if use_auth:
        if args['user'] is None:
            args['user'] = 'admin'
        if args['password'] is None:
            args['password'] = __salt__['pillar.get']('data:mongodb:users:{}:password'.format(args['user']))
        if args['authdb'] is None:
            args['authdb'] = 'admin'
    else:
        args.update({
            'user': None,
            'password': None,
            'authdb': None,
        })

    return args
