#!py

import json
from collections import OrderedDict
from functools import partial

# Fake definition for linters
__salt__ = {}

BIONIC = '18.04'
_MEGABYTES = 1024 ** 2
_GIGABYTES = 1024 ** 3

MONGO_ALL_SRVS = ['mongod', 'mongos', 'mongocfg']

MONITOR_LOCAL_DBS_GRANTS = ['collStats', 'dbHash', 'dbStats', 'find', 'getShardVersion',
                            'indexStats', 'killCursors', 'listCollections', 'listIndexes',
                            'planCacheRead']
MONITOR_LOCAL_COLLECTIONS_GRANTS = ['collStats', 'dbHash', 'dbStats', 'find', 'killCursors',
                                    'listCollections', 'listIndexes', 'planCacheRead']

MONGO_ROLES = {
    # Rough equivalent to clusterMonitor role without proprietary monitoring features
    # https://docs.mongodb.com/manual/reference/built-in-roles/#clusterMonitor
    'mdbMonitor': {
        # Provides the find action on all system.profile collections in the cluster
        'database': 'admin',
        'privileges': [
            {
                'resource': {
                    'db': '',
                    'collection': 'system.profile',
                    },
                'actions': ['find'],
                },
            # Provides the following actions on all databases in the cluster
            {
                'resource': {
                    'db': '',
                    'collection': '',
                    },
                'actions': ['collStats', 'dbStats', 'getShardVersion', 'indexStats', 'useUUID'],
                },
            # Provides the following actions on the cluster as a whole
            {
                'resource': {
                    'cluster': True,
                    },
                'actions': ['connPoolStats', 'getLog', 'getParameter', 'getShardMap', 'hostInfo',
                            'inprog', 'listDatabases', 'listSessions', 'listShards', 'netstat',
                            'replSetGetConfig', 'replSetGetStatus', 'serverStatus', 'shardingState',
                            'top'],
                },
            {
                'resource': {
                    'db': 'config',
                    'collection': '',
                    },
                'actions': MONITOR_LOCAL_DBS_GRANTS,
                },
        ],
    },
    # Sharding role
    'mdbShardingManager': {
        'database': 'admin',
        'privileges': [
            {
                'resource': {
                    'db': 'admin',
                    'collection': '',
                    },
                'actions': ['viewRole'],
            },
            # https://docs.mongodb.com/manual/reference/command/clearJumboFlag 4.0+
            {
                'resource': {
                    'anyResource': True,
                    },
                'actions': ['enableSharding', 'flushRouterConfig', 'getShardVersion',
                            'getShardMap', 'shardingState', 'moveChunk', 'splitChunk',
                            'splitVector', 'clearJumboFlag'],
            },
            # Misc permissions to enable sh.status() cmd
            {
                'resource': {
                    'db': 'config',
                    'collection': '',
                    },
                'actions': ['find'],
            },
        ],
    },
    # Internal admin role
    'mdbInternalAdmin': {
        'database': 'admin',
        'privileges': [
            {
                'resource': {
                    'anyResource': True
                },
                'actions': ['anyAction'],
            }
        ],
        'roles': [
            {'role': 'root', 'db': 'admin'},
            {'role': 'dbOwner', 'db': 'admin'},
            {'role': 'dbOwner', 'db': 'local'},
            {'role': 'dbOwner', 'db': 'config'},
        ],
    },
    # Internal monitor role (mdb-metrics, monrun, ...)
    'mdbInternalMonitor': {
        'database': 'admin',
        'privileges': [],
        'roles': [
            {'role': 'clusterMonitor', 'db': 'admin'},
            {'role': 'readWrite', 'db': 'mdb_internal'},
        ],
    },
    # Role for Client-side replication (for example Debezium) [Not so public]
    'mdbReplication': {
        'database': 'admin',
        'privileges': [
            {
                'resource': {
                    'db': '',
                    'collection': 'system.users',
                },
                'actions': ['find'],
            },
            {
                'resource': {
                    'db': '',
                    'collection': 'system.profile',
                },
                'actions': ['find'],
            },
            {
                'resource': {
                    'db': 'admin',
                    'collection': 'system.roles',
                },
                'actions': ['find'],
            },
            {
                'resource': {
                    'db': 'admin',
                    'collection': 'system.version',
                },
                'actions': ['find'],
            },
        ],
        'roles': [
            {'role': 'readAnyDatabase', 'db': 'admin'},
            {'role': 'read', 'db': 'local'},
            {'role': 'read', 'db': 'config'},
        ],
    },
    # Role for Writing to any database (for example for Yandex Tracker) [Not so public]
    'mdbGlobalWriter': {
        'database': 'admin',
        'privileges': [
            {
                'resource': {
                    'db': '',
                    'collection': '',
                },
                'actions': ['dropDatabase'],
            },
        ],
        'roles': [
            {'role': 'readWriteAnyDatabase', 'db': 'admin'},
        ],
    },
}

