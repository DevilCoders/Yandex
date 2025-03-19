import logging
import socket
from collections import OrderedDict
from functools import wraps
from random import SystemRandom

import string
from nacl.encoding import URLSafeBase64Encoder
from nacl.public import Box, PrivateKey, PublicKey
from nacl.utils import random

import sshtunnel
import click
from click import echo, prompt
from cloud.mdb.cli.common.parameters import ListParamType
from cloud.mdb.cli.dbaas.internal import vault
from cloud.mdb.cli.dbaas.internal.config import config_option, get_config, get_config_key, update_config_key

RUNNING_CLUSTER_STATUS = 'RUNNING'
UPDATING_CLUSTER_STATUSES = [
    'CREATING',
    'MODIFYING',
    'STOPPING',
    'STARTING',
    'DELETING',
    'RESTORING-ONLINE',
    'RESTORING-OFFLINE',
    'MAINTAINING-OFFLINE',
]
STOPPED_CLUSTER_STATUS = 'STOPPED'
DELETED_CLUSTER_STATUSES = [
    'DELETED',
    'METADATA-DELETING',
    'METADATA-DELETED',
    'PURGING',
    'PURGED',
]
ERROR_CLUSTER_STATUSES = [
    'CREATE-ERROR',
    'MODIFY-ERROR',
    'STOP-ERROR',
    'START-ERROR',
    'DELETE-ERROR',
    'PURGE-ERROR',
    'METADATA-DELETE-ERROR',
    'RESTORE-ONLINE-ERROR',
    'RESTORE-OFFLINE-ERROR',
    'MAINTAIN-OFFLINE-ERROR',
]
CLUSTER_STATUSES = [
    RUNNING_CLUSTER_STATUS,
    *UPDATING_CLUSTER_STATUSES,
    STOPPED_CLUSTER_STATUS,
    *DELETED_CLUSTER_STATUSES,
    *ERROR_CLUSTER_STATUSES,
]


class ClusterStatus(click.Choice):
    """
    Command-line parameter type for cluster statuses.
    """

    name = 'cluster_status'

    def __init__(self):
        super().__init__(CLUSTER_STATUSES)


