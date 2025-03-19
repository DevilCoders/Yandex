#!/usr/bin/pymds
# -*- coding: utf-8 -*-

import argparse
import logging
import os
import socket
import json
import time
import sys
import pymongo
import imp
from collections import defaultdict

from mastermind.errors import MastermindNetworkError
import mastermind

from mds.admin.library.python.sa_scripts import utils
from mds.admin.library.python.sa_scripts import mm
from mds.admin.library.python.sa_scripts.utils import detect_environment, detect_federation

utils.exit_if_file_exists('/var/tmp/disable_mastermind_monrun')
logger = logging.getLogger('mastermind_monrun')
HOSTNAME = socket.gethostname()


def get_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title='monrun')

    couple_bad_append = subparsers.add_parser('couple_bad', help='bad replicas groupsets')
    couple_bad_append.set_defaults(func=couple_bad)

    couple_broken_append = subparsers.add_parser('couple_broken', help='broken couples')
    couple_broken_append.set_defaults(func=couple_broken)

    bad_groups_append = subparsers.add_parser('bad_groups', help='broken uncoupled groups')
    bad_groups_append.set_defaults(func=bad_groups)

    couple_lrc_state_bad_append = subparsers.add_parser(
        'couple_lrc_state_bad', help='bad lrc groupsets'
    )
    couple_lrc_state_bad_append.set_defaults(func=couple_lrc_state_bad)

    dc_space_append = subparsers.add_parser('dc_space', help='dc free space')
    dc_space_append.set_defaults(func=dc_space)

    couples_diff_append = subparsers.add_parser('couples_diff', help='Lost couples')
    couples_diff_append.set_defaults(func=couples_diff)

    bad_data_unavailable_append = subparsers.add_parser(
        'bad_data_unavailable', help='bad data unavailable'
    )
    bad_data_unavailable_append.add_argument('-o', '--old', action='store_true', help='Old logic')
    bad_data_unavailable_append.set_defaults(func=bad_data_unavailable)

    couple_lost_append = subparsers.add_parser('couple_lost', help='lost couples')
    couple_lost_append.add_argument('-i', '--ignore-couples', default='')
    couple_lost_append.set_defaults(func=couple_lost)

    ns_settings_correct_append = subparsers.add_parser(
        'ns_settings_correct', help='check min-units/add-units'
    )
    ns_settings_correct_append.set_defaults(func=ns_settings_correct)

    man_x2_append = subparsers.add_parser('man_x2', help='x2 groups man')
    man_x2_append.set_defaults(func=man_x2)

    man_first_group_append = subparsers.add_parser('man_first_group', help='first groups man')
    man_first_group_append.set_defaults(func=man_first_group)

    mandatory_dcs_append = subparsers.add_parser('mandatory_dcs', help='')
    mandatory_dcs_append.set_defaults(func=mandatory_dcs)

    couples_on_1g_host_append = subparsers.add_parser(
        'couples_on_1g_host', help='couples with 2 groups on 1G hosts'
    )
    couples_on_1g_host_append.set_defaults(func=couples_on_1g_host)

    parser.add_argument('-l', '--log-file', default='/var/log/mastermind/monrun.log')
    parser.add_argument('-d', '--debug', action='store_true', help='print debug info')
    parser.add_argument('-r', '--run', action='store_true', help='check execution')
    args = parser.parse_args()
    return args


def setup_logging(log_file):
    _format = logging.Formatter(
        "[%(asctime)s] [%(name)s] [%(process)d] [%(funcName)s] [%(processName)s] %(levelname)s: %(message)s"
    )
    _handler = logging.FileHandler(log_file)
    _handler.setFormatter(_format)
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger().addHandler(_handler)