USERDB_ROLES = {
    'mdbDbAdmin': {
        'database': '{db}',
        'privileges': [
            {
                'resource': {
                    'db': '{db}',
                    'collection': '',
                    },
                'actions': [
                    'collMod',
                    'planCacheWrite',
                    'planCacheRead',
                    'planCacheIndexFilter',
                    'bypassDocumentValidation',
                ],
            },
        ],
        'roles': [
            {'role': 'readWrite', 'db': '{db}'},
        ],
    }
}

MONGO_DEFAULTS = {
    'roles': MONGO_ROLES,
    'userdb_roles': USERDB_ROLES,
    'config_prefix': '/etc/mongodb',
    'user': 'mongodb',
    'group': 'mongodb',
    'homedir': '/home/mongodb',
    'logdir': '/var/lib/mongodb',
    'initialized_flag': '/home/mongodb/.mongod_dbpath_initialized',
    'version': {
        'major_num': 500,
        'major_human': '5.0',
        'full_num': None,
        },
    'feature_compatibility_version': '5.0',
    'use_auth': True,
    'cluster_auth': 'keyfile',
    'use_ssl': True,
    'use_mongod': True,
    'use_mongos': False,
    'use_mongocfg': False,
    'use_wd_mongodb': True,  # TODO: Check, probably not needed anymore
    'low_memory': False,
    'shutdownTimeout': 60,
    'config': {
        'mongos': {
            '_settings': {
                'balancerWindowStopDurationMinutes': 300,
                'tlsDisabledProtocols': 'TLS1_0,TLS1_1',
                },
            'replication': {
                'localPingThresholdMs': 15,
                },
            'sharding': {
                'configDB': None,
                },
            'net': {
                'port': 27017,
                'maxIncomingConnections': 1024,
                'maxIncomingConnectionsOverride': ['::1/128', '127.0.0.1'],
                'reservedAdminThreads': 8,
                'ssl': {
                    'mode': 'allowSSL',
                    },
                },
            'operationProfiling': {
                'slowOpThresholdMs': 300,
                'slowOpSampleRate': 1,
                'mode': 'slowOp',
                },
            'processManagement': {
                'pidFilePath': '/var/run/mongos.pid',
                },
            'systemLog': {
                'path': '/var/log/mongodb/mongos.log',
                },
            'auditLog': {
                'path': '/var/log/mongodb/mongos_audit.log',
                },
            'setParameter': {
                'ShardingTaskExecutorPoolMinSize': 1,
                'ShardingTaskExecutorPoolMaxConnecting': 2,
                # 'taskExecutorPoolSize': 1, <-- mongodb default, we set our default at conf/templates/mongos.conf
            },
            '_srv_name': 'mongos',
            'cli': {  # mongo client-related things
                'cmd': '/usr/bin/mongo --ipv6',
                'host': '',
                'ssl_args': '',
                'ssl_uri_args': '',
                'ssl_mongodump_args': '',
                },
            },
        'mongocfg': {
            '_settings': {
                'tlsDisabledProtocols': 'TLS1_0,TLS1_1',
                },
            'replication': {
                'replSetName': 'mongocfg',
                },
            'storage': {
                'dbPath': '/var/lib/mongodb',
                'wiredTiger': {
                    'engineConfig': {
                        'cacheSizeGB': '0.5',
                        },
                    },
                },
            'operationProfiling': {
                'slowOpThresholdMs': 300,
                'slowOpSampleRate': 1,
                'mode': 'slowOp',
                },
            'net': {
                'port': 27019,
                'maxIncomingConnections': 1024,
                'maxIncomingConnectionsOverride': ['::1/128', '127.0.0.1'],
                'reservedAdminThreads': 8,
                'ssl': {
                    'mode': 'allowSSL',
                    },
                },
            'security': {
                'enableEncryption': False,
                'kmip': {
                    'serverName': None,
                    'port': 0,
                    'serverCa': None,
                    'clientCertificate': None,
                    'keyIdentifier': None,
                    },
                },
            'processManagement': {
                'pidFilePath': '/var/run/mongocfg.pid',
                },
            'systemLog': {
                'path': '/var/log/mongodb/mongocfg.log',
                },
            'auditLog': {
                'path': '/var/log/mongodb/mongocfg_audit.log',
                },
            'setParameter': {},
            '_srv_name': 'mongocfg',
            'cli': {  # mongo client-related things
                'cmd': '/usr/bin/mongo --ipv6',
                'host': '',
                'ssl_args': '',
                'ssl_uri_args': '',
                'ssl_mongodump_args': '',
                },
            },
        'mongod': {
            '_settings': {
                'oplogMaxSize': None,
                'tlsDisabledProtocols': 'TLS1_0,TLS1_1',
                },
            'replication': {
                'replSetName': 'rs01',
                },
            'storage': {
                'dbPath': '/var/lib/mongodb',
                'directoryPerDB': 'true',
                'engine': 'wiredTiger',
                'syncPeriodSecs': 5,
                'wiredTiger': {
                    'engineConfig': {
                        'cacheSizeGB': '0.5',
                        },
                    'collectionConfig': {
                        'blockCompressor': 'snappy',
                        },
                    },
                'journal': {
                    'enabled': True,
                    'commitInterval': 100,
                    },
                },
            'operationProfiling': {
                'slowOpThresholdMs': 300,
                'slowOpSampleRate': 1,
                'mode': 'slowOp',
                },
            'net': {
                'port': 27018,
                'maxIncomingConnections': 1024,
                'maxIncomingConnectionsOverride': ['::1/128', '127.0.0.1'],
                'reservedAdminThreads': 8,
                'ssl': {
                    'mode': 'allowSSL',
                    },
                },
            'security': {
                'enableEncryption': False,
                'kmip': {
                    'serverName': None,
                    'port': 0,
                    'serverCa': None,
                    'clientCertificate': None,
                    'keyIdentifier': None,
                    },
                },
            'processManagement': {
                'pidFilePath': '/var/run/mongodb.pid',
                },
            'systemLog': {
                'path': '/var/log/mongodb/mongodb.log',
                },
            'auditLog': {
                'path': '/var/log/mongodb/mongodb_audit.log',
                'filter': '{}',
                'runtimeConfiguration': False,
                },
            'setParameter': {},
            '_srv_name': 'mongodb',
            'cli': {  # mongo client-related things
                'cmd': '/usr/bin/mongo --ipv6',
                'host': '',
                'ssl_args': '',
                'ssl_uri_args': '',
                'ssl_mongodump_args': '',
                },
            },
        },
    }


