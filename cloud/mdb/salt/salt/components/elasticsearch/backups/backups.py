#!/usr/bin/env python3

import argparse
import json
import socket
import logging
import time

from elasticsearch import Elasticsearch, exceptions

from boto3 import Session
from botocore.config import Config
from botocore.exceptions import ClientError


CONFIG_FILE = '/etc/yandex/mdb-elasticsearch/backups.conf'
REPOSITORY = 'yc-automatic-backups'
S3_META_KEY = 'backups/.yc-metadata.json'
EXTENSIONS_PREFIX = 'object_cache/extensions/'

logger = logging.getLogger('es-backups')

def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true')

    subparsers = parser.add_subparsers(dest='command')

    parser_update = subparsers.add_parser('update-meta')
    parser_update.add_argument('--force', action='store_true')    
    parser_update.set_defaults(cmd=cmd_update_metadata)

    parser_check_age = subparsers.add_parser('check-age')
    parser_check_age.set_defaults(cmd=cmd_check_age)

    parser_sync_repo = subparsers.add_parser('sync-repo')
    parser_sync_repo.set_defaults(cmd=cmd_sync_repo)

    return parser.parse_args()


def cmd_sync_repo(args):
    config = load_config()

    if not config['enabled']:
        logger.info('Skip repository sync: backups disabled')
        return 

    client = _get_client(config.get('elastic'))
    do_sync(client, config)


def do_sync(client, config):
    logger.info('Syncing shapshot repository and policy')
    sync_repo(client, config.get('repository'))
    sync_policy(client, config.get('policy'))


def sync_repo(client, cfg):
    repository_settings = {}
    try:
        repo_resp = client.snapshot.get_repository(cfg.get('name'))
        repository_settings = repo_resp[cfg.get('name')]['settings']
    except exceptions.NotFoundError:
        pass

    if not repository_settings == cfg.get('settings'):
        logger.info('Invalid repository settings, update')
        client.snapshot.create_repository(cfg.get('name'), {
            'type': 's3',
            'settings': cfg.get('settings')
        })

    logger.info('Snapshot repository synced')


def sync_policy(client, cfg):
    policy = {}
    try:
        policy_resp = client.transport.perform_request('GET', '/_slm/policy/' + cfg.get('name'))
        policy = policy_resp[cfg.get('name')]['policy']
    except exceptions.NotFoundError:
        pass
 
    if not policy == cfg.get('settings'):
        logger.info('Invalid policy settings, update')
        client.transport.perform_request('PUT', '/_slm/policy/' + cfg.get('name'), body = cfg.get('settings'))

    logger.info('Snapshot policy synced')


def cmd_check_age(args):
    """
    Monrun check for latest backups age
    """
    logging.disable(logging.CRITICAL)
    config = load_config()

    if not config['enabled']:
        print('0;disabled')
        return 

    client = _get_client(config.get('elastic'))

    if not is_master(client):
        print('0;skipped, not master')
        return 

    s3client = _s3_client(config.get('s3'))
    bucket = config.get('s3').get('bucket')

    metadata = load_metadata(s3client, bucket)

    if len(metadata) == 0:
        # TODO any ideas ?
        print('1;no backups')
        return 

    latest = max(metadata, key = lambda m: m['end_time_ms'])
    now = int(time.time()) * 1000
    age = (now - latest['end_time_ms'])/ (60*60*1000)

    if age > 5:
        print('2;backup too old, %dh' % age)
        return 

    if age > 2:
        print('1;backup too old, %dh' % age)
        return 

    print('0;OK')
    return


def cmd_update_metadata(args):
    """
    Updates backups metadata
    """
    config = load_config()

    client = _get_client(config.get('elastic'))

    logger.info('Check node is master')
    if not is_master(client) and not args.force:
        logger.info('Skip metadata update: not master')
        return

    do_sync(client, config)

    logger.info('Start metadata update')

    s3client = _s3_client(config.get('s3'))
    bucket = config.get('s3').get('bucket')

    logger.info('Get actual snapshots from elasticsearch')
    snapshots = get_snapshots(client)
    logger.debug('Actual snapshots: %d', len(snapshots))

    logger.info('Loading metadata from store')
    metadata = load_metadata(s3client, bucket)
    logger.debug('Snapshots in metadata: %d', len(metadata))

    added = 0
    removed = 0
    # remove meta for deleted snapshots
    tmp = []
    for m in metadata:
        if has_snapshot(snapshots, m['id']):
            tmp.append(m)
        else:
            removed += 1
            logger.debug('Remove snapshot: %s', m['id'])

    metadata = tmp

    # add meta for new snapshots
    added = 0
    for s in snapshots:
        if not has_snapshot(metadata, s['id']):
            added += 1
            meta = get_snapshot_metadata(client, config.get('cluster_id'), s['id'], config.get('index_details'))
            meta['extensions'] = config.get('extensions').get('active')
            logger.debug('Add snapshot: %s', s['id'])
            metadata.append(meta)

    if added == 0 and removed == 0:
        logger.info('Metadata not changed')
    else:
        logger.info('Uploading updated metadata. Snapshots added %d / removed %d', added, removed)
        store_metadata(s3client, bucket, metadata)

    logger.info('Done metadata update')

    logger.info('Cleaning up extensions')
    cleanup_extensions(s3client, bucket, get_extensions(metadata) | set(config.get('extensions').get('all')))
    logger.info('Done cleaning up extensions')


