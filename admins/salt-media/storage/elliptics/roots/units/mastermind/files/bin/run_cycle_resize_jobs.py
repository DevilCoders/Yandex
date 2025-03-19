#!/usr/bin/pymds
import sys
import time
import ast
import json
import argparse
import logging
import subprocess
import shlex

import pymongo

from mastermind import CollectorClient, CollectorGrpcOptions
import mds.mastermind.snapshot.bindings.python.mastermind_snapshot as mastermind_snapshot


JOBS_ACTIVE_STATUSES = ['not_approved', 'new', 'executing', 'pending', 'broken']
TYPE_REPLICAS_REDUCE_JOB = 'replicas_reduce_job'
TYPE_REPLICAS_INCREASE_JOB = 'replicas_increase_job'

CONFIG_PATH = '/etc/elliptics/mastermind.conf'
JOBS_COLLECTION_NAME = 'jobs'

DEFAULT_LOOP_DELAY = 600


def get_config():
    try:
        with open(CONFIG_PATH, 'r') as config_file:
            return json.load(config_file)
    except Exception as e:
        raise ValueError('Failed to load config file {}: {}'.format(CONFIG_PATH, e))


def get_logger(debug=False):
    root_logger = logging.getLogger()

    level = logging.DEBUG if debug else logging.INFO
    root_logger.setLevel(level)

    handler = logging.FileHandler("/var/log/run_cycle_resize_jobs.log")
    handler.setFormatter(logging.Formatter(fmt='[%(asctime)s] [%(levelname)s] %(message)s'))
    root_logger.addHandler(handler)

    return root_logger


def get_mongo_client(config):
    metadata_url = config.get('metadata', {}).get('url', '')
    if not metadata_url:
        raise ValueError('Mongo db url is not set')
    return pymongo.mongo_replica_set_client.MongoReplicaSetClient(metadata_url)


def get_jobs_collection(config, mongo_client):
    db_name = config.get('metadata', {}).get('jobs', {}).get('db', '')
    return mongo_client[db_name][JOBS_COLLECTION_NAME]


def get_collector_client(config, traceid=None):
    storage_state_source = config.get('storage_state_source', {})
    collector_addresses = storage_state_source.get('addresses')
    collector_timeout = storage_state_source.get('timeout', 30)
    return CollectorClient(
        addresses=collector_addresses,
        trace_id=traceid,
        options=CollectorGrpcOptions(
            get_timeout_sec=collector_timeout, make_timeout_sec=collector_timeout,
        ),
    )


def get_snapshot(collector_client):
    fb_snapshot = collector_client.get_snapshot()
    snapshot = mastermind_snapshot.get_snapshot(fb_snapshot)
    return snapshot


