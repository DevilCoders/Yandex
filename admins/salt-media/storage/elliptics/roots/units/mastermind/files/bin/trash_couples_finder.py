#!/usr/bin/env pymds
"""
Script to find trash couples in lrc groupsets left by cancelled convert jobs.

It must be run at host with access to mastermind mongo database & collector.
By default it is assumed that script is executed on collector host.

Usage example:
    pymds trash_couples_finder.py list
"""
import time
import json
import argparse
from collections import defaultdict

import pymongo

import mastermind
import mds.mastermind.snapshot.bindings.python.mastermind_snapshot as mastermind_snapshot

CONFIG_PATH = '/etc/elliptics/mastermind.conf'
DEFAULT_COLLECTOR_ADDR = 'localhost:50057'  # by default run script at collector host


def get_config(path=CONFIG_PATH):
    with open(path, 'r') as config_file:
        config = json.load(config_file)
    return config


def get_mongo_client(config):
    if not config.get('metadata', {}).get('url', ''):
        raise ValueError('Mongo db url is not set')
    url = config['metadata']['url']
    return pymongo.mongo_replica_set_client.MongoReplicaSetClient(url)


def fetch_snapshot(addresses=DEFAULT_COLLECTOR_ADDR, timeout=None):
    collector_client = mastermind.CollectorClient(addresses=addresses)
    fb_snapshot = collector_client.get_snapshot(timeout=timeout)
    snapshot = mastermind_snapshot.get_snapshot(fb_snapshot)
    return snapshot


class ConvertJob(object):
    def __init__(self, groupset, couple_id, groups, ts):
        self.groupset = groupset
        self.gs = groupset

        self.groupset_id = self.gs.id
        self.gsid = self.gs.id

        self.couple_id = couple_id
        self.cid = couple_id

        self.groups = groups

        self.ts = ts