def _pillar(*args, **kwargs):
    return __salt__['pillar.get'](*args, **kwargs)


def _grains(*args, **kwargs):
    return __salt__['grains.get'](*args, **kwargs)


def getMongoSConfigDB(mongocfg_config):
    '''
    Get URI of configdb for mongos
    '''
    mongocfg_port = mongocfg_config['net']['port']
    mongocfg_rs_name = mongocfg_config['replication']['replSetName']

    # format mongocfg url, like "rsname/host1:27019,host2:27019,host3:27019"
    return __salt__['mongodb.get_subcluster_conn_spec'](
        ['mongodb_cluster.mongocfg', 'mongodb_cluster.mongoinfra'],
        mongocfg_port,
        rsname=mongocfg_rs_name,
    )


def setMongoDSettings(mongodb):
    '''
    Set several mongod-related settings. Like oplog size, RS name, etc...
    '''

    # In case of custom shard name, us it as rs name
    shard_name = _pillar('data:dbaas:shard_name', None)
    shard_id = _pillar('data:dbaas:shard_id', None)
    if shard_name and shard_name != shard_id:
        mongodb['config']['mongod']['replication']['replSetName'] = shard_name

    # Reserve 8 connections for monitoring and admin (MDB-4177) and 7 for each replica
    # (-7 for itself as we do not need to connect to itself for replication purposes)
    reservedAdminThreads = 8 - 7 + (7 * len(_pillar('data:dbaas:cluster_hosts')))
    mongodb['config']['mongod']['net']['reservedAdminThreads'] = reservedAdminThreads

    # Set oplogMaxSize, default to 5% of disk size
    if not mongodb['config']['mongod']['_settings'].get('oplogMaxSize', None):
        oplogMaxSize = int(_pillar('data:dbaas:space_limit', 10000)) / 1024 / 1024 * 0.05
        if oplogMaxSize < 990:
            oplogMaxSize = 990
        mongodb['config']['mongod']['_settings']['oplogMaxSize'] = oplogMaxSize