def fetch_snapshot_grpc():
    import mds.mastermind.snapshot.bindings.python.mastermind_snapshot as mastermind_snapshot

    mm_config = CouplesDiff.get_config()
    storage_state_source = mm_config.get('storage_state_source', {})
    collector_addresses = storage_state_source.get('addresses')
    collector_timeout = storage_state_source.get('timeout', 30)

    collector_client = mastermind.CollectorClient(addresses=collector_addresses)
    fb_snapshot = collector_client.get_snapshot(timeout=collector_timeout)
    snapshot = mastermind_snapshot.get_snapshot(fb_snapshot)

    return snapshot


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def couple_bad(args):
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=20)
    bad_groupsets = list(mm_client.groupsets.filter(state='bad', type='replicas'))

    res = []
    for groupset in bad_groupsets:
        add_to_res = True
        for g in groupset.groups:
            active_job = g._data.get('active_job')
            if g.status != 'COUPLED':
                # FIX ME
                if active_job and active_job['type'] == 'add_groupset_to_couple_job':
                    add_to_res = False
                if args.debug and add_to_res:
                    try:
                        print("{} {}".format(g.node_backends[0].hostname, groupset.id))
                    except Exception:
                        print("{} {}".format(str(g.history.nodes[-1]).split()[2].split(':')[0], groupset.id))
        if add_to_res:
            res.append(groupset)

    if len(res):
        logger.info("Bad groupsets: {}".format([x.id for x in res]))
        if not args.debug:
            print("2;{} couples bad!".format(len(res)))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def couple_lost(args):
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=20)
    lost_couples = list(mm_client.couples.filter(state='lost'))

    if args.ignore_couples:
        ignore = set(list([int(x) for x in args.ignore_couples.split(',')]))
    else:
        ignore = set()

    res = set()
    for couple in lost_couples:
        res.add(couple.id)
        if args.debug:
            print(couple)

    if not args.debug:
        msg = ''
        code = 0
        if len(res - ignore):
            code = 2
            if len(res - ignore) > 10:
                msg += "lost: {}!".format(len(res - ignore))
            else:
                msg += "lost: {}!".format(list(res - ignore))

        if len(ignore - res):
            code = 2
            msg += "not lost: {}".format(list(ignore - res))

        if not msg:
            msg = "OK"
        print("{};{}".format(code, msg))


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def couple_broken(args):
    STATE_FILE = '/var/tmp/mm_broken_check.state'
    try:
        with open(STATE_FILE, 'r') as sf:
            state = json.load(sf)
    except Exception:
        state = {}

    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=20)

    params = {
        'state': 'broken',
        'ignore_namespaces': ['storage_cache'],
        'fields': ['id', 'create_ts', 'groupsets'],
    }
    broken_couples = mm_client.couples.filter(**params)

    res = []
    for couple in broken_couples:
        couple_id = str(couple.id)
        create_ts = couple.create_ts

        # TODO: Remove after MDS-17298
        try:
            active_job = (
                couple.groupsets.get('lrc-8-2-2-v2', '').groups[0]._data.get('active_job', {})
            )
        except Exception:
            active_job = {}
        if active_job.get('status', '') in ['executing'] and active_job.get('type', '') in [
            'add_groupset_to_couple_job'
        ]:
            logger.info("Skip couple {} in job {}".format(couple.id, active_job))
            continue

        is_broken = False
        # mb new couple
        if create_ts is None:
            flap = state.get(couple_id, 0)
            if flap == 0:
                state[couple_id] = 1
            elif flap > 3:
                is_broken = True
            else:
                state[couple_id] += 1
        else:
            if int(time.time()) - create_ts > 3600:
                is_broken = True

        if is_broken:
            if args.debug:
                print(couple_id)
            res.append(couple_id)

    if len(res):
        logger.info("Broken couples: {}".format(res))
        if not args.debug:
            print("2;{} couples broken!".format(len(res)))
    else:
        print("0;OK")

    if len(broken_couples) == 0:
        state = {}
    with open(STATE_FILE, 'w') as sf:
        json.dump(state, sf)


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def bad_groups(args):

    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME])

    params = {
        'type': 'uncoupled',
        'state': 'broken',
        'fields': ['id'],
    }
    broken_groups = mm_client.groups.filter(**params)

    res = []
    for group in broken_groups:
        res.append(group)
        if args.debug:
            print(group)

    bad_groups = list(mm_client.groups.filter(state='bad', type='data'))
    for group in bad_groups:
        if group.groupset:
            continue
        if len(group.node_backends) != 1:
            continue
        active_job = group._data.get('active_job')
        if active_job:
            continue

        res.append(group)
        if args.debug:
            print(group.id)

    if len(res):
        logger.info("Bad/broken groups: {}".format([x.id for x in res]))
        if not args.debug:
            print("2;{} groups bad/broken!".format(len(res)))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def couple_lrc_state_bad(args):
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=20)
    bad_groupsets = list(
        mm_client.groupsets.filter(
            state='bad', type='lrc', fields=['id', 'status', 'status_text', 'groups']
        )
    )

    res = []
    for groupset in bad_groupsets:
        add_to_res = True
        for g in groupset.groups:
            active_job = g._data.get('active_job')
            if g.status != 'COUPLED':
                # FIX ME
                if active_job and active_job['type'] == 'add_groupset_to_couple_job':
                    add_to_res = True
                if args.debug and add_to_res:
                    try:
                        print("{} {}".format(g.node_backends[0].hostname, groupset.id))
                    except Exception:
                        print("{} {}".format(str(g.history.nodes[-1]).split()[2].split(':')[0], groupset.id))
        res.append(groupset)

    if len(res):
        logger.info("Bad lrc groupsets: {}".format([x.id for x in res]))
        if not args.debug:
            print("2;{} LRC couple bad!!".format(len(res)))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def dc_space(args):
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=60)

    stats = mm_client.request('get_flow_stats', None)

    s = ""
    for dc, stat in stats['dc'].iteritems():
        if stat['total_space'] > 0:
            percentage = round(
                (
                    float(stat.get('uncoupled_space', 0))
                    + float(stat.get('future_backends_space', 0))
                )
                / stat.get('total_space', 0)
                * 100,
                2,
            )
            if percentage <= 5:
                s += "{0}: {1}%; ".format(dc, percentage)

    if s:
        print("2;Space left in {}".format(s))
    else:
        print("0;OK")