def has_snapshot(snapshots, sid):
    for s in snapshots:
        if s['id'] == sid:
            return True
    return False


def get_snapshots(client):
    snapshots = client.cat.snapshots(repository=REPOSITORY, params={'h': 'id,status', 'format': 'json'}, request_timeout=60) # ignore_unavailable?
    # drop in-progress,failed, incompatible, partial snapshots
    return [s for s in snapshots if s['status'] == 'SUCCESS']


def get_snapshot_metadata(client, cid, id, index_details):
    params = {}
    if index_details:
        params['index_details'] = 'true'
    data = client.snapshot.get(repository=REPOSITORY, snapshot=id, params=params)['snapshots'][0]

    size = 0
    if index_details:
        for v in data['index_details'].values():
            size += v['size_in_bytes']

    return {
        'id': id,
        'cluster_id': cid,
        'version': data['version'],
        'start_time_ms': data['start_time_in_millis'],
        'end_time_ms': data['end_time_in_millis'],
        'indices_total': len(data['indices']),
        'indices': data['indices'] if len(data['indices']) <= 100 else data['indices'][:100],
        'size': size
    }


def load_metadata(s3client, bucket):
    meta = {'version':1,'snapshots':[]}

    try:
        meta = json.loads(s3client.get_object(Bucket=bucket, Key=S3_META_KEY)['Body'].read())
    except ClientError as e:
        if _error_code(e) not in ('404', 'NoSuchKey'):
            raise

    return meta.get('snapshots')


def store_metadata(s3client, bucket, metadata):
    meta = {'version':1, 'snapshots':metadata}
    s3client.put_object(Bucket=bucket, Key=S3_META_KEY, Body=json.dumps(meta))


def get_extensions(metadata):
    extensions = set()
    for m in metadata:
        if 'extensions' in m:
            extensions |= set(m['extensions'])
    return extensions


def cleanup_extensions(s3client, bucket, extensions):
    resp = s3client.list_objects_v2(Bucket=bucket, Prefix=EXTENSIONS_PREFIX)
    delete = []
    if 'Contents' in resp:
        for o in resp.Contents:
            eid = o.Key[len(EXTENSIONS_PREFIX):-4]
            if not eid in extensions:
                logger.debug('Remove extension {}'.format(eid))
                delete.push({'Key':o.Key})

    if len(delete) > 0:
        s3client.delete_objects(Bucket=bucket, Delete={'Objects':delete})


def printj(v):
    print(json.dumps(v, indent=4))


def load_config():
    with open(CONFIG_FILE, 'r') as f:
        return json.load(f)


def _s3_client(cfg):
    """
    Create and return S3 client.
    """
    session = Session(
        aws_access_key_id=cfg.get('access_key_id'),
        aws_secret_access_key=cfg.get('access_secret_key'))

    endpoint_url = cfg.get('endpoint')
    if endpoint_url:
        endpoint_url = endpoint_url.replace('+path://', '://')

    s3_config = {
        'addressing_style': 'path',
    }

    if cfg.get('virtual_addressing_style'):
        s3_config['addressing_style'] = 'virtual'

    if cfg.get('region'):
        s3_config['region_name'] = cfg.get('region')

    return session.client(
        service_name = 's3',
        endpoint_url = endpoint_url,
        config = Config(s3 = s3_config, retries = {
            'max_attempts': 3,
            'mode': 'standard'
        }))


def _error_code(exception):
    return exception.response.get('Error', {}).get('Code')


def _get_client(cfg):
    """
    Create and return Elasticsearch client.
    """
    hosts = [
        {
            'host': socket.gethostname(),
            'port': 9200,
        }
    ]
    return Elasticsearch(
        hosts,
        use_ssl=True,
        verify_certs=True,
        http_auth=(cfg.get('user'), cfg.get('password')),
        ca_certs='/etc/ssl/certs/ca-certificates.crt'
    )


def is_master(client):
    return client.cat.master(format='json')[0]['host'] == socket.getfqdn()    


def main():
    args = parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    args.cmd(args)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)5s] %(name)-20s: %(message)s')
    main()
