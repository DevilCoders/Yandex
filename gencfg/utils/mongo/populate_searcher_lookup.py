#!/skynet/python/bin/python

import os
import sys
import time
import pymongo
import logging
from argparse import ArgumentParser
import json
import copy

sys.path.append(os.path.abspath('.'))

import gencfg
import ujson
import core
import gaux
from core.svnapi import SvnRepository
from core.db import CURDB
from json_diff import diff, eq
import utils.api.searcherlookup_groups_instances

from mongo_params import ALL_HEARTBEAT_C_MONGODB
import common


_db = pymongo.MongoReplicaSetClient(
    ALL_HEARTBEAT_C_MONGODB.uri,
    connectTimeoutMS=5000,
    replicaSet=ALL_HEARTBEAT_C_MONGODB.replicaset,
    w='3',
    wtimeout=60000,
    read_preference=ALL_HEARTBEAT_C_MONGODB.read_preference,
)['topology_commits']

collection = _db['search_lookup']
tags = _db['tags']


def get_full_state(current_commit):
    cursor = tags.find({'commit': {'$lt': current_commit}}).sort('commit', -1)
    for rec in cursor:
        if 'fullstate' in rec:
            return rec
    return None


def iter_full_state(commit):
    cursor = collection.find({'commit': int(commit)})
    for rec in cursor:
        data = common.de_binary(rec['data'])
        yield str(rec['group']), data


class Parser(object):
    def __init__(self, root='./'):
        self.commit = common.get_current_commit(root)
        self.root = root
        # ======================================= RX-224 START ===================================================
        self._groups = {g.card.name: common.recurse_fix_json_types(g.card.as_dict()) for g in CURDB.groups.get_groups() if g.card.properties.created_from_portovm_group is None}
        for group in CURDB.groups.get_groups():
            if not group.has_portovm_guest_group():
                continue
            guest_card_name = '{}_GUEST'.format(group.card.name)
            guest_card = common.recurse_fix_json_types(group.guest_group_card_dict())
            self._groups[guest_card_name] = guest_card
        # ======================================= RX-224 FINISH ==================================================
        print self.commit

        # ==================================== RX-302 START ===============================================
        for group in CURDB.groups.get_groups():
            self._groups[group.card.name]['reqs']['volumes'] = common.recurse_fix_json_types([x.to_card_node().as_dict() for x in gaux.aux_volumes.volumes_as_objects(group)])
        # ==================================== RX-302 FINISH ==============================================

    @classmethod
    def fix_instances(cls, instances):
        fixed = {}
        for one in instances:
            one['hostname'] = one['hostname'].replace('.', '!')
            key = '{}:{}'.format(one['hostname'], one['port'])
            fixed[key] = one
        return fixed

    def parse_group_instances(self, group_name):
        util_params = dict(groups=group_name)
        group = utils.api.searcherlookup_groups_instances.jsmain(util_params)[group_name]
        return self.fix_instances(group['instances'])

    def parse_group_card(self, group_name):
        return self._groups[group_name]

    def wrap_group(self, group, data):
        rec = {
            'commit': int(self.commit),
            'group': group,
            'data': common.binary(data)
        }
        return rec

    def wrap_diff(self, group, data_diff):
        rec = {
            'commit': int(self.commit),
            'group': group,
            'data_diff': common.binary(data_diff)
        }
        return rec

    def wrap_dead(self, group):
        rec = {
            'commit': int(self.commit),
            'group': group,
            'dead': 1
        }
        return rec

    def iterate_groups(self):
        for group in sorted(self._groups):
            instances = self.parse_group_instances(group)
            card = self.parse_group_card(group)
            yield self.wrap_group(
                group,
                data={'instances': instances, 'card': card}
            )

    def iterate_diff(self, base_cursor):
        new_names = set(self._groups)
        old_names = set()
        for name, old_data in base_cursor:
            old_names.add(name)
            if name in new_names:
                new_instances = self.parse_group_instances(name)
                new_card = self.parse_group_card(name)
                new_data = {'instances': new_instances, 'card': new_card}
                if not eq(old_data, new_data):
                    data_diff = diff(old_data, new_data)
                    yield self.wrap_diff(name, data_diff=data_diff)
            else:
                yield self.wrap_dead(name)
        for name in new_names.difference(old_names):
            new_instances = self.parse_group_instances(name)
            new_card = self.parse_group_card(name)
            new_data = {'instances': new_instances, 'card': new_card}
            yield self.wrap_group(name, data=new_data)


class Manager(object):
    def __init__(self, root, full=False):
        self.parser = Parser(root)
        self.full = full

    @property
    def commit(self):
        return self.parser.commit

    def to_stdout(self):
        for rec in self.parser.iterate_groups():
            jsoned = copy.copy(rec)
            jsoned['data'] = common.de_binary(jsoned['data'])
            print ujson.dumps(jsoned, indent=4, sort_keys=True)

    def contains(self, commit=None):
        commit = commit or self.commit
        for _ in collection.find({'commit': int(commit)}, limit=1):
            return True
        return False

    def cleanup(self, commit=None):
        commit = commit or self.commit
        collection.remove({'commit': int(commit)})

    def insert(self, gen):
        try:
            collection.insert(gen)
        except pymongo.errors.InvalidOperation:
            print 'no diff'

    def to_mongo(self):
        if self.full:
            self.insert(self.parser.iterate_groups())
        else:
            base = get_full_state(self.commit)
            print 'base is', base['commit']
            full = iter_full_state(base['commit'])
            self.insert(self.parser.iterate_diff(full))
        print 'done'


def main(args):
    root = './'

    manager = Manager(root, args.full)
    if args.nomongo:
        manager.to_stdout()
    else:
        if manager.contains():
            logging.info('commit {} already in db, cleaning up'.format(manager.commit))
            manager.cleanup()
            logging.info('cleaned up')
        manager.to_mongo()


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('--nomongo', action='store_true')
    parser.add_argument('--full', action='store_true')
    return parser.parse_args()


if __name__ == '__main__':
    arguments = parse_args()
    for i in range(3):
        try:
            main(arguments)
            break
        except pymongo.errors.AutoReconnect as ex:
            logging.exception(ex)
            time.sleep(5)
    else:
        raise Exception('mongo is not available')