class CouplesDiff:
    def __init__(self):
        self.config = self.get_config()
        self.mc = self.get_mongo_client()
        self.mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=60)

    @staticmethod
    def get_config():
        CONFIG_PATH = '/etc/elliptics/mastermind.conf'
        try:
            with open(CONFIG_PATH, 'r') as config_file:
                return json.load(config_file)
        except Exception as e:
            print("1;Failed to load config file {}: {}".format(CONFIG_PATH, e))
            sys.exit(1)

    def get_mongo_client(self):
        if not self.config.get('metadata', {}).get('url', ''):
            print("1;Can't connect to MongoDB")
            sys.exit(1)
        return pymongo.MongoReplicaSetClient(self.config['metadata']['url'])

    def update_couples(self, file_old_couples, now_couples):
        with open('{}.tmp'.format(file_old_couples), 'w') as tmp_file:
            tmp_file.write('\n'.join(now_couples))

        os.rename(tmp_file.name, file_old_couples)
        logger.info('Updated data {} with {} couples'.format(file_old_couples, len(now_couples)))

    @mm.mm_retry(tries=5, delay=1, backoff=1.5)
    def get_now_couples(self):
        params = {
            'fields': ['couple_id'],
            'ignore_namespaces': ['storage_cache'],
        }

        couple_data_list = self.mm_client.request('get_couples_list', [params])

        now_couples = set(str(data['couple_id']) for data in couple_data_list)

        return now_couples

    @mm.mm_retry(tries=5, delay=1, backoff=1.5)
    def get_deleted_couples(self, couples):
        params = {
            "types": ['discard_groupset_job'],
            "tags": {"couple_ids": [int(c) for c in couples]},
            "create_ts": {'min_create_ts': int(time.time()) - 86400},
            "statuses": ['completed', 'executing'],
        }
        res = self.mm_client.request('get_job_list', [params])

        jobs = res.get('jobs', [])
        deleted_couples = set()
        for j in jobs:
            for c in j['couples_being_broken']:
                deleted_couples.add(str(c))

        logger.debug('Deleted couples found ({})'.format(list(deleted_couples)))
        return deleted_couples

    def get_newly_created_couples(self, couples):
        couples_collection = self.mc[self.config['metadata']['couples']['db']]['couples']

        expected_snapshot_update_period = self.config.get('storage', {}).get(
            'expected_snapshot_update_period', 3600
        )
        recently_created_couple_ts = time.time() - expected_snapshot_update_period

        newly_created_couples = set()
        for couples_info in couples_collection.find(
            {
                'id': {'$in': [int(c) for c in couples]},
                'create_ts': {'$gte': recently_created_couple_ts},
            }
        ):
            newly_created_couples.add(str(couples_info['id']))

        logger.debug('Newly created couples found ({})'.format(list(newly_created_couples)))
        return newly_created_couples

    def check_lost_couples(self, now_couples, read_file_old_couples):
        lost_couples = read_file_old_couples - now_couples

        deleted_couples = self.get_deleted_couples(lost_couples)
        non_deleted_couples = lost_couples - deleted_couples
        newly_created_couples = self.get_newly_created_couples(non_deleted_couples)

        really_lost_couples = non_deleted_couples - newly_created_couples
        return really_lost_couples, newly_created_couples, deleted_couples

    def couples_diff(self):
        now_couples = self.get_now_couples()

        file_old_couples = '/tmp/result_couple'

        if os.path.isfile(file_old_couples):
            if os.path.getsize(file_old_couples) > 0:
                with open(file_old_couples, 'r') as f:
                    read_file_old_couples = set()
                    for line in f:
                        read_file_old_couples.add(line.strip())

                lost_couples, newly_created_couples, deleted_couples = self.check_lost_couples(
                    now_couples, read_file_old_couples
                )
                now_couples |= newly_created_couples
                if lost_couples:
                    self.update_couples(file_old_couples, lost_couples | now_couples)

                    if len(lost_couples) > 10:
                        print('2; FATAL ERROR, LOST COUPLES {0}'.format(len(lost_couples)))
                    else:
                        print('2; FATAL ERROR, LOST COUPLE {0}'.format(list(lost_couples)))
                    logger.error('Different couples found ({}). Exit 1'.format(list(lost_couples)))
                    sys.exit(1)

                new_couples_amount = (
                    len(now_couples) + len(deleted_couples) - len(read_file_old_couples)
                )
                if new_couples_amount > 0:
                    logger.info(
                        'Detected append {} new couples; {} update {} -> {}'.format(
                            new_couples_amount,
                            file_old_couples,
                            len(read_file_old_couples),
                            len(now_couples),
                        )
                    )
                    self.update_couples(file_old_couples, now_couples)
                    print('0; OK')
                else:
                    assert new_couples_amount == 0
                    print('0; OK')
                    logger.info('Different not found. Couples {0}'.format(len(now_couples)))
                    if deleted_couples:
                        self.update_couples(file_old_couples, now_couples)
            else:
                self.update_couples(file_old_couples, now_couples)
                print('1; Created file')
                logger.info('Create not zero {0} file.'.format(file_old_couples))
        else:
            self.update_couples(file_old_couples, now_couples)
            print('1; Created file')
            logger.info('Create new {0} file.'.format(file_old_couples))


