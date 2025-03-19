#!/usr/bin/env pymds

import json
import argparse

import pymongo


CONFIG_PATH = '/etc/elliptics/mastermind.conf'
JOBS_COLLECTION_NAME = 'jobs'


def get_config():
    try:
        with open(CONFIG_PATH, 'r') as config_file:
            return json.load(config_file)
    except Exception as e:
        raise ValueError('Failed to load config file {}: {}'.format(CONFIG_PATH, e))


def get_mongo_client(config):
    metadata_url = config.get('metadata', {}).get('url', '')
    if not metadata_url:
        raise ValueError('Mongo db url is not set')
    options = config.get('metadata', {}).get('options', {})
    if 'tls' not in options:
        options['tls'] = True  # to not run without tls
    if options['tls'] and 'tlsCAFile' not in options:
        options['tlsCAFile'] = "/usr/share/yandex-internal-root-ca/YandexInternalRootCA.crt"
        # yandex cert from yandex-internal-root-ca

    return pymongo.mongo_replica_set_client.MongoReplicaSetClient(metadata_url)
    # return pymongo.mongo_replica_set_client.MongoReplicaSetClient(metadata_url, **options)


def get_jobs_collection(config, mongo_client):
    db_name = config['metadata']['jobs']['db']
    return mongo_client[db_name][JOBS_COLLECTION_NAME]


def get_pending_jobs_cursor(
    config,
    mongo_client,
    only_job_types=None,
    exclude_job_types=None,
    exclude_locked=True,
    exclude_attached=True,

):
    coll = get_jobs_collection(config, mongo_client)
    query = {'status': {'$in': ['pending']}}

    if only_job_types is not None or exclude_job_types is not None:
        query['type'] = {}
        if only_job_types is not None:
            query['type']['$in'] = only_job_types
        if exclude_job_types is not None:
            query['type']['$nin'] = exclude_job_types

    if exclude_locked:
        query['locked_by'] = {'$exists': False}
    if exclude_attached:
        query['attached_tickets'] = {'$exists': False}

    projection = {'id': 1, 'type': 1, 'tasks': 1}
    cursor = coll.find(query, projection)

    return cursor


def print_retry_task_commands(jobs_cursor):
    for job in jobs_cursor:
        tasks = job['tasks']
        for task in tasks:
            if task['status'] == 'failed':
                try:
                    print("mastermind job retry_task {} {}".format(job['id'], task['id']))
                except Exception as e:
                    print(e)


def parse_list_arg(raw_val):
    if raw_val is None:
        return raw_val
    return [v.strip() for v in raw_val.split(',')]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--only-job-types', help='Restart only these job types (example: "move_job,restore_job")', action='store'
    )
    parser.add_argument(
        '--exclude-job-types', help='Do not restart these job types', action='store'
    )
    parser.add_argument(
        '--allow-locked', help='Restart locked jobs (they are ignored by default)', action='store_true'
    )
    parser.add_argument(
        '--allow-attached-tickets', help='Restart jobs with attached tickets (they are ignored by default)', action='store_true'
    )

    args = parser.parse_args()
    only_job_types = parse_list_arg(args.only_job_types)
    exclude_job_types = parse_list_arg(args.exclude_job_types)
    exclude_locked = not args.allow_locked
    exclude_attached = not args.allow_attached_tickets

    config = get_config()
    mongo_client = get_mongo_client(config)
    jobs_cursor = get_pending_jobs_cursor(
        config,
        mongo_client,
        only_job_types,
        exclude_job_types,
        exclude_locked,
        exclude_attached,
    )

    print_retry_task_commands(jobs_cursor)
