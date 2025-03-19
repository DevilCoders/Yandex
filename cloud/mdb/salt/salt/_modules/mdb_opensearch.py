import base64
import crypt
import hashlib
import socket
import subprocess
import re
import os
import uuid
from itertools import chain
import zipfile

try:
    import six
except ImportError:
    from salt.ext import six

_default = object()

__salt__ = {}


SALT_LEN = 16
BCRYPT_SALT_LEN = 22

DATA_NODE_ROLE = 'opensearch_cluster.datanode'
MASTER_NODE_ROLE = 'opensearch_cluster.masternode'

LICENSE_LEVEL = {
    'oss': 0,
    'basic': 1,
    'gold': 2,
    'platinum': 3,
    'enterprise': 4,
    'trial': 5,
}

_GIGABYTES = 1024**3
_MEGABYTES = 1024**2

_JVM_MEM_SHARE = 0.7
_JVM_MEM_RESERVED = 3584 * _MEGABYTES
_JVM_MEM_MIN = 512 * _MEGABYTES

SYSTEM_USERS_ROLES = {
    'admin': ['superuser'],
    'mdb_admin': ['superuser'],
    'mdb_monitor': ['mdb_monitor'],
    'mdb_kibana': ['kibana_admin'],
}

DEFAULT_ROLES = ['superuser']

PLUGINS_VERSION_MAP = {
    '7.10.2': '7.10.2+yandex0',
}

# Path to root of cached in s3 objects
OBJECT_CACHE_ROOT = 'object_cache'


def pillar(key, default=_default):
    """
    Like __salt__['pillar.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised regardless of 'pillar_raise_on_missing' option.
    """
    value = __salt__['pillar.get'](key, default=default)
    if value is _default:
        raise KeyError('Pillar key not found: {0}'.format(key))

    return value


def is_opensearch():
    return pillar('data:opensearch', None) is not None


def cpillar(key, default=_default):
    return pillar('data:opensearch:' + key, default)


def grains(key, default=_default, **kwargs):
    """
    Like __salt__['grains.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised.
    """
    value = __salt__['grains.get'](key, default=default, **kwargs)
    if value is _default:
        raise KeyError('Grains key not found: {0}'.format(key))

    return value


def fqdn():
    """
    Return fqdn.

    Use grains id instead of fqdn as there are issues with DNS (CLOUD-48493).
    """
    return grains('id')


def hosts():
    """
    Return cluster hosts as tuple: (master nodes, data nodes).
    """
    result = {'master_nodes': {}, 'data_nodes': {}}
    for subcluster in pillar('data:dbaas:cluster:subclusters', {}).values():
        if DATA_NODE_ROLE in subcluster['roles']:
            result['data_nodes'] = subcluster['hosts']
        elif MASTER_NODE_ROLE in subcluster['roles']:
            result['master_nodes'] = subcluster['hosts']
        else:
            result['data_nodes'] = subcluster['hosts']
    return result


def initial_master_nodes():
    """
    Return list of initial master nodes.
    """
    all_hosts = hosts()
    if all_hosts['master_nodes']:
        nodes = all_hosts['master_nodes']
    else:
        nodes = all_hosts['data_nodes']
    return sorted([node for node in nodes.keys()])


def seed_peers():
    """
    Return list of seed peers.
    """
    all_hosts = hosts()
    return sorted([peer for peer in chain(all_hosts['data_nodes'].keys(), all_hosts['master_nodes'].keys())])


def data_node_urls():
    """
    Return list of data node urls.
    """
    secure_uri = 'https://{}:9200'

    all_hosts = hosts()
    data_nodes = set((secure_uri.format(h) for h in all_hosts['data_nodes'].keys()))

    return sorted(list(data_nodes))


def cluster_name():
    """
    Return cluster name.
    """
    return cpillar('cluster_name', None) or pillar('data:dbaas:cluster_id')