def couples_diff(args):
    cd = CouplesDiff()
    cd.couples_diff()


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def bad_data_unavailable(args):
    def print_debug(msg, groupsets):
        if args.debug:
            print("{}:".format(msg))
            for gs in groupsets:
                print(gs)

    def groupset_in_restore_or_move_job(gs):
        has_job = False
        for x in gs.groups:
            active_job = x._data.get('active_job', {})
            if active_job.get('type', '') in ['restore_group_job', 'move_job']:
                has_job = True

            if has_job:
                gs_has_good_group = False
                try:
                    if x.node_backends[0].status in ['RO', 'OK']:
                        gs_has_good_group = True
                except KeyError:
                    pass

                if gs_has_good_group:
                    return True

        return False

    def groupset_in_restart_job(gs):
        has_job = False
        for x in gs.groups:
            active_job = x._data.get('active_job', {})
            if active_job.get('type', '') == 'host_restart_elliptics_job':
                has_job = True

            if has_job:
                gs_has_good_group = False
                try:
                    if x.node_backends[0].status == 'DOWNTIME' and not no_route_list_meta(x):
                        gs_has_good_group = True
                except KeyError:
                    pass

                if gs_has_good_group:
                    return True
        return False

    def no_route_list_meta(group):
        try:
            group.meta()
        except Exception as e:
            if 'No such device or address: -6' in e.message:
                return True

        return False

    level = 0
    errors = []
    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=60)

    gs_bad = list(
        mm_client.groupsets.filter(
            state='bad', fields=['id', 'status_text', 'status', 'type', 'groups', 'couple_ids']
        )
    )
    gs_bad.extend(
        list(
            mm_client.groupsets.filter(
                state='broken',
                fields=['id', 'status_text', 'status', 'type', 'groups', 'couple_ids'],
            )
        )
    )

    for status in ["BAD_DATA_UNAVAILABLE", "BAD_INDICES_UNAVAILABLE"]:
        lrc_bad = [gs for gs in gs_bad if gs.status == status and gs.type == 'lrc']
        if len(lrc_bad) > 0:
            logger.info("lrc {}: {}".format(status, [str(x.id) for x in lrc_bad]))
            level = 2
            msg = "{} lrc gs {} ({}, ...)".format(len(lrc_bad), status, lrc_bad[0].id.split(":")[0])
            errors.append(msg)
            print_debug(msg, lrc_bad)

    # TODO: fix replicas after MDS-8351
    if args.old:
        replicas_bad = [gs for gs in gs_bad if gs.type == 'replicas']
        gs_to_check = {}  # {couple_id: gs_id}
        for gs in replicas_bad:
            all_groups_is_bad = all([x.status not in ['COUPLED', 'RO'] for x in gs.groups])

            if not all_groups_is_bad:
                continue

            if not gs.couple_ids:
                # No user data, no problem with unavailability.
                continue

            if len(gs.couple_ids) > 1:
                raise RuntimeError('Impossible: replicas grouspset have multiple couples')

            if groupset_in_restore_or_move_job(gs):
                continue

            if groupset_in_restart_job(gs):
                continue

            all_groups_6_or_downtime = all(
                [x.status == 'DOWNTIME' or no_route_list_meta(x) for x in gs.groups]
            )
            if not all_groups_6_or_downtime:
                continue

            couple_id = gs.couple_ids[0]
            # Note: we store gs for couple, since later couple can have no replicas groupsets.
            gs_to_check[couple_id] = gs
    else:
        replicas_bad = [gs for gs in gs_bad if gs.type == 'replicas']
        gs_to_check = {}  # {couple_id: gs_id}
        for gs in replicas_bad:
            if gs.status != "BAD_DATA_UNAVAILABLE":
                continue

            couple_id = gs.couple_ids[0]
            # Note: we store gs for couple, since later couple can have no replicas groupsets.
            gs_to_check[couple_id] = gs

    replicas_data_unavailable = []
    couples = mm_client.couples.filter(
        ids=list(gs_to_check.keys()), fields=['id', 'groupsets', 'settings']
    )
    for c in couples:
        if gs_to_check[c.id].type not in c.read_preference:
            # Couple store data in lrc groupsets, so user data is not unavailable.
            # But replicas groupsets are discarded after some time. So all groups can be BAD and it's not a problem.
            # TODO: After MDS-8351 we can monitor only groupsets with status BAD_DATA_UNAVAILABLE.
            continue
        replicas_data_unavailable.append(gs_to_check[c.id])

    if len(replicas_data_unavailable) > 0:
        logger.info(
            "replicas BAD_DATA_UNAVAILABLE: {}".format(
                [str(x.id) for x in replicas_data_unavailable]
            )
        )
        level = 2
        msg = "{} gs BAD_DATA_UNAVAILABLE ({}, ...)".format(
            len(replicas_data_unavailable), replicas_data_unavailable[0].id
        )
        errors.append(msg)
        print_debug(msg, replicas_data_unavailable)

    if level > 0:
        error = "; ".join(errors)
        if not args.debug:
            print("{}; {}!".format(level, error))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def ns_settings_correct(args):
    ENV = detect_environment()

    if ENV == 'testing':
        print("0; Testing")
        sys.exit(0)

    # MDS-16301
    FED = detect_federation()
    if FED not in [1, '1']:
        print("0; Federation {}".format(FED))
        sys.exit(0)

    mm_config = CouplesDiff.get_config()
    add_units_default = mm_config['weight'].get('add_units', 2)
    min_units_default = mm_config['weight'].get('min_units', 1)

    namespaces_mandatory_dcs = mm_config.get("namespaces-mandatory-dcs", {}).keys()

    mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=60)
    flow_stats = mm_client.request('get_flow_stats', None)
    dict_dc = flow_stats['dc'].keys()

    BadNamespaces = {'warn': [], 'crit': []}
    BigNamespaces = ['disk']

    for Namespace in mm_client.namespaces.filter(deleted=False):
        Settings = Namespace.settings

        if 'static-couple' in Settings:
            continue

        # MDS-17220
        if Settings.get('federation', '') == 'ycloud':
            continue

        # Namespace not found in this federation
        if Namespace.id not in flow_stats['namespaces']:
            continue

        # MDS-18396
        if Namespace.id in namespaces_mandatory_dcs:
            continue

        good_units = (
            flow_stats.get('namespaces_only', {}).get(Namespace.id, {}).get('open_couples', 0)
        )

        dcs_open_couples = []
        for dc in dict_dc:
            if dc in flow_stats['namespaces'][Namespace.id]:
                dc_outages = flow_stats['namespaces'][Namespace.id][dc]['outages']
                dcs_open_couples.append(
                    dc_outages.get('open_couples', 0) + dc_outages.get('downtimed_couples', 0)
                )
        dc_good_units = min(dcs_open_couples)

        min_units = int(str(Settings.get('min-units', min_units_default)))
        add_units = int(str(Settings.get('add-units', add_units_default)))
        need_units = min_units + add_units

        if dc_good_units < min_units:
            BadNamespaces['crit'].append(
                "{} {}/({}+{})".format(Namespace.id, dc_good_units, min_units, add_units)
            )
        elif Namespace.id in BigNamespaces and dc_good_units < need_units:
            BadNamespaces['crit'].append(
                "{} {}/({}+{})".format(Namespace.id, dc_good_units, min_units, add_units)
            )
        elif good_units < need_units:
            BadNamespaces['warn'].append(
                "{} {}/({}+{})".format(Namespace.id, good_units, min_units, add_units)
            )

    if BadNamespaces['warn'] == [] and BadNamespaces['crit'] == []:
        print("0; Ok")
    else:
        out = ''
        if BadNamespaces['crit']:
            out += "2; disabling dc: "
            out += ", ".join(BadNamespaces['crit'])
        if BadNamespaces['warn']:
            if out != '':
                out += "; add-units errors: "
            else:
                out += "1; add-units errors: "
            out += ", ".join(BadNamespaces['warn'])

        print(out)


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def man_x2(args):
    snapshot = fetch_snapshot_grpc()

    ignore_namespaces = ['disk', 's3-sdc']
    logger.info("Ignore namespaces: {}".format(ignore_namespaces))

    res = []
    run = dict()
    for groupset in snapshot.groupsets:
        if groupset.type != 'replicas':
            continue

        if len(groupset.groups) > 2:
            continue

        add_to_res = False
        m_group = ''
        for g in groupset.groups:
            try:
                m_host = g.backend.node.host.hostname
                dc = g.backend.node.host.dc
            except AttributeError:
                continue

            if dc == 'man':
                add_to_res = True
                m_group = g

            ns = groupset.namespace

            if ns in ignore_namespaces:
                add_to_res = False

        if add_to_res:
            if args.debug and add_to_res:
                print("{} {} {}".format(groupset, ns, groupset.stat.effective_used_space))
            run[m_group] = m_host
            res.append(groupset)

    if args.run:
        for g, h in run.items():
            print(
                "mastermind job request create move --out-of-dc --mandatory-dcs sas --mandatory-dcs vla --mandatory-dcs vlx --hurry --id man_x2_{}_{} {}".format(
                    h, g, g
                )
            )
        sys.exit()

    if len(res):
        logger.info("x2 groups in man: {}".format(len(res)))
        if not args.debug:
            print("2;{} x2 groups in man!".format(len(res)))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def man_first_group(args):
    snapshot = fetch_snapshot_grpc()

    res = []
    run = dict()
    for groupset in snapshot.groupsets:
        add_to_res = False
        if groupset.type != 'replicas':
            continue

        if len(groupset.groups) < 3:
            continue

        group_0 = groupset.groups[0]

        try:
            host_0 = groupset.groups[0].backend.node.host.hostname
            dc = groupset.groups[0].backend.node.host.dc
        except AttributeError:
            continue

        ns = groupset.namespace

        if dc == 'man':
            add_to_res = True

        if add_to_res:
            if args.debug:
                print("{} {} {}".format(group_0, dc, ns))
            run[group_0] = host_0
            res.append(groupset)

    if args.run:
        for g, h in run.items():
            print(
                "mastermind job request create move --out-of-dc --mandatory-dcs sas --mandatory-dcs vla --mandatory-dcs vlx --hurry --id man_first_group_{}_{} {}".format(
                    h, g, g
                )
            )
        sys.exit()

    if len(res):
        logger.info("first groups in man: {}".format(len(res)))
        if not args.debug:
            print("2;{} first groups in man!".format(len(res)))
    else:
        print("0;OK")


