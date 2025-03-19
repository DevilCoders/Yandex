"""
Helpers for working with yandexcloud sdk
"""

import os
import json
import base64
import getpass
import logging

import certifi
import yandexcloud

from helpers import (
    cloud_logging,
    iam,
    compute,
    vpc,
    ssh,
    tunnel,
    resourcemanager,
    s3,
    vault,
    mdb_clickhouse,
)

LOG = logging.getLogger('yandexcloud')

SA_ROLES = [
    'dataproc.agent',  # Currently doesn't need?
    'storage.uploader',
    'storage.viewer',
    'storage.admin',  # Needs for create and delete buckets
    'logging.writer',
]


def get_service_account_key(ctx):
    """
    Load service account credentials
    """
    # Return service_acount_key from file if exists
    if os.path.exists('key.json'):
        with open('key.json') as fh:
            return json.load(fh)
    secret_id = ctx.conf.get('sa_key_secret_id')
    if not secret_id:
        raise Exception('Please, specify sa_key_secret_id or key.json for service-account')
    secrets = vault.get_version(ctx, secret_id, packed_value=False)['value']
    for s in secrets:
        if s.get('key') == 'key.json':
            try:
                parsed = json.loads(base64.b64decode(s['value']))
                return parsed
            except Exception as exc:
                raise Exception(f'Failed to parse {secret_id}', exc)
    raise Exception(f'Secret {secret_id} doesn\'t contain key.json')


def init_yandexsdk(ctx):
    """
    Initialize sdk for working
    """
    standard_ca = open(certifi.where(), 'rb').read()
    internal_ca_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'allCAs.pem')
    if not os.path.exists(internal_ca_path):
        raise RuntimeError(f'Can not find file with root CAs {internal_ca_path}\nPlease, run "make allCAs.pem"')
    internal_ca = open(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'allCAs.pem'), 'rb').read()
    ctx.state['yandexsdk'] = yandexcloud.SDK(
        endpoint=ctx.conf['environment']['endpoint'],
        service_account_key=get_service_account_key(ctx),
        root_certificates=standard_ca + internal_ca,
    )


def setup_labels(ctx):
    """
    Compose labels for all cloud resources
    """
    ctx.labels = {
        'owner': getpass.getuser(),
    }
    if os.environ.get('COMMIT'):
        ctx.labels['commit'] = os.environ['COMMIT']
    if os.environ.get('BRANCH'):
        ctx.labels['branch'] = os.environ['BRANCH']
    ctx.labels['pr'] = os.environ.get('ARCANUMID', 'null')


def is_evictable(ctx, resource):
    """
    Filter resources that should be evicted
    """
    labels = {}
    if hasattr(resource, 'labels'):
        labels = resource.labels
    elif hasattr(resource, 'description'):
        # Backup plan for s3 bucket and iam service-account
        # If resource doesn't have labels attr see to description
        try:
            labels = json.loads(resource.description)
        except ValueError:
            # resource doesn't look like from tests, just skip it.
            return False
    else:
        # resource doesn't look like from tests, just skip it.
        return False
    if labels.get('owner') != ctx.labels['owner']:
        return False
    if ctx.labels['owner'] == 'robot-pgaas-ci' and \
            labels.get('branch') != ctx.labels['branch']:
        return False
    return True


def delete_resources(ctx):
    """
    Clean resources for current branch
    """
    # Delete compute instances
    to_delete = []
    for instance in compute.instances_list(ctx):
        if is_evictable(ctx, instance):
            LOG.info(f'Deleting outdated compute instance {instance.name}')
            to_delete.append(instance.name)
    compute.instances_delete(ctx, to_delete)
    # Delete mdb clusters
    to_delete = []
    for cluster in mdb_clickhouse.clusters_list(ctx):
        if is_evictable(ctx, cluster):
            LOG.info(f'Deleting outdated mdb_clickhouse {cluster.name}')
            to_delete.append(cluster.id)
    mdb_clickhouse.clusters_delete(ctx, to_delete)
    # Delete vpc entities
    for subnet in vpc.subnets_list(ctx):
        if is_evictable(ctx, subnet):
            LOG.debug(f'Deleting outdated vpc subnet {subnet.name}')
            vpc.subnet_delete(ctx, subnet.name)
    for net in vpc.networks_list(ctx):
        if is_evictable(ctx, net):
            LOG.debug(f'Deleting outdated vpc network {net.name}')
            vpc.network_delete(ctx, net.name)
    for log_group in cloud_logging.log_groups_list(ctx):
        if is_evictable(ctx, log_group):
            LOG.debug(f'Deleting outdated log group {log_group.name}')
            cloud_logging.delete_log_group(ctx, log_group.id)
    # For deleting outdated s3 bucket we need a pair of keys.
    # So we can not do that before creating a new service account and credentials.
    # Delete outdated bucket only if s3 client exists, after test run.
    if ctx.state.get('s3'):
        for bucket in s3.bucket_list(ctx):
            if is_evictable(ctx, bucket):
                LOG.debug(f'Deleting outdated s3 bucket {bucket.name} with all objects')
                s3.bucket_delete(ctx, bucket.name)
    for service_account in iam.service_accounts_list(ctx):
        if is_evictable(ctx, service_account):
            LOG.debug(f'Deleting outdated iam service_account {service_account}')
            iam.service_account_delete(ctx, service_account.name)
            continue