def cluster_status_options(with_short_aliases=False):
    """
    A set of command-line options for filtering by cluster statuses.
    """
    short_aliases = {
        'statuses': ['--status'],
        'exclude_statuses': ['--xstatus', '--exclude-status'],
        'running': ['--running'],
        'updating': ['--updating'],
        'stopped': ['--stopped'],
        'deleted': ['--deleted'],
        'error': ['--error'],
        'exclude_running': ['--no-running'],
        'exclude_updating': ['--no-updating'],
        'exclude_stopped': ['--no-stopped'],
        'exclude_deleted': ['--no-deleted'],
        'exclude_error': ['--no-error'],
        'all': ['-a', '--all'],
    }
    long_aliases = {
        'statuses': ['--cluster-status', '--cluster-statuses'],
        'exclude_statuses': ['--exclude-cluster-status', '--exclude-cluster-statuses'],
        'running': ['--running-clusters', '--include-running-clusters'],
        'updating': ['--updating-clusters', '--include-updating-clusters'],
        'stopped': ['--stopped-clusters', '--include-stopped-clusters'],
        'deleted': ['--deleted-clusters', '--include-deleted-clusters'],
        'error': ['--error-clusters', '--include-error-clusters'],
        'exclude_running': ['--no-running-clusters', '--exclude-running-clusters'],
        'exclude_updating': ['--no-updating-clusters', '--exclude-updating-clusters'],
        'exclude_stopped': ['--no-stopped-clusters', '--exclude-stopped-clusters'],
        'exclude_deleted': ['--no-deleted-clusters', '--exclude-deleted-clusters'],
        'exclude_error': ['--no-error-clusters', '--exclude-error-clusters'],
        'all': ['--all-clusters'],
    }
    if with_short_aliases:
        aliases = {}
        for i_option, i_aliases in short_aliases.items():
            aliases[i_option] = i_aliases + long_aliases[i_option]
    else:
        aliases = long_aliases

    def decorator(func):
        @click.option(
            *aliases['statuses'],
            'statuses',
            type=ListParamType(type=ClusterStatus()),
            help='Filter objects to output by specified cluster statuses.',
        )
        @click.option(
            *aliases['exclude_statuses'],
            'exclude_statuses',
            type=ListParamType(type=ClusterStatus()),
            help='Filter objects to not output by specified cluster statuses. Default behavior is to filter out'
            f' deleted clusters (clusters with statuses {", ".join(DELETED_CLUSTER_STATUSES)}).',
        )
        @click.option(*aliases['running'], 'running', is_flag=True, help='Include clusters in running state.')
        @click.option(
            *aliases['updating'],
            'updating',
            is_flag=True,
            help=f'Include updating clusters (clusters with statuses {", ".join(UPDATING_CLUSTER_STATUSES)}).',
        )
        @click.option(*aliases['stopped'], 'stopped', is_flag=True, help='Include clusters in stopped state.')
        @click.option(
            *aliases['deleted'],
            'deleted',
            is_flag=True,
            help=f'Include deleted clusters (clusters with statuses {", ".join(DELETED_CLUSTER_STATUSES)}).',
        )
        @click.option(
            *aliases['error'],
            'error',
            is_flag=True,
            help=f'Include clusters in error state (clusters with statuses {", ".join(ERROR_CLUSTER_STATUSES)}).',
        )
        @click.option(
            *aliases['exclude_running'], 'exclude_running', is_flag=True, help='Exclude clusters in running state.'
        )
        @click.option(
            *aliases['exclude_updating'],
            'exclude_updating',
            is_flag=True,
            help=f'Exclude updating clusters (clusters with statuses {", ".join(UPDATING_CLUSTER_STATUSES)}).',
        )
        @click.option(
            *aliases['exclude_stopped'], 'exclude_stopped', is_flag=True, help='Exclude clusters in stopped state.'
        )
        @click.option(
            *aliases['exclude_deleted'],
            'exclude_deleted',
            is_flag=True,
            help=f'Exclude deleted clusters (clusters with statuses {", ".join(DELETED_CLUSTER_STATUSES)}).',
        )
        @click.option(
            *aliases['exclude_error'],
            'exclude_error',
            is_flag=True,
            help=f'Exclude clusters in error state (clusters with statuses {", ".join(ERROR_CLUSTER_STATUSES)}).',
        )
        @click.option(*aliases['all'], 'all', is_flag=True)
        @wraps(func)
        def wrapper(
            *args,
            statuses,
            exclude_statuses,
            running,
            updating,
            stopped,
            deleted,
            error,
            exclude_running,
            exclude_updating,
            exclude_stopped,
            exclude_deleted,
            exclude_error,
            all,
            **kwargs,
        ):
            cluster_statuses = statuses or []
            exclude_cluster_statuses = exclude_statuses or []

            if running:
                cluster_statuses.append(RUNNING_CLUSTER_STATUS)

            if updating:
                cluster_statuses.extend(UPDATING_CLUSTER_STATUSES)

            if stopped:
                cluster_statuses.append(STOPPED_CLUSTER_STATUS)

            if deleted:
                cluster_statuses.extend(DELETED_CLUSTER_STATUSES)

            if error:
                cluster_statuses.extend(ERROR_CLUSTER_STATUSES)

            if exclude_running:
                exclude_cluster_statuses.append(RUNNING_CLUSTER_STATUS)

            if exclude_updating:
                exclude_cluster_statuses.extend(UPDATING_CLUSTER_STATUSES)

            if exclude_stopped:
                exclude_cluster_statuses.append(STOPPED_CLUSTER_STATUS)

            if exclude_deleted:
                exclude_cluster_statuses.extend(DELETED_CLUSTER_STATUSES)

            if exclude_error:
                exclude_cluster_statuses.extend(ERROR_CLUSTER_STATUSES)

            if all or (len(cluster_statuses) == 0 and statuses is None):
                cluster_statuses = None

            if all or (len(exclude_cluster_statuses) == 0 and exclude_statuses is None):
                exclude_cluster_statuses = None

            return func(
                *args, **kwargs, cluster_statuses=cluster_statuses, exclude_cluster_statuses=exclude_cluster_statuses
            )

        return wrapper

    return decorator


def generate_id(ctx):
    id_prefix = config_option(ctx, 'intapi', 'id_prefix')
    symbols = string.ascii_lowercase.replace('wxyz', '') + string.digits
    return id_prefix + ''.join(SystemRandom().choice(symbols) for _ in range(17))


def encrypt(ctx, secret):
    """
    Encrypt the specified secret.
    """
    encrypted_secret = _crypto_box(ctx).encrypt(secret.encode(), random(Box.NONCE_SIZE))
    data = URLSafeBase64Encoder.encode(encrypted_secret).decode()
    return {'encryption_version': 1, 'data': data}


def decrypt(ctx, secret):
    """
    Decrypt the specified secret.
    """
    raw_secret = URLSafeBase64Encoder.decode(secret['data'].encode())
    return _crypto_box(ctx).decrypt(raw_secret).decode()