@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def mandatory_dcs(args):
    snapshot = fetch_snapshot_grpc()

    mm_config = CouplesDiff.get_config()
    namespaces_mandatory_dcs = mm_config.get('namespaces-mandatory-dcs', {})
    res_dc = []
    res_federation = []
    for ns in snapshot.namespaces:
        if ns not in namespaces_mandatory_dcs:
            continue
        ns_federation = json.loads(ns.settings).get('federation', 'default')
        mandatory_dcs = set(namespaces_mandatory_dcs[ns])
        for groupset in ns.replicas_groupsets:
            dcs = []
            for g in groupset.groups:
                dc = ''
                try:
                    for backend in g.node_backends:
                        dc = backend.node.host.dc
                except Exception:
                    pass
                dcs.append(dc)
            dcs = set(dcs)

            if not mandatory_dcs.issubset(dcs):
                if args.debug:
                    print("{} {}".format(groupset, ns))
                res_dc.append(groupset)
            else:
                for m_dc in mandatory_dcs:
                    for g in groupset.groups:
                        dc = ''
                        federation = 'default'
                        try:
                            for backend in g.node_backends:
                                dc = backend.node.host.dc
                                federation = backend.node.host.federation
                        except Exception:
                            pass
                        if dc == m_dc and federation != ns_federation:
                            if args.debug:
                                print("{} {}".format(groupset, federation))
                            res_federation.append(groupset)

    if len(res_dc) or len(res_federation):
        msg = "{} mandatory dcs and {} federations failed".format(len(res_dc), len(res_federation))
        logger.info(msg)
        if not args.debug:
            print("2;{}".format(msg))
    else:
        print("0;OK")