def original_domain():
    _, _, domain_orig = grains('id').partition('.')
    return domain_orig


def node_roles():
    roles = []
    if is_master_node():
        roles.append('master')
    if is_data_node():
        roles.append('data')
        roles.append('ingest')
    return roles


def is_master_node():
    """
    if host is a master node.
    """
    return cpillar('is_master', False)


def is_data_node():
    """
    if host is a data node.
    """
    return cpillar('is_data', False)


def licensed_for(license):
    """Check if current version has defined level of feature support"""
    license = license.lower()
    assert license in LICENSE_LEVEL, 'Unknown license {}'.format(license)
    try:
        return LICENSE_LEVEL[cpillar('edition', 'oss')] >= LICENSE_LEVEL[license]
    except KeyError:
        return False


def edition():
    """Returns Elasticsearch edition"""
    return cpillar('edition', 'oss')


def version():
    """
    Return Elasticsearch version.
    """
    return cpillar('version')


def plugins_version():
    """
    Return Elasticsearch version.
    """
    ver = version()
    return PLUGINS_VERSION_MAP.get(ver, ver)


def version_cmp(comparing_version):
    """
    Compare the Elasticsearch version from pillar with the version passed in as an argument.
    """
    return __salt__['pkg.version_cmp'](version(), comparing_version)


def version_ge(comparing_version):
    """
    Perform version comparison using version_cmp, but return a boolean:
     - If the current version is lesser to the provided in the arg, then False;
     - If the current version is greater or equal to the provided, then True;
    """
    if version_cmp(comparing_version) in (0, 1):
        return True
    return False


def version_lt(comparing_version):
    """
    Perform version comparison using version_cmp, but return a boolean.
    Return True if current version is less than the provided in the arg.
    """
    if version_cmp(comparing_version) == -1:
        return True
    return False


def _generate_salt(string, length=SALT_LEN):
    return ensure_str(base64.b64encode(hashlib.sha256(ensure_binary(string)).digest())[:length])


def _hash_bcrypt(password, salt_seed=None, salt=None):
    import bcrypt

    """ Generates a 2a variant of brcypt pw with 2^10 cost """
    if salt_seed is None and salt is None:
        raise ValueError('both salt_seed and salt are not provided')

    if salt is None:
        # 2a = algo variation
        # 10 = number of rounds. Expressed in power of 2
        # 22 char long is the default (should be about 128 bits when raw bytestream)
        salt_string = _generate_salt(salt_seed, BCRYPT_SALT_LEN)
        # Base64 char range differs from bcrypt char range in these two.
        salt = r'$2a$10${salt}'.format(salt=salt_string.replace('+', '/').replace('=', '/'))
    return ensure_str(bcrypt.hashpw(ensure_binary(password), ensure_binary(salt)))


def render_htpasswd():
    """Renders a string which is used to fill htpasswd file for Basic HTTP Auth"""
    user_list = cpillar('users')
    strings = []
    for name in sorted(user_list.keys()):
        crypt_salt = r'$6${salt}$'.format(salt=_generate_salt(name))
        strings.append('{user}:{pw}'.format(user=name, pw=crypt.crypt(user_list[name]['password'], crypt_salt)))
    return '\n'.join(strings) + '\n'


def nginx_server_names(fqdn=None):
    """Using FQDN and cluster id generates a string containing all possible server names"""
    server_names = []

    if fqdn is None:
        fqdn = socket.getfqdn()

    cluster_id = pillar('data:dbaas:cluster_id', None)  # may be missing when creating images
    hostname, _, mydomain = fqdn.partition('.')

    # Host names: management and external
    for domain in {'db.yandex.net', mydomain}:
        server_names.append('{host}.{domain}'.format(host=hostname, domain=domain))

    # CNAMEs
    if cluster_id is not None:
        cname_prefix = 'c-{cid}'.format(cid=cluster_id)
        for zone_prefix in ('rw', 'ro'):
            server_names.append(
                '{host}.{prefix}.{domain}'.format(host=cname_prefix, prefix=zone_prefix, domain=mydomain)
            )

    return ' '.join(sorted(server_names))