class ResizeJobsStarter(object):
    def __init__(self, config, logger, mongo_client, snapshot, autoapprove=False):
        self._logger = logger
        self._snapshot = snapshot
        self._autoapprove = autoapprove

        self._jobs_collection = get_jobs_collection(config, mongo_client)

    def _get_resize_job(self, x2_groupset_id):
        """Get running resize job of groupset `x2_groupset_id`"""
        jobs = list(
            self._jobs_collection.find(
                {
                    'type': {'$in': [TYPE_REPLICAS_REDUCE_JOB, TYPE_REPLICAS_INCREASE_JOB]},
                    'status': {'$in': JOBS_ACTIVE_STATUSES},
                },
                {'id': 1, 'type': 1, 'groupset_id': 1, 'status': 1},
            )
        )

        for job in jobs:
            if job['groupset_id'].startswith(x2_groupset_id):
                return job

        return None

    def _start_reduce_job(self, groupset_id, new_size):
        self._logger.info(
            'Start %s job, groupset_id=%s, new_size=%d',
            TYPE_REPLICAS_REDUCE_JOB,
            groupset_id,
            new_size,
        )

        command = "mastermind groupset reduce {groupset_or_group} {new_size}".format(
            groupset_or_group=groupset_id, new_size=new_size
        )
        self._start_job(command)

    def _start_increase_job(self, groupset_id, new_size):
        self._logger.info(
            'Start %s job, groupset_id=%s, new_size=%d',
            TYPE_REPLICAS_INCREASE_JOB,
            groupset_id,
            new_size,
        )

        command = "mastermind groupset increase {groupset_or_group} {new_size}".format(
            groupset_or_group=groupset_id, new_size=new_size
        )
        self._start_job(command)

    def _start_job(self, cmd):
        if self._autoapprove:
            cmd = cmd + ' -a'

        p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
        p.wait()
        stdout = p.stdout.read()

        if p.returncode == 0:
            job_info = ast.literal_eval(stdout)  # stdout is text representation of python dict
            job_id = job_info["id"]
            create_ts = job_info["create_ts"]

            self._logger.info('Job created, id=%s, create_ts=%s', job_id, create_ts)

        elif 'LockAlreadyAcquiredError' in stdout:
            self._logger.info('Failed to create job because of already acquired lock')
        else:
            self._logger.error(
                'Failed to create job for unknown reason, returncode=%s, mm cli stdout: %s',
                p.returncode,
                stdout,
            )

    def run(self, x2_groupset_id):
        """Start resize job if no other resize job is running

        Args:
            x2_groupset_id (str) id of x2 groupset, can be prefix of x3 groupset
        """

        running_job = self._get_resize_job(x2_groupset_id)
        if running_job is not None:
            self._logger.info(
                'Job is running, x2_groupset_id=%s, groupset_id=%s, job_id=%s, status=%s',
                x2_groupset_id,
                running_job['groupset_id'],
                running_job['id'],
                running_job['status'],
            )
            return

        found_gs = None
        for gs in self._snapshot.groupsets:
            if gs.id.startswith(x2_groupset_id):
                found_gs = gs
                break

        if found_gs is None:
            self._logger.error("Groupset with prefix '%s' not found in snapshot", x2_groupset_id)
            return

        gs_id = found_gs.id
        gs_size = len(gs_id.split(':'))
        gs_prefix_size = len(x2_groupset_id.split(':'))

        if gs_size == gs_prefix_size:
            self._start_increase_job(gs_id, gs_size + 1)
        elif gs_size == gs_prefix_size + 1:
            self._start_reduce_job(gs_id, gs_size - 1)
        else:
            self._logger.error(
                'Found groupset with wrong size, x2_groupset_id=%s, groupset_id=%s',
                x2_groupset_id,
                gs_id,
            )


def parse_groupset_id_prefixes(raw_value):
    return raw_value.split(',')


def single_run_cmd(args):
    config = get_config()
    logger = get_logger(debug=args.debug_logging)
    mongo_client = get_mongo_client(config)
    collector_client = get_collector_client(config)
    snapshot = get_snapshot(collector_client)

    groupset_id_prefixes = parse_groupset_id_prefixes(args.groupset_id_prefixes)
    jobs_starter = ResizeJobsStarter(config, logger, mongo_client, snapshot, args.autoapprove)

    for gs_id_prefix in groupset_id_prefixes:
        jobs_starter.run(gs_id_prefix)


def loop_run_cmd(args):
    config = get_config()
    logger = get_logger(debug=args.debug_logging)
    mongo_client = get_mongo_client(config)
    collector_client = get_collector_client(config)
    snapshot = get_snapshot(collector_client)

    groupset_id_prefixes = parse_groupset_id_prefixes(args.groupset_id_prefixes)
    jobs_starter = ResizeJobsStarter(config, logger, mongo_client, snapshot, args.autoapprove)

    while True:
        for gs_id_prefix in groupset_id_prefixes:
            jobs_starter.run(gs_id_prefix)

        time.sleep(args.loop_delay_sec)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--groupset-id-prefixes', help="Comma separated list of groupset id prefixes", required=True
    )
    parser.add_argument(
        '--autoapprove',
        help="Enable auto approve of created jobs",
        action='store_true',
        default=False,
    )
    parser.add_argument(
        '--debug-logging', help="Enable debug logging", action='store_true',
    )
    subparsers = parser.add_subparsers()

    single_run_parser = subparsers.add_parser('single_run', help="Do single run and exit")
    single_run_parser.set_defaults(func=single_run_cmd)

    loop_run_parser = subparsers.add_parser('loop_run', help="Run in a loop continuously")
    loop_run_parser.add_argument(
        '--loop-delay-sec', help="Delay between runs", type=int, default=DEFAULT_LOOP_DELAY,
    )
    loop_run_parser.set_defaults(func=loop_run_cmd)

    args = parser.parse_args()
    if hasattr(args, 'func'):
        args.func(args)
    else:
        parser.print_help()