def setMongoSSettings(mongodb):
    '''
    Set several mongos-related settings
    '''
    mongodb['config']['mongos']['sharding']['configDB'] = getMongoSConfigDB(
        mongodb['config']['mongocfg']
    )

    # format mongod shard urls list if mongodb-add-shards exists in pillar
    mongodb_add_shards = _pillar('mongodb-add-shards', [])
    mongodb_add_shards_map = {}
    mongod_port = mongodb['config']['mongod']['net']['port']

    if mongodb_add_shards:
        for subcluster in _pillar('data:dbaas:cluster:subclusters').values():
            if 'mongodb_cluster.mongod' in subcluster['roles'] or \
                    'mongodb_cluster' in subcluster['roles']:
                for shard_id in subcluster['shards']:
                    shard_url = __salt__['mongodb.get_shard_conn_spec'](
                        'mongodb_cluster.mongod',
                        mongod_port,
                        shard_id,
                        default_rsname='rs01',
                    )
                    shard_name = shard_url.split('/')[0]
                    mongodb_add_shards_map[shard_name] = shard_url

    mongodb['add_shards'] = mongodb_add_shards_map

    mongodb_delete_shard_name = _pillar('mongodb-delete-shard-name')
    if mongodb_delete_shard_name:
        mongodb['delete_shard'] = mongodb_delete_shard_name


def setMongoCliSettings(mongodb):
    opts = {
        # 'cmd': '/usr/bin/mongo --ipv6 {ssl_args} --port {port} --host {host} ',
        # We don't want to provide host here as for unautentificated users (when no users' created)
        # we have to use localhost
        'cmd': '/usr/bin/mongo --ipv6 {ssl_args} --port {port}',
        'host': _grains('fqdn', '127.0.0.1'),
        'ssl_cli_args': '',
        'ssl_uri_args': '',
        'ssl_mongodump_args': '',
    }

    if mongodb['use_ssl']:
        ca_certs = '{config_prefix}/ssl/allCAs.pem'.format(config_prefix=mongodb['config_prefix'])
        tls_ssl = 'ssl'
        if int(mongodb['version']['major_num']) >= 402:
            tls_ssl = 'tls'

        opts.update({
            'ssl_cli_args': ' --{tls_ssl} --{tls_ssl}CAFile {ca_certs} '.format(
                tls_ssl=tls_ssl,
                ca_certs=ca_certs,
            ),
            # mongodump/mongorestore doesn't support --tls even on 4.2 =(
            'ssl_mongodump_args': ' --{tls_ssl} --{tls_ssl}CAFile {ca_certs} '.format(
                tls_ssl='ssl',
                ca_certs=ca_certs,
            ),
            # Not surwe if i should replace ssl to tls here as well
            # https://docs.mongodb.com/manual/reference/connection-string/#urioption.tls
            # won't do it for now
            'ssl_uri_args':
                '&ssl=true&ssl_ca_certs={ca_certs}&sslCertificateAuthorityFile={ca_certs}'.format(
                    # tls_ssl=tls_ssl,
                    ca_certs=ca_certs,
            ),
        })

        # Do it always as we call mongo cli for localhost usually
        #  (as we do it before admin user created)
        opts['ssl_cli_args'] += '--{tls_ssl}AllowInvalidHostnames'.format(
            tls_ssl=tls_ssl,
        )

        if opts['host'] == '127.0.0.1' or opts['host'] == 'localhost':
            opts['ssl_uri_args'] += '&sslAllowInvalidHostnames=true'
            opts['ssl_mongodump_args'] += '--{tls_ssl}AllowInvalidHostnames'.format(
                tls_ssl='ssl',
            )

    for srv in MONGO_ALL_SRVS:
        mongodb['config'][srv]['cli'].update(opts)
        mongodb['config'][srv]['cli']['cmd'] = opts['cmd'].format(
            ssl_args=opts['ssl_cli_args'],
            port=mongodb['config'][srv]['net']['port'],
            host=opts['host'],
        )