def setup_logging():
    """
    Setup logging attributes for libraries
    """
    log_level = logging.getLevelName(os.environ.get('LOGGING', 'WARNING'))
    for logger in (
            'sh.command',
            'vpc',
            'compute',
            'dataproc',
            'iam',
            'tunnel',
            'zeppelin',
            'mdb_clickhouse',
            'resourcemanager'):
        logging.getLogger(logger).setLevel(log_level)


def setup_environment(ctx):
    """
    Setup all cloud resources before running tests
    """
    setup_logging()
    # Dictionary for cleanup all created resources
    ctx.state = {
        'dnscache': {},
        'resources': {
            'instances': {},
            'networks': {},
            'subnets': {},
            'service_accounts': {},
            'log_groups': {},
        },
        'tunnels': {},
        'use_ipv6': False,
    }
    conf = ctx.conf
    setup_labels(ctx)
    init_yandexsdk(ctx)

    LOG.info(f'Test id: {ctx.id}')

    # Create new temporary network if user did not provide it
    env = conf['environment']
    if not env.get('network_id'):
        env['network_id'] = vpc.network_create(ctx, ctx.id)
    if not env.get('log_group_id'):
        env['log_group_id'] = cloud_logging.create_log_group(ctx)
    if not env.get('subnet_id'):
        env['subnet_id'] = vpc.subnet_create(ctx,
                                             ctx.id,
                                             env['zone'],
                                             ctx.state['resources']['networks'][ctx.id],
                                             conf['vpc']['v4_cidr_block'])
    else:
        # rewrite v4_cidr_blocks if subnet_id provided
        subnet = vpc.subnet_info(ctx, env['subnet_id'])
        if hasattr(subnet, 'v4_cidr_blocks'):
            conf['vpc']['v4_cidr_block'] = subnet.v4_cidr_blocks[0]
        if hasattr(subnet, 'v6_cidr_blocks') and subnet.v6_cidr_blocks != []:
            ctx.state['use_ipv6'] = True

    if not ctx.conf['compute'].get('service_account_id'):
        sa_id = iam.service_account_create(ctx,
                                           ctx.id,
                                           description=json.dumps(ctx.labels))
        ctx.conf['compute']['service_account_id'] = sa_id
        resourcemanager.service_account_update_roles(ctx,
                                                     sa_id,
                                                     SA_ROLES)
        keys = iam.service_account_create_aws_credentials(ctx, sa_id, 'static credentials for dataproc-image tests')
        ctx.state['s3_keys'] = keys
        s3.client_get(ctx, keys)
        s3.bucket_create(ctx, ctx.id)
    if not ctx.conf['compute'].get('image_id'):
        ctx.conf['compute']['image_id'] = compute.get_image_by_family(ctx,
                                                                      'dataproc-image-1-4')
    ssh.load_key(ctx)
    # Setup cluster structure for tests
    ctx.state['clusters'] = {}
    # Setup ctx for jinja2 variables
    ctx.state['render'] = {
        'bucket': ctx.id,
        'owner': ctx.labels['owner'],
        'commit': ctx.labels['commit'],
        'branch': ctx.labels['branch'],
        'pr': ctx.labels['pr'],
        'conf': ctx.conf,
        'clickhouse_db': ctx.conf['mdb']['clickhouse']['db'],
        'clickhouse_user': ctx.conf['mdb']['clickhouse']['user'],
        'clickhouse_password': ctx.conf['mdb']['clickhouse']['password'],
    }


def is_disabled_cleanup():
    """
    Should we disable cleanup?
    """
    if os.environ.get('ON_ERROR', 'cleanup') == 'stop':
        return True
    return False


def teardown_environment(ctx):
    """
    Removing all created cloud resources
    """
    if is_disabled_cleanup():
        LOG.debug('teardown disabled by ON_ERROR environment variable')
        return
    LOG.debug('Starting teardown environment')
    tunnel.tunnels_clean(ctx)
    ssh.clean_key(ctx)
    delete_resources(ctx)