def _crypto_box(ctx):
    secrets = _get_crypto_secrets(ctx)
    private_key = PrivateKey(secrets['private-key'].encode(), URLSafeBase64Encoder)
    public_key = PublicKey(secrets['public-key'].encode(), URLSafeBase64Encoder)
    return Box(private_key, public_key)


def _get_crypto_secrets(ctx):
    if 'crypto_secrets' not in ctx.obj:
        secret_id = get_config_key(ctx, ('intapi', 'crypto_secrets'))
        ctx.obj['crypto_secrets'] = vault.get_secret(secret_id)

    return ctx.obj['crypto_secrets']


def gen_random_string(length, symbols=(string.ascii_letters + string.digits)):
    return ''.join(SystemRandom().choice(symbols) for _ in range(length))


def get_oauth_token(ctx, config_section):
    ctx_key = f'{config_section}_token'

    if ctx_key not in ctx.obj:
        config = ctx.obj['config'][config_section]

        token = config.get('token')
        if not token:
            name = config['name']
            oauth_token_url = config['oauth_token_url']
            echo(
                f'Accessing {name} API requires additional configuration.'
                f' Please go to {oauth_token_url} in order to obtain OAuth token.'
            )
            token = prompt('Enter OAuth token', hide_input=True)
            update_config_key(ctx, f'{config_section}.token', token)
        elif token.startswith('sec-'):
            secret = vault.get_secret(token)
            token = secret.get('oauth') or secret.get('token')

        ctx.obj[ctx_key] = token

    return ctx.obj[ctx_key]


def format_references(ctx, config_section, macros):
    config = ctx.obj['config']

    predefined_macros = {
        'ui_url': config['ui']['url'],
        'accessible_folder_id': config['compute']['folder'] or config['ui']['accessible_folder_id'],
    }

    solomon_config = config.get('solomon')
    if solomon_config:
        predefined_macros['solomon_url'] = solomon_config['rest_endpoint']

    monitoring_config = config.get('monitoring')
    if monitoring_config:
        predefined_macros['monitoring_url'] = monitoring_config['url']

    admin_ui_config = config.get('admin_ui')
    if admin_ui_config:
        predefined_macros['admin_ui_url'] = admin_ui_config['url']

    backstage_ui_config = config.get('backstage_ui')
    if backstage_ui_config:
        predefined_macros['backstage_ui_url'] = backstage_ui_config['url']

    walle_ui_config = config.get('walle_ui')
    if walle_ui_config:
        predefined_macros['walle_ui_url'] = walle_ui_config['url']

    macros = {**predefined_macros, **macros}

    cluster_type = macros.get('cluster_type')
    if cluster_type:
        macros['service_name'] = to_service_name(cluster_type)

    references = OrderedDict()
    for entry in config[config_section]:
        cluster_types = entry.get('cluster_types')
        if cluster_types and cluster_type not in cluster_types:
            continue

        references[entry['name']] = entry['url'].format_map(macros)

    return references


def format_hosts(hosts):
    return ','.join(host['fqdn'] for host in hosts)


def debug_mode(ctx):
    """
    Return True if debug mode is enabled, or False otherwise.
    """
    return ctx.obj['debug']


def dry_run_mode(ctx):
    """
    Return True if dry-run mode is enabled, or False otherwise.
    """
    return ctx.obj['dry_run']


def open_ssh_tunnel_if_required(ctx, host, port=None):
    """
    Open ssh tunnel for the specified host and port if jumphost is configured. Otherwise, do nothing and return None.
    """
    jumphost_config = get_config(ctx).get('jumphost', {})
    if not jumphost_config:
        return None

    if not port:
        host, port = host.rsplit(':', 1)
        port = int(port)

    sshtunnel.SSH_TIMEOUT = 10

    bind_port = get_free_port()
    debug_level = logging.DEBUG if debug_mode(ctx) else logging.CRITICAL
    return sshtunnel.open_tunnel(
        ssh_address_or_host=(jumphost_config['host'], 22),
        ssh_username=jumphost_config['user'],
        remote_bind_address=(host, port),
        local_bind_address=('127.0.0.1', bind_port),
        debug_level=debug_level,
    )


def get_free_port():
    with socket.socket() as s:
        s.bind(('', 0))
        return s.getsockname()[1]


def get_polling_interval(ctx):
    """
    The number of seconds to sleep between reloading object state. For example, while waiting for task completion.
    """
    return get_config(ctx).get('polling_interval', 5)


def to_service_name(cluster_type):
    """
    Convert cluster type to service name.

    "clickhouse" -> "managed-clickhouse"
    "postgresql" -> "managed-postgresql"
    "hadoop" -> "data-proc"
    ...
    """
    if cluster_type == 'hadoop':
        return 'data-proc'
    else:
        return f'managed-{cluster_type}'