def fixSslTlsMode(mongodbVersion, sslTlsMode):
    '''
    For MongoDB 42+ use *TLS instead of *SSL
    '''
    if int(mongodbVersion['major_num']) >= 402:
        return sslTlsMode.replace('SSL', 'TLS')
    return sslTlsMode


def run():
    _filter_by = partial(__salt__['grains.filter_by'], base='default')

    mongodb = _filter_by({
        'default': MONGO_DEFAULTS,
        BIONIC: {
            },
        },
        grain='osrelease',
        merge=_pillar('data:mongodb', {}))

    # TODO: Think about better way
    if 'version' not in mongodb:
        mongodb['version'] = {}
    mongodb['version'].update(__salt__['mdb_mongodb.version']())

    # net.ssl.mode for compute MDB-5700
    defaultSSLmode = 'allowSSL'
    if _pillar('data:dbaas:vtype') == 'compute':
        defaultSSLmode = 'preferSSL'
        if _pillar('data:dbaas:assign_public_ip', False):
            defaultSSLmode = 'requireSSL'

    for srv in MONGO_ALL_SRVS:
        mongodb['config'][srv]['net']['ssl']['mode'] = fixSslTlsMode(
            mongodb['version'],
            _pillar('data:mongodb:config:{srv}:net:ssl:mode'.format(srv=srv), defaultSSLmode),
        )

    # By default use 1/2 of memory for Wired Tiger cache
    # + exceptions for small instance types
    if _pillar('data:dbaas:flavor'):
        memory_gb = _pillar('data:dbaas:flavor:memory_guarantee') / _GIGABYTES
        if memory_gb <= 2:
            default_cache_size = 0.5
        elif memory_gb <= 4:
            default_cache_size = 1
        else:
            default_cache_size = (memory_gb - 1) / 2.0

        for srv in ['mongod', 'mongocfg']:
            if not _pillar(
                'data:mongodb:config:{}:storage:wiredTiger:engineConfig:cacheSizeGB'.format(srv)
            ):
                mongodb['config'][srv]['storage']['wiredTiger'][
                    'engineConfig']['cacheSizeGB'] = default_cache_size

    # Get list of deployed services on current host
    # Actually, only one service should be deployed per host
    deployed = []
    for srv in MONGO_ALL_SRVS:
        if mongodb.get('use_{}'.format(srv), False):
            deployed.append(srv)

    mongodb['services_deployed'] = deployed

    # Fill different ID's and names
    mongodb.update({
        'cluster_id': _pillar('data:dbaas:cluster_id'),
        'shard_name': _pillar('data:dbaas:shard_name'),
        'shard_id': _pillar('data:dbaas:shard_id'),
        'subcluster_name': _pillar('data:dbaas:subcluster_name'),
        'subcluster_id': _pillar('data:dbaas:subcluster_id'),
        })

    # ZK-related (for backups)
    zk_lock_id = mongodb.get('shard_id', None) or mongodb.get('subcluster_id', None)
    mongodb.update({
        'zk_hosts': _pillar('data:mongodb:zk_hosts', []),
        'zk_restart_root': 'locks/{0}/restart'.format(zk_lock_id),
        'zk_resetup_root': 'locks/{0}/resetup'.format(zk_lock_id),
        'restart_lock_timeout': 300,
        })

    # Service-specific settings
    if mongodb['use_mongod']:
        setMongoDSettings(mongodb)
    if mongodb['use_mongos']:
        setMongoSSettings(mongodb)

    # Sharding
    mongodb['sharding_enabled'] = _pillar('data:mongodb:sharding_enabled', False)

    # mongo client settings, for easier use in states
    setMongoCliSettings(mongodb)

    if _pillar('data:dbaas:flavor:memory_guarantee') <= 8589934592:
        mongodb['low_memory'] = True

    audit_filter_str = mongodb['config']['mongod']['auditLog'].get('filter')
    if audit_filter_str is not None:
        audit_filter = json.loads(audit_filter_str, object_pairs_hook=OrderedDict)
        mongodb['config']['mongod']['auditLog']['filter'] = json.dumps(audit_filter)

    # Set MongoCFG security config just like in MongoD
    mongodb['config']['mongocfg']['security'].update(mongodb['config']['mongod']['security'])

    return mongodb