# MDS-17366
@mm.mm_retry(tries=5, delay=1, backoff=1.5)
def couples_on_1g_host(args):
    ENV = detect_environment()

    if ENV == 'testing':
        print("0; Testing")
        sys.exit(0)

    snapshot = fetch_snapshot_grpc()

    res = []
    move_1g = dict()
    for gs in snapshot.groupsets.values():
        g1 = []
        if gs.type != 'replicas':
            continue

        if len(gs.groups) > 3:
            continue

        for group in gs.groups:
            try:
                if group.backend.node.stat.max_net_rate == 131072000:
                    g1.append(group.group_id)
                    if gs.namespace == 'docker-registry':
                        move_1g[group.group_id] = group.backend.node.host.hostname
            except Exception:
                continue
        if len(g1) >= 2:
            res.append(gs)
            move_1g[gs.groups[0]] = gs.groups[0].backend.node.host.hostname
            if args.debug:
                print(gs)

    if args.run:
        for g, h in move_1g.items():
            print(
                "mastermind job request create move --out-of-dc --mandatory-dcs sas --mandatory-dcs vla --mandatory-dcs vlx --hurry --id couples_on_1g_host_{}_{} {}".format(
                    h, g, g
                )
            )
        sys.exit()

    if len(res) == 0:
        print("0;Ok")
    else:
        if not args.debug:
            print("2;{} couples with 2 groups on 1G hosts".format(len(res)))


def main():
    args = get_args()
    setup_logging(args.log_file)
    logger.debug('Start {} with arguments: {}'.format(args.func.func_name, args))
    start = time.time()
    pid = os.getpid()
    try:
        args.func(args)
        logger.info("Finish {}, pid: {}".format(args.func.func_name, pid))
    except MastermindNetworkError as e:
        pid = os.getpid()
        logger.exception("Failed to run {}".format(args.func.func_name))
        print("0; Probably mastermind is stopped: {}".format(e))
    except Exception:
        logger.exception("Failed to run {}".format(args.func.func_name))
        print("2; Script failed, check: 'grep {} {}'".format(pid, args.log_file))
    finally:
        logger.debug(
            'Function {} was executed {} sec'.format(args.func.func_name, time.time() - start)
        )


if __name__ == '__main__':
    main()