def jvm_mem_share():
    if is_master_node():
        return cpillar('config:master_node:jvm_mem_share', 60)
    # if is_ml_node():
    #     return cpillar('config:ml_node:jvm_mem_share', 25)
    return cpillar('config:data_node:jvm_mem_share', 50)


def jvm_mem_max():
    if is_master_node():
        return cpillar('config:master_node:jvm_mem_max', 31 * _GIGABYTES)
    # if is_ml_node():
    #     return cpillar('config:ml_node:jvm_mem_max', 2 * _GIGABYTES)
    return cpillar('config:data_node:jvm_mem_max', 31 * _GIGABYTES)


def jvm_xmx_mb():
    total = int(pillar('data:dbaas:flavor:memory_limit', '0'))
    v = min(total * jvm_mem_share() // 100, jvm_mem_max())
    v = min(v, total - _JVM_MEM_RESERVED)
    return max(v, _JVM_MEM_MIN) // _MEGABYTES


def data_node_settings():
    settings_map = {
        'fielddata_cache_size': 'indices.fielddata.cache.size',
        'max_clause_count': 'indices.query.bool.max_clause_count',
        'reindex_remote_whitelist': 'reindex.remote.whitelist',
    }
    settings = cpillar('config:data_node', {})
    config = {settings_map[k]: v for k, v in settings.items() if k in settings_map}
    config['reindex.ssl.certificate_authorities'] = ['/etc/elasticsearch/certs/ca.pem']
    if 'reindex_ssl_ca_path' in settings:
        config['reindex.ssl.certificate_authorities'].append(settings['reindex_ssl_ca_path'])
    return config


def geo():
    parts = fqdn().split('-')
    if len(parts) < 2:
        raise Exception('Unexpected fqdn format: %s', fqdn())
    return parts[0]


def pillar_users():
    return cpillar('users', {})


def _run_cmd(cmd, stdin=None):
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if stdin is not None:
        stdin = ensure_binary(stdin)
    out, outerr = proc.communicate(stdin)
    retcode = proc.poll()
    if retcode:
        raise subprocess.CalledProcessError(retcode, cmd, output=out)
    return ensure_str(out), ensure_str(outerr)


# Plugins Management
def install_plugin(plugin):
    plugin_path = "file:///usr/share/opensearch/core-plugins/{0}-2.1.0.zip".format(plugin)
    cmd = ['/usr/share/opensearch/bin/opensearch-plugin', 'install', '-b', plugin_path]
    return _run_cmd(cmd)


def remove_plugin(plugin):
    cmd = ['/usr/share/opensearch/bin/opensearch-plugin', 'remove', plugin]
    return _run_cmd(cmd)


def installed_plugins():
    cmd = ['/usr/share/opensearch/bin/opensearch-plugin', 'list']
    out, _ = _run_cmd(cmd)
    return out.splitlines()


def installed_and_update_plugins():
    cmd = ['/usr/share/opensearch/bin/opensearch-plugin', 'list']
    update = []
    out, err = _run_cmd(cmd)

    # Parse follow output
    # $ /usr/share/elasticsearch/bin/elasticsearch-plugin list
    # stdout: repository-s3
    # stderr: WARNING: plugin [repository-s3] was built for Elasticsearch version 7.10.2 but version 7.11.2 is required

    for n in err.splitlines():
        m = re.search(r'^WARNING: plugin \[(.*)\]', n)
        update.append(m.group(1))
    return out.splitlines(), update


def pillar_plugins():
    return set(cpillar('plugins', [])) | set([
        'repository-s3',
        'opensearch-security',
        'opensearch-asynchronous-search',
        'opensearch-knn',
        'opensearch-notifications',
        'opensearch-observability',
        'opensearch-reports-scheduler',
        'opensearch-sql',
        'opensearch-job-scheduler',
        'opensearch-ml',
        'opensearch-notifications-core',
        'opensearch-performance-analyzer',
        'opensearch-security',
        'opensearch-alerting',
        'opensearch-anomaly-detection',
        'opensearch-cross-cluster-replication',
        'opensearch-index-management',
    ])


def has_repository_s3():
    return 'repository-s3' in pillar_plugins()


def s3client_settings():
    result = {}
    if not has_repository_s3():
        return result
    s3clients = cpillar('s3:client', {})

    if 'default' not in s3clients:
        result.update(
            {
                's3.client.default.endpoint': pillar('data:object_storage:endpoint', ''),
                's3.client.default.signer_override': 'ycIamToken',
            }
        )

    if 'yc-automatic-backups' not in s3clients:
        result.update(
            {
                's3.client.yc-automatic-backups.endpoint': pillar('data:s3:endpoint', '').replace('+path', ''),
                's3.client.yc-automatic-backups.path_style_access': not pillar(
                    'data:s3:virtual_addressing_style', False
                ),
            }
        )

    for client, params in s3clients.items():
        for key, value in params.items():
            result["s3.client.{}.{}".format(client, key)] = value
    return result


# Secrets Management
def keystore_settings():
    settings = cpillar('keystore', {})
    if has_repository_s3():
        settings.setdefault('s3.client.default.access_key', 'access_key')
        settings.setdefault('s3.client.default.secret_key', 'secret_key')

        if auto_backups_enabled():
            settings.setdefault('s3.client.yc-automatic-backups.access_key', pillar('data:s3:access_key_id', ''))
            settings.setdefault('s3.client.yc-automatic-backups.secret_key', pillar('data:s3:access_secret_key', ''))
    return settings

def keystore_add(key, value):
    cmd = ['/usr/share/opensearch/bin/opensearch-keystore', 'add', '--stdin', '--force', key]
    _run_cmd(cmd, stdin=value)  # no output


def keystore_remove(key):
    cmd = ['/usr/share/opensearch/bin/opensearch-keystore', 'remove', key]
    _run_cmd(cmd)  # no output


def keystore_keys():
    cmd = ['/usr/share/opensearch/bin/opensearch-keystore', 'list']
    out, _ = _run_cmd(cmd)
    return out.splitlines()


# Users Management
def render_users_file():
    """
    Renders the 'users' file containing user:pwhash strings separated by a newline
    """
    user_list = cpillar('users', {})
    strings = []
    for name in sorted(user_list.keys()):
        # using username as a seed might sound like a bad idea, but we do have to make
        # passwords predictable to avoid constant file changes. Thus random salt is not suitable here.
        strings.append('{user}:{pw}'.format(user=name, pw=_hash_bcrypt(user_list[name]['password'], salt_seed=name)))
    return '\n'.join(strings) + '\n'


def render_users_roles_file():
    """
    Renders the 'users_roles' file containing user:role1,role2 designation separated by a newline
    """
    user_list = cpillar('users', {})
    strings = []
    roles = {}
    for name in sorted(user_list.keys()):
        defined_roles = user_list[name].get('roles', DEFAULT_ROLES)
        if name in SYSTEM_USERS_ROLES:
            defined_roles = SYSTEM_USERS_ROLES[name]
        for role in defined_roles:
            try:
                roles[role].add(name)
            except KeyError:
                roles[role] = {name}
    for role, users in roles.items():
        strings.append('{role}:{users}'.format(role=role, users=','.join(sorted(list(users)))))
    return '\n'.join(sorted(strings)) + '\n'


def internal_users():
    user_list = cpillar('users', {})
    result = {}
    for name in sorted(user_list.keys()):
        result[name] = {
            'hash': _hash_bcrypt(user_list[name]['password'], salt_seed=name),
            'reserved': True,
            'hidden': user_list[name]['internal'] if 'internal' in user_list[name] else False,
            'opendistro_security_roles': ['all_access', 'security_rest_api_access'],
        }
    return result


def auth_chain_kibana():
    providers = cpillar('auth:providers', [])
    result = {}
    has_native = False
    max_order = 0
    for p in providers:
        tp = p['type']
        if p['name'] == 'Native':
            has_native = True
            tp = 'token' if licensed_for('gold') else 'basic'
        result['{}.{}'.format(tp, p['name'])] = p['settings']
        max_order = max(max_order, p['settings']['order'])

    if not has_native:
        tp = 'token' if licensed_for('gold') else 'basic'
        result['{}.Native'.format(tp)] = {'order': max_order + 1}

    return {'xpack.security.authc.providers': result}


def auth_chain_elastic():
    realms = cpillar('auth:realms', {})
    result = {"file.File": {"order": 0}}

    has_native = False
    max_order = 0
    for tp, rset in realms.items():
        for r in rset:
            result["{}.{}".format(tp, r["name"])] = r["settings"]
        if tp == "native":
            has_native = True
        max_order = max(max_order, r["settings"]["order"])

    if not has_native:
        result["native.Native"] = {"order": max_order + 1}

    return {"xpack.security.authc.realms": result}


def auth_files():
    realms = cpillar('auth:realms', {})
    result = []

    for tp, rset in realms.items():
        for idx, r in enumerate(rset):
            if "files" in r:
                for f in r["files"].keys():
                    result.append(
                        (
                            "{}_{}".format(r["name"], f),
                            "data:elasticsearch:auth:realms:{}:{}:files:{}".format(tp, idx, f),
                        )
                    )

    return result


def _request(api, method='GET', retry=3, **kwargs):
    """
    Send request to Elasticsearch.
    """
    import requests
    from requests.adapters import HTTPAdapter
    from requests.packages.urllib3.util.retry import Retry

    adapter = HTTPAdapter(
        max_retries=Retry(
            total=retry,
            backoff_factor=1,  # will sleep for 0s, 1s, 2s, 4s, 8s ...
            status_forcelist=[404, 500, 502, 503, 504],
            method_whitelist=["GET"],
        )
    )
    http = requests.Session()
    http.mount("https://", adapter)
    http.mount("http://", adapter)

    r = http.request(
        method,
        'https://' + fqdn() + ':9200/' + api,
        auth=(
            cpillar('users:mdb_admin:name'),
            cpillar('users:mdb_admin:password'),
        ),
        **kwargs
    )
    r.raise_for_status()
    return r


def is_current_master():
    """
    if host is a current master in cluster.
    """
    return fqdn() == current_master()


def current_master():
    r = _request('_cat/master?h=host')
    return r.content.strip()


def flush():
    r = _request('_flush', timeout=5 * 60)
    return r.json()


def get_license():
    r = _request('_license')
    return r.json()


def delete_license():
    r = _request('_license', method='DELETE')
    return r.json()


def update_license(license):
    r = _request(
        '_license',
        method='PUT',
        data=license,
        params={'acknowledge': 'true'},
        headers={'content-type': 'application/json'},
        retry=5,
    )
    return r.json()


def wait_for_status(status, timeout=5 * 60):
    params = {'wait_for_status': status, 'timeout': '{}s'.format(timeout)}
    _request('_cluster/health', params=params, timeout=timeout + 5)


def get_repository(reponame):
    try:
        r = _request('_snapshot/' + reponame)
        return r.json(), True
    except Exception:
        return {}, False


def update_repository(reponame, settings):
    r = _request('_snapshot/' + reponame, method='PUT', json={'type': 's3', 'settings': settings})
    return r.json()


def get_snapshot_policy(policy_name):
    try:
        r = _request('_slm/policy/' + policy_name)
        return r.json(), True
    except Exception:
        return {}, False


def update_snapshot_policy(policy_name, settings):
    r = _request('_slm/policy/' + policy_name, method='PUT', json=settings)
    return r.json()


def auto_backups_enabled():
    return cpillar('auto_backups:enabled', False)


def reload_secure_settings():
    r = _request('_nodes/_local/reload_secure_settings', method='POST')
    return r.json()


def do_backup():
    if not auto_backups_enabled():
        raise Exception('Cluster backups not enabled')

    r = _request(
        '_snapshot/yc-automatic-backups/manual-snapshot-' + uuid.uuid4().hex,
        method='POST',
        params={'wait_for_completion': 'true'},
    )
    return r.json()


def do_restore(backup_id):
    r = _request('_all/_close?expand_wildcards=all', method='POST')
    r.raise_for_status()

    if version_ge('7.11'):
        r = _request('_data_stream/*?expand_wildcards=all', method='DELETE')
        r.raise_for_status()

    r = _request(
        '_snapshot/yc-automatic-restore/{}/_restore'.format(backup_id),
        method='POST',
        params={'wait_for_completion': 'true'},
        # we must include pattern for indices here due to bug https://github.com/elastic/elasticsearch/issues/75192 fixed in 7.14
        json={'include_global_state': True, "indices": "*"},
    )
    return r.json()


def s3bucket():
    return (
        pillar('data:s3_bucket', '')
        or pillar('data:s3_buckets:backup', '')
        or pillar('data:s3_buckets:secure_backups', '')
    )


# Extensions Management
def active_extension_ids():
    extensions = cpillar('extensions', [])
    return [ext['id'] for ext in extensions if ext['active']]


def all_extension_ids():
    extensions = cpillar('extensions', [])
    return [ext['id'] for ext in extensions]


def extensions():
    '''
    Returns active extensions that should be installed
    '''
    return _extension_params(active_extension_ids())


def new_extensions():
    '''
    Returns new extensions for downloading and validation
    '''
    return _extension_params(pillar('extensions', []))


def _extension_params(ids):
    extensions = cpillar('extensions', [])
    sa = True if pillar('data:service_account_id', None) else False

    extlist = []
    for ext in extensions:
        if ext['id'] in ids:
            extlist.append(
                {
                    'id': ext['id'],
                    'name': ext['name'],
                    'path': os.path.join(OBJECT_CACHE_ROOT, 'extensions', ext['id'] + '.zip'),
                    'uri': ext['uri'],
                    'use_service_account': sa,
                }
            )

    return extlist


def validate_extension(extname, archive):
    '''
    Validates extension
    '''
    import magic

    total_size_limit = cpillar('extension_size_limit', 1 * _GIGABYTES)
    files_limit = cpillar('extension_files_limit', 100)
    total = 0
    with zipfile.ZipFile(archive) as arch:
        if len(arch.infolist()) > files_limit:
            raise RuntimeError(
                'Extension contains too many files: {actual} > {smax}'.format(
                    actual=len(arch.infolist()), smax=files_limit
                )
            )
        for info in arch.infolist():
            if info.file_size > 100 * _MEGABYTES:
                raise RuntimeError(
                    'Extension file is too large: {actual} > {smax}'.format(
                        actual=info.file_size, smax=100 * _MEGABYTES
                    )
                )
            total = total + info.file_size
            if total > total_size_limit:
                raise RuntimeError(
                    'Extension is too large: {actual} > {smax}'.format(actual=total, smax=total_size_limit)
                )
            if os.path.normpath(info.filename).startswith('..'):
                raise RuntimeError('Invalid filename in extension (two dots)')
            with arch.open(info, 'r') as file:
                ftype = magic.from_buffer(file.read(2048), mime=True)
                if not allowed_extension_file_type(ftype):
                    raise RuntimeError('Invalid file type in extension: {type}'.format(type=ftype))
    return


def allowed_extension_file_type(ftype):
    if ftype == 'text/plain':
        return True
    return False


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