class TrashCouplesFinder(object):
    def __init__(
        self, config, snapshot, mongo_client, convert_job_min_age_sec=None,
        check_eventually_converted_couples=False, exclude_locked=False,
    ):
        self.snapshot = snapshot
        self.client = mongo_client

        db_name = config['metadata']['jobs']['db']
        self.jobs_collection = self.client[db_name].jobs

        self.convert_job_min_age = convert_job_min_age_sec
        self.check_eventually_converted_couples = check_eventually_converted_couples
        self.exclude_locked = exclude_locked

    def _get_job_gsid(self, job):
        """Get groupset_id from job mongo document"""
        return ':'.join(map(str, sorted(job['groups'])))

    def _get_job_ts(self, job):
        if job.get('finish_ts'):
            return job['finish_ts']
        elif job.get('start_ts'):
            return job['start_ts']
        else:
            return job['create_ts']

    def _get_convert_jobs(self, filters=None):
        """Get 'convert to lrc' jobs. Use `filters` to select jobs subset"""
        query = {
            'type': 'add_groupset_to_couple_job',
        }
        if filters is not None:
            query.update(**filters)
        projection = {
            'id': 1, 'groups': 1, 'couple': 1, 'status': 1,
            'create_ts': 1, 'start_ts': 1, 'finish_ts': 1,
        }
        return list(self.jobs_collection.find(query, projection))

    def _get_cancelled_convert_jobs(self, exclude_multiple_attempts=True):
        """Get converted jobs that was cancelled

        Args:
            exclude_multiple_attempts (bool): if True for every combination
                of (groupset_id, couple_id) return only 1 job
        """
        jobs = self._get_convert_jobs({'status': 'cancelled'})
        now_time = int(time.time())

        if self.convert_job_min_age is not None:
            # Exclude too young jobs
            jobs = [j for j in jobs if now_time - self._get_job_ts(j) >= self.convert_job_min_age]

        # sort jobs by timestamp to select latest job for each (groupset_id, couple_id) pair
        jobs.sort(key=lambda j: self._get_job_ts(j), reverse=True)

        uniq_ids = set()

        jobs_info = []
        for j in jobs:
            gsid = self._get_job_gsid(j)
            cid = j['couple']
            ts = self._get_job_ts(j)

            if (gsid, cid) in uniq_ids:
                # there was more than one attempt to convert couple to lrc groupset
                if exclude_multiple_attempts:
                    continue

            gs = self.snapshot.lrc_groupsets.get(gsid)
            if gs is None:
                # Exclude removed groupsets
                continue
            jobs_info.append(ConvertJob(gs, cid, j['groups'], ts))
            uniq_ids.add((gsid, cid))

        return jobs_info

    def _get_groupset_included_couples_info(self, cancelled_jobs):
        """Get couples that included to lrc groupset in snapshot"""
        return set((job.gsid, job.cid) for job in cancelled_jobs if job.cid in job.gs.couple_ids)

    def _get_couples_without_other_groupsets(self, cancelled_jobs):
        """Get set of couple ids that doesn't have other groupsets"""
        bad_couple_ids = set()
        for job in cancelled_jobs:
            couple = self.snapshot.couples.get(int(job.couple_id))
            if couple is None:
                raise RuntimeError("Snapshot doesn't have couple %d" % job.couple_id)

            lrc_groupsets = [gs for gs in couple.groupsets if gs.type == 'lrc']
            replicas_groupsets = [gs for gs in couple.groupsets if gs.type == 'replicas']

            if len(replicas_groupsets) == 0 and len(lrc_groupsets) == 0:
                # couple doesn't exist elsewhere
                bad_couple_ids.add(job.couple_id)

            elif len(lrc_groupsets) == 1 and lrc_groupsets[0].id == job.groupset_id:
                # couple can be potentially deleted from all groupsets
                bad_couple_ids.add(job.couple_id)

        return bad_couple_ids

    def _get_eventually_converted_couples_info(self, cancelled_jobs):
        """Get couples that was successfully converted to lrc"""
        cancelled_groups_list = [j.groups for j in cancelled_jobs]
        related_convert_jobs = self._get_convert_jobs({'groups': {'$in': cancelled_groups_list}})

        related_convert_jobs_by_id = defaultdict(list)
        for j in related_convert_jobs:
            gsid = self._get_job_gsid(j)
            if j['status'] == 'completed':
                related_convert_jobs_by_id[gsid].append(j['couple'])

        return set(
            (job.groupset_id, job.couple_id) for job in cancelled_jobs
            if job.couple_id in related_convert_jobs_by_id[job.groupset_id]
        )

    def _get_removed_trash_couples_info(self, cancelled_jobs):
        """Get timestamps of remove couple jobs"""
        remove_jobs = list(self.jobs_collection.find(
            {'type': 'remove_lrc_groupset_job', "status": {"$ne": "cancelled"}},
            {
                'id': 1, 'type': 1, 'groups': 1, 'lrc_groupset': 1,
                'couple_to_remove': 1, 'tags.couple_ids': 1,
                'create_ts': 1, 'start_ts': 1, 'finish_ts': 1
            }
        ))

        cancelled_ids = set((job.groupset_id, job.couple_id) for job in cancelled_jobs)

        remove_jobs_by_id = defaultdict(list)
        for job in remove_jobs:
            groupset_id = self._get_job_gsid(job)
            couple_ids = job['tags']['couple_ids']

            for couple_id in couple_ids:
                id_ = (groupset_id, couple_id)

                if id_ not in cancelled_ids:
                    # we are interested only in trash couples left by cancelled converts
                    continue
                remove_jobs_by_id[id_].append(job)

        remove_ts_by_id = {
            id_: max(self._get_job_ts(j) for j in jobs)
            for id_, jobs in remove_jobs_by_id.items()
        }
        return remove_ts_by_id

    def _get_locked_groupset_ids(self, cancelled_jobs):
        """Get set with groupset ids locked by other jobs"""
        lrc_job_types = [
            'add_groupset_to_couple_job',
            'defrag_lrc_groupset_v2_job',
            'make_lrc_groups_job',
            'make_lrc_reserved_groups_job',
            'move_lrc_groupset_job',
            'recover_lrc_groupset_job',
            'remove_lrc_groupset_job',
            'restore_lrc_group_job',
            'restore_uncoupled_lrc_group_job',
        ]
        jobs = list(self.jobs_collection.find(
            {
                'type': {'$in': lrc_job_types},
                'status': {'$in': ['new', 'executing', 'pending', 'broken']}
            },
            {'tags.groupset_ids': 1},
        ))

        groupset_ids = set(id_ for j in jobs for id_ in j['tags']['groupset_ids'])
        locked_groupset_ids = set(
            job.groupset_id for job in cancelled_jobs
            if job.groupset_id in groupset_ids
        )
        return locked_groupset_ids

    def _get_locked_couple_ids(self, cancelled_jobs):
        """Get set with couple ids locked by other jobs"""

        # TODO: search locked couples in all possible jobs
        lrc_job_types = [
            'add_groupset_to_couple_job',
            'defrag_lrc_groupset_v2_job',
            'make_lrc_groups_job',
            'make_lrc_reserved_groups_job',
            'move_lrc_groupset_job',
            'recover_lrc_groupset_job',
            'remove_lrc_groupset_job',
            'restore_lrc_group_job',
            'restore_uncoupled_lrc_group_job',
        ]
        jobs = list(self.jobs_collection.find(
            {
                'type': {'$in': lrc_job_types},
                'status': {'$in': ['new', 'executing', 'pending', 'broken']}
            },
            {'tags.couple_ids': 1},
        ))

        couple_ids = set(id_ for j in jobs for id_ in j['tags'].get('couple_ids', []))
        locked_couple_ids = set(
            job.couple_id for job in cancelled_jobs
            if job.groupset_id in couple_ids
        )
        return locked_couple_ids

    def _get_trash_couples(self):
        """Get trash couples left by cancelled lrc conversions

        Returns:
            (list): list with trash couples info, ConvertJob
        """
        cancelled_jobs = self._get_cancelled_convert_jobs(exclude_multiple_attempts=True)
        homeless_couple_ids = self._get_couples_without_other_groupsets(cancelled_jobs)
        groupset_included_couples = self._get_groupset_included_couples_info(cancelled_jobs)

        if self.check_eventually_converted_couples:
            # Successfully converted couples must be included to groupset in snapshot,
            # so this check is redundant
            eventually_converted_couples = self._get_eventually_converted_couples_info(cancelled_jobs)
            potentially_alive_couples = groupset_included_couples | eventually_converted_couples
        else:
            potentially_alive_couples = groupset_included_couples

        remove_ts_by_id = self._get_removed_trash_couples_info(cancelled_jobs)

        trash_couples = []
        for job in cancelled_jobs:
            id_ = (job.groupset_id, job.couple_id)
            if id_ in potentially_alive_couples:
                # exclude potentially alive couples:
                continue
            elif job.couple_id in homeless_couple_ids:
                # potentially couple doesn't have any other groupsets except bad one
                continue
            elif remove_ts_by_id.get(id_, 0) > job.ts:
                # exclude already removed trash couples
                continue

            trash_couples.append(job)

        if self.exclude_locked:
            locked_groupset_ids = self._get_locked_groupset_ids(cancelled_jobs)
            locked_couple_ids = self._get_locked_couple_ids(cancelled_jobs)
            trash_couples = [
                j for j in trash_couples
                if j.groupset_id not in locked_groupset_ids
                   and j.couple_id not in locked_couple_ids
            ]

        return trash_couples

    def find(self):
        return self._get_trash_couples()


