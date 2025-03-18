#!/skynet/python/bin/python
import os
import sys
import json
import bson
import time
import logging
import datetime
import functools

import pymongo

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
import common
import mongo_params

from core.db import CURDB

LOG_FORMAT = '%(levelname)s\t%(asctime)s\t%(message)s'
SECTIONS = ['hosts_to_groups', 'hosts_to_hardware']


def work_time(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        r_value = func(*args, **kwargs)
        logging.debug('WORKTIME {} {}s'.format(func.__name__, time.time() - start))
        return r_value
    return wrapper


def get_mongo_collection(collection_name, test=False):
    return pymongo.MongoReplicaSetClient(
        mongo_params.ALL_HEARTBEAT_C_MONGODB.uri,
        connectTimeoutMS=5000,
        replicaSet=mongo_params.ALL_HEARTBEAT_C_MONGODB.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=mongo_params.ALL_HEARTBEAT_C_MONGODB.read_preference
    )['test_topology_commits' if test else 'topology_commits'][collection_name]


def get_latest_commit(collection):
    for record in collection.find({}, fields={'commit': 1}, sort=[('commit', pymongo.DESCENDING)], limit=1):
        commit = record['commit']
        return commit
    return None


def get_oldest_commit(commit, leave_last_n=100, max_time_delta=datetime.timedelta(hours=7)):
    if commit is None:
        return None

    count = 0
    time_delta = bson.ObjectId.from_datetime(datetime.datetime.utcnow() - max_time_delta)
    for record in get_mongo_collection('commits', test=False).find(
        {
            'test_passed': True,
            'commit': {'$lt': str(commit)}
        },
        sort=[('commit', -1)]
    ):
        count += 1
        if record['_id'] < time_delta and count >= leave_last_n:
            return int(record['commit'])
    return None


@work_time
def upload_data_to_mongo(collection, key_field, data):
    commit = int(common.get_current_commit('./'))
    current_datetime = datetime.datetime.utcnow()

    bulk_op = None
    for i, (key, value) in enumerate(data.items()):
        if bulk_op is None:
            bulk_op = collection.initialize_unordered_bulk_op()

        bulk_op.find({key_field: key, 'commit': commit}).upsert().replace_one({
            key_field: key,
            'data': value,
            'commit': commit,
            'datetime': current_datetime
        })

        if i and i % 1000 == 0:
            bulk_op.execute()
            logging.debug('Inserted 1000 docs')
            bulk_op = None

    if bulk_op is not None:
        bulk_op.execute()
        logging.debug('Inserted last chunk docs')
    logging.info('Finish inserting {} docs'.format(len(data)))

    return commit


@work_time
def remove_old_data_from_mongo(collection, commit, max_time_delta=datetime.timedelta(hours=7)):
    if commit is None:
        return
    collection.remove({'commit': {'$lte': commit}})


@work_time
def hosts_to_groups():
    groups_by_hosts = {}
    for host in CURDB.hosts.get_hosts():
        groups_by_hosts[host.name] = sorted(x.card.name for x in CURDB.groups.get_host_groups(host))
    return groups_by_hosts


@work_time
def hosts_to_hardware():
    hardware_by_hosts = {}
    for host in CURDB.hosts.get_hosts():
        hardware_by_hosts[host.name] = host.save_to_json_object()
    return hardware_by_hosts


def parse_cmd():
    parser = ArgumentParser(description='Script to upload host info trunk data to mongo')

    parser.add_argument('-s', '--section', type=str, required=True, choices=SECTIONS,
                        help='Name of section to upload')

    parser.add_argument('--dry-run', action='store_true', default=False,
                        help='Optional. Run but not modify anything.')

    parser.add_argument('--prod-run', action='store_true', default=False,
                        help='Optional. Run and write to PROD mongo db.')

    parser.add_argument('--test-run', action='store_true', default=False,
                        help='Optional. Run and write to test mongo db.')

    parser.add_argument('--auto-clean', action='store_true', default=False,
                        help='Optional. Remove old data after upload new from mongo db.')

    parser.add_argument('--prod-clean', action='store_true', default=False,
                        help='Optional. Clean old data from PROD mongo db.')

    parser.add_argument('--test-clean', action='store_true', default=False,
                        help='Optional. Clean old data from test mongo db.')

    parser.add_argument('--dump', action='store_true', default=False,
                        help='Optional. Dump data to stdout.')

    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def run_clean_mongo(options):
    collection = None

    # Remove old data from mongo
    if options.test_clean:
        logging.info('START clean test MongoDB')
        collection = get_mongo_collection(options.section, test=True)

    elif options.prod_clean:
        logging.info('START clean PROD MongoDB')
        collection = get_mongo_collection(options.section, test=False)

    if collection is not None:
        commit = get_latest_commit(collection)
        oldest_commit = get_oldest_commit(commit)
        remove_old_data_from_mongo(collection, oldest_commit)


def run_upload_data(options):
    # Prepare data from working copy
    logging.info('START preparing data from working copy')
    data, key_field = None, None
    if options.section == 'hosts_to_groups':
        data = hosts_to_groups()
        key_field = 'hostname'
    elif options.section == 'hosts_to_hardware':
        data = hosts_to_hardware()
        key_field = 'hostname'

    collection, commit = None, None

    # Upload data to mongo
    if options.dry_run:
        logging.info('SKIP upload to MongoDB')

    elif options.test_run:
        logging.info('START upload to test MongoDB')
        collection = get_mongo_collection(options.section, test=True)
        commit = upload_data_to_mongo(collection, key_field, data)

    elif options.prod_run:
        logging.info('START upload to PROD MongoDB')
        collection = get_mongo_collection(options.section, test=False)
        commit = upload_data_to_mongo(collection, key_field, data)

    else:
        logging.info('SKIP upload to MongoDB')

    # Remove old commit if needed
    if (options.test_run or options.prod_run) and options.auto_clean:
        logging.info('START remove old data from MongoDB')
        oldest_commit = get_oldest_commit(commit)
        remove_old_data_from_mongo(collection, oldest_commit)

    # Print data to stdout
    if options.dump:
        print(json.dumps(data, indent=4))


@work_time
def main():
    options = parse_cmd()

    if options.verbose:
        logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT)
    elif options.dump:
        logging.basicConfig(level=logging.CRITICAL, format=LOG_FORMAT)
    else:
        logging.basicConfig(level=logging.INFO, format=LOG_FORMAT)

    # Check for uniqueness
    if (options.dry_run + options.test_run + options.prod_run + options.test_clean + options.prod_clean) > 1:
        logging.critical('Only one of {{--dry-run, --prod-run, --test-run, --prod-clean, --test-clean}} option')
        raise ValueError('Only one of {{--dry-run, --prod-run, --test-run, --prod-clean, --test-clean}} option')

    # Select clean or upload
    if options.test_clean or options.prod_clean:
        run_clean_mongo(options)
    else:
        run_upload_data(options)

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
