#!py
from functools import partial

BIONIC = '18.04'

_MEGABYTES = 1024 ** 2
_GIGABYTES = 1024 ** 3

_ZK_DEFAULTS = {
    'config_prefix': '/etc/zookeeper/conf.yandex',
    'java_bin': '/usr/bin/java',
    'log_level': 'INFO',
    'jvm_xmx': '512M',
    'use_ssl': False,
    'nodes': {
    },
    'config': {
        'tickTime': 2000,
        'initLimit': 3600,
        'syncLimit': 20,
        'dataDir': '/var/lib/zookeeper',
        'clientPort': 2181,
        'skipACL': 'yes',  # Overrided when SSL merged
        'maxClientCnxns': 2000,
        'forceSync': 'no',
        'quorumListenOnAllIPs': 'true',
        'leaderServes': 'yes',
        'autopurge.snapRetainCount': 5,
        'autopurge.purgeInterval': 1,
        'snapCount': 200000,
        '4lw.commands.whitelist': 'stat,mntr,ruok,isro',
        'admin.enableServer': 'true',
        'admin.serverAddress': '127.0.0.1',
        'admin.idleTimeout': 500,
        'reconfigEnabled': 'true'
    },
    'users' : None,
}

_ZK_CONFIG_SSL = {
    'secureClientPort': 2281,
    'serverCnxnFactory': 'org.apache.zookeeper.server.NettyServerCnxnFactory',
    'ssl.keyStore.location': '/etc/zookeeper/conf.yandex/ssl/server.jks',
    'ssl.trustStore.location': '/etc/zookeeper/conf.yandex/ssl/truststore.jks',
    'ssl.quorum.keyStore.location': '/etc/zookeeper/conf.yandex/ssl/server.jks',
    'ssl.quorum.trustStore.location': '/etc/zookeeper/conf.yandex/ssl/truststore.jks',
    'skipACL': 'no',
    'sslQuorum': 'true',
    'portUnification': 'false',
}

_ZK_CONFIG_SSL_MAINTENANCE_STAGE_1 = {
    'skipACL': 'yes',
    'sslQuorum': 'false',
    'portUnification': 'true',
}

_ZK_CONFIG_SASL = {
    'authProvider.1': 'org.apache.zookeeper.server.auth.SASLAuthenticationProvider',
    'requireClientAuthScheme': 'sasl',
    'jaasLoginRenew': 3600000,
}

_ZK_CONFIG_SSL_MAINTENANCE_STAGE_2 = {
    'skipACL': 'no',
    'sslQuorum': 'true',
    'portUnification': 'true',
}

_ZK_BIONIC_FLAGS = """
    -XX:+UseG1GC
    -Xmx{xmx_size}
    -XX:+HeapDumpOnOutOfMemoryError
    -XX:HeapDumpPath=/var/cores
    -Xlog:gc:/var/log/zookeeper/gc.log:time
    -Djute.maxbuffer=16777216
    -Djava.net.preferIPv6Addresses=true
    -Djava.net.preferIPv4Stack=false
"""

_ZK_BIONIC_DEFAULT_VER = '3.5.5-1+yandex19-3067ff6'

_VERSIONS = {
    '3.6.3-1+yandex33-fd53f2f': [
        'zookeeper'
    ],
    '3.5.9-1+yandex25-9120b6b': [
        'zookeeper'
    ],
    '3.5.8-1+yandex21-4a5fc1c': [
        'zookeeper'
    ],
    '3.5.7-1+yandex20-aa503fd': [
        'zookeeper'
    ],
    '3.5.6-1+yandex31-dee6c71': [
        'zookeeper'
    ],
    '3.5.5-1+yandex19-3067ff6': [
        'zookeeper'
    ],
    '3.4.13-1+yandex0': [
        'zookeeper',
        'zookeeperd',
        'zookeeper-bin',
        'libzookeeper-mt2',
        'libzookeeper-java',
    ],
    '3.4.6-0yandex5': [
        'zookeeper',
        'zookeeperd',
        'zookeeper-bin',
        'libzookeeper-mt2',
        'libzookeeper-java',
    ],
}


def run():
    # MUST be inside run(): __*__ are not defined until after the whole file is loaded.
    _pillar = partial(__salt__['pillar.get'])
    _grains = partial(__salt__['grains.get'])
    _filter_by = partial(__salt__['grains.filter_by'], base='default', merge=_pillar('data:zk'))

    def get_xmx():
        if _pillar('data:dbaas:flavor') and not _pillar('data:zk:jvm_xmx'):
            return '{}M'.format(
                max(int(_pillar('data:dbaas:flavor:memory_guarantee') / _MEGABYTES - 1024), 512))
        return _pillar(
            'data:zk:jvm_xmx',
            _ZK_DEFAULTS.get('jvm_xmx', '512M'))

    def get_java_bin():
        base = '/usr/lib/jvm/java-11-openjdk-' + _grains('osarch') + '/bin/java'
        if _pillar('data:dbaas:vtype') == 'compute':
            return base + '-zookeeper'
        return base

    def get_version():
        return _pillar('data:zk:version', _ZK_BIONIC_DEFAULT_VER)

    def get_packages(version):
        vers = {}
        for pkg in _VERSIONS.get(version):
            vers[pkg] = version
        return vers

    def get_global_outstanding_limit():
        if _pillar('data:zk:global_outstanding_limit'):
            return int(_pillar('data:zk:global_outstanding_limit'))
        if _pillar('data:dbaas:flavor'):
            # Each 4Gb gives +1000 to limit.
            limit = max((int(_pillar('data:dbaas:flavor:memory_guarantee')) / _GIGABYTES / 4) * 1000, 1000)
            return min(limit, 10000)  # Limit to 10000 for safety. If need more - use pillar value.
        return 1000

    if _pillar('data:unmanaged:enable_zk_tls', False):
        _ZK_DEFAULTS['config'].update(_ZK_CONFIG_SSL)

    if _pillar('data:zk:scram_auth_enabled', False):
        _ZK_DEFAULTS['config'].update(_ZK_CONFIG_SASL)

    stage = _pillar('tls-stage', 0)
    if stage == 1:
        _ZK_DEFAULTS['config'].update(_ZK_CONFIG_SSL_MAINTENANCE_STAGE_1)
    elif stage == 2:
        _ZK_DEFAULTS['config'].update(_ZK_CONFIG_SSL_MAINTENANCE_STAGE_2)

    _ZK_DEFAULTS['config']['globalOutstandingLimit'] = get_global_outstanding_limit()

    return _filter_by({
        'default': _ZK_DEFAULTS,
        BIONIC: {
            'jvm_flags': ' '.join(
                _ZK_BIONIC_FLAGS.format(xmx_size=get_xmx()).split()),
            'java_bin': get_java_bin(),
            'packages': get_packages(get_version()),
        },
    }, grain='osrelease')