def get_inconsistent_keys_count(gs):
    """Get count of inconsistent keys

    Args:
        gs: lrc groupset from snapshot
    """
    res = 0
    for trio in [(1,3,9), (2,4,10), (5,7,11), (6,8,12)]:
        group_counts = [gs.groups[i-1].stat.files_alive for i in trio]
        max_cnt = max(group_counts)
        for i, cnt in zip(trio, group_counts):
            if cnt != max_cnt:
                res += max_cnt - cnt
    return res


def list_command(args):
    config = get_config()
    snapshot = fetch_snapshot(addresses=args.collector_addr)
    client = get_mongo_client(config)

    finder = TrashCouplesFinder(
        config,
        snapshot,
        client,
        convert_job_min_age_sec=args.convert_job_min_age,
        check_eventually_converted_couples=args.check_eventually_converted_couples,
        exclude_locked=args.exclude_locked
    )
    trash_couples = finder.find()
    trash_couples.sort(key=lambda job: get_inconsistent_keys_count(job.groupset), reverse=True)
    show_couples = trash_couples[:args.limit] if args.limit is not None else trash_couples

    print('Found %d garbage couples' % len(trash_couples))
    print('=============================================================')
    print('===  groupset_id    couple_id    inconsistent_keys_count  ===')
    print('=============================================================')
    for job in show_couples:
        print('%s    %s    %s' % (job.groupset_id, job.couple_id, get_inconsistent_keys_count(job.groupset)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    list_parser = subparsers.add_parser(
        'list',
        help='Show trash couples left by cancelled lrc conver jobs, '
             'sorted by number of inconsistent elliptics keys (highest first)'
    )
    list_parser.add_argument(
        '--collector-addr',
        default=DEFAULT_COLLECTOR_ADDR,
        help='address of collector in form "host:port"',
    )
    list_parser.add_argument('--limit', help='Show limited number of couples', default=None, type=int)
    list_parser.add_argument('--convert-job-min-age', help='Minimal age for cancelled convert jobs')
    list_parser.add_argument(
        '--exclude-locked', action='store_true',
        help='Exclude groupsets and couples locked by other jobs',
    )
    list_parser.add_argument(
        '--check-eventually-converted-couples', action='store_true',
        help='Check that there was no completed conversions for couple. This is very long check',
    )
    list_parser.set_defaults(func=list_command)

    args = parser.parse_args()
    if hasattr(args, 'func'):
        args.func(args)
    else:
        parser.print_help()
