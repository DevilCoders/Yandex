#!/skynet/python/bin/python

import os
import sys
import time
import logging
import datetime
import argparse
import multiprocessing

import copy_reg
import types

import pymongo

sys.path.append(os.path.abspath('.'))

import gencfg
import ujson
import core
import core.svnapi as svn
import core.db as db
import gaux
import gaux.aux_hbf
import gaux.aux_portovm as portovm
import gaux.aux_abc
import gaux.aux_staff
import mongo_params

import utils.api.searcherlookup_groups_instances
import common

import base64
import zlib
import json

def _pickle_method(m):
    if m.im_self is None:
        return getattr, (m.im_class, m.im_func.func_name)
    else:
        return getattr, (m.im_self, m.im_func.func_name)

copy_reg.pickle(types.MethodType, _pickle_method)


_db = pymongo.MongoReplicaSetClient(
    mongo_params.ALL_HEARTBEAT_C_MONGODB.uri,
    connectTimeoutMS=5000,
    replicaSet=mongo_params.ALL_HEARTBEAT_C_MONGODB.replicaset,
    w='3',
    wtimeout=60000,
    read_preference=mongo_params.ALL_HEARTBEAT_C_MONGODB.read_preference
)['topology_commits']


def get_gencfg_trunk(v2=False):
    params = mongo_params.ALL_HEARTBEAT_D_MONGODB if v2 else mongo_params.ALL_HEARTBEAT_C_MONGODB
    return pymongo.MongoReplicaSetClient(
        params.uri,
        connectTimeoutMS=5000,
        replicaSet=params.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=params.read_preference,
    )['topology_commits']['gencfg_trunk']


def get_gencfg_groups(v2=False):
    params = mongo_params.ALL_HEARTBEAT_A_MONGODB if v2 else mongo_params.ALL_HEARTBEAT_C_MONGODB
    return pymongo.MongoReplicaSetClient(
        params.uri,
        connectTimeoutMS=5000,
        replicaSet=params.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=params.read_preference,
    )['topology_commits']['groups']


gencfg_constants = _db['gencfg_constants']


def _load_all_groups():
    groups = [
        g.card.name
        for g in db.CURDB.groups.get_groups()
        if g.card.properties.created_from_portovm_group is None
    ]
    groups += [
        '{}_GUEST'.format(g.card.name)
        for g in db.CURDB.groups.get_groups()
        if g.has_portovm_guest_group()
    ]
    return sorted(groups)


def _load_itypes():
    return [itype.name for itype in db.CURDB.itypes.get_itypes()]


def _load_ctypes():
    return [ctype.name for ctype in db.CURDB.ctypes.get_ctypes()]


def _load_metaprjs():
    return db.CURDB.constants.METAPRJS.keys()


def _load_hbf_macroses():
    return [macro.to_json() for macro in db.CURDB.hbfmacroses.get_hbf_macroses()]


def _load_hbf_ranges():
    hbf_ranges = db.CURDB.hbfranges.to_json()
    for hbf_range in hbf_ranges.values():
        hbf_range['resolved_acl'] = gaux.aux_staff.unwrap_dpts(hbf_range['acl'])
    return hbf_ranges


def _load_dispenser_projects():
    leaf_project_keys = db.CURDB.dispenser.get_leaf_projects()

    dispenser_projects = {}
    for project_key in leaf_project_keys:
        dispenser_projects[project_key] = {
            'name': project_key,
            'resolved_acl': list(db.CURDB.dispenser.get_project_acl(project_key))
        }
    return dispenser_projects


def _load_staff_users():
    return db.CURDB.users.available_staff_users()


def _load_staff_groups():
    return db.CURDB.staffgroups.available_staff_groups()


def _load_abc_services():
    return db.CURDB.abcgroups.available_abc_services(disable_deprecated=True)


class DbParser(object):
    def __init__(self, root, options):
        logging.info('loading db')

        self._store_parsed = options.dry_run
        self._groups = _load_all_groups()  # load all groups anyway to fill caches
        if options.groups:
            self._groups = [options.groups]

        self.commit = common.get_current_commit(root)
        self.created = common.get_current_commit_creation_date(root)
        self._itypes = _load_itypes()
        self._ctypes = _load_ctypes()

        logging.info('processing commit %s', self.commit)

    def iterate_groups(self):
        pool = multiprocessing.Pool(16)

        for result in pool.imap_unordered(self._iterate_group, self._groups, chunksize=50):
            yield result

    def _iterate_group(self, group_name):
        parsed = self._parse_group(group_name)

        result = dict(
            group_name=group_name,
            wrapped_trunk_v1=self._wrap_gencfg_trunk(group_name, parsed, dump_fn=common.binary),
            wrapped_trunk_v2=self._wrap_gencfg_trunk(group_name, parsed, dump_fn=common.binary2),
            wrapped_groups=self._wrap_gencfg_groups(group_name),
        )
        if self._store_parsed:
            result['parsed'] = parsed

        return result

    def _parse_group(self, group_name):
        util_params = dict(groups=group_name)
        group = utils.api.searcherlookup_groups_instances.jsmain(util_params)[group_name]
        return self.fix_group(group)

    @classmethod
    def fix_group(cls, group):
        fixed = {}
        for instance in group['instances']:
            key = '{}:{}'.format(instance['hostname'], instance['port'])
            fixed[key] = instance
        return fixed

    def _wrap_gencfg_trunk(self, group, instances, dump_fn):
        card, hosts = _get_card_and_hosts(group)
        rec = {
            'commit': int(self.commit),
            'group': group,
            'instances': dump_fn(instances),
            'master': card['master'],
            'owners': card['owners'],
            'hosts': hosts,
            'card': card,
        }
        return rec

    def _wrap_gencfg_groups(self, group_name):
        if not group_name.endswith('_GUEST'):
            group = db.CURDB.groups.get_group(group_name)
            instances = list(group.get_kinda_busy_instances())
            instance_params = {(inst.host.name, inst.port): {'cpu_guarantee': _cpu(inst)} for inst in instances}
            cpu = long(sum(x.power for x in instances)) if not group.card.master else 0L
            if group.parent.db.version <= "2.2.21":
                mem = long(group.card.reqs.instances.memory.value) * len(instances)
            else:
                mem = long(group.card.reqs.instances.memory_guarantee.value) * len(instances)

            master = group.card.master.card.name if group.card.master else None

            rec = {
                'instances': common.binary(sorted(instance_params)),
                'instances_params': common.binary(instance_params),
                'master': master,
                'owners': group.card.owners,
                'cpu': cpu,
                'mem': mem,
                'count': len(instances),
            }
        else:
            group = db.CURDB.groups.get_group(group_name.rpartition('_')[0])
            guest_instances = [gaux.aux_portovm.guest_instance(x, db.CURDB) for x in group.get_kinda_busy_instances()]
            guest_instance_params = {(inst.host.name, inst.port): {'cpu_guarantee': _cpu(inst)} for inst in guest_instances}
            guest_cpu = long(sum(x.power for x in guest_instances))
            guest_mem = long(group.card.reqs.instances.memory_guarantee.value) * len(guest_instances)

            rec = {
                'instances': common.binary(sorted(guest_instance_params)),
                'instances_params': common.binary(guest_instance_params),
                'master': None,
                'owners': group.card.owners + group.card.guest.owners,
                'cpu': guest_cpu,
                'mem': guest_mem,
                'count': len(guest_instances),
            }

        rec.update({
            'commit': str(self.commit),
            'group': group_name,
            'time': datetime.datetime.utcnow(),
            'created': self.created,
        })
        return rec


def _cpu(instance):
    return int((100.0 * instance.power) / instance.host.power)


def _get_card_and_hosts(group):
    if group == 'ALL_SEARCH_VM':
        card, hosts = _get_card_and_hosts_all_search_vm(group)
        if len(hosts) == 0:
            raise Exception('Group ALL_SEARCH_VM is empty (MINILSR-224)')
    elif group == 'ALL_SAMOGON_RUNTIME_VM':
        card, hosts = _get_card_and_hosts_all_samogon_vm(group)
    elif group.endswith('_GUEST'):
        card, hosts = _get_card_and_hosts_guest(group)
    else:
        card, hosts = _get_card_and_hosts_default(group)

    return card, hosts


def _get_card_and_hosts_all_search_vm(group):
    # GENCFG-1654 RX-224

    igroup = db.CURDB.groups.get_group(group)
    card = _get_card_as_dict(igroup)
    hosts = []
    portovm.guest_instance.cached_data.clear()  # WORKAROUND TODO fix it
    for lgroup in db.CURDB.groups.get_groups():
        if lgroup.card.properties.nonsearch:
            continue
        if lgroup.card.on_update_trigger is not None:
            continue
        if lgroup.card.tags.itype not in ('psi', 'portovm'):
            continue
        hosts.extend([portovm.guest_instance(x, db=db.CURDB).host.name for x in lgroup.get_kinda_busy_instances()])
    hosts = sorted(set(hosts))

    return card, hosts


def _get_card_and_hosts_all_samogon_vm(group):
    # GENCFG-1654 RX-224

    igroup = db.CURDB.groups.get_group(group)
    card = _get_card_as_dict(igroup)
    hosts = []
    for lgroup in db.CURDB.groups.get_groups():
        if lgroup.card.properties.nonsearch:
            continue
        if lgroup.card.on_update_trigger is not None:
            continue
        if 'samogon' not in lgroup.card.tags.prj:
            continue
        hosts.extend([gaux.aux_hbf.generate_mtn_hostname(x, lgroup, '') for x in lgroup.get_kinda_busy_instances()])
    hosts = sorted(set(hosts))

    return card, hosts


def _get_card_and_hosts_guest(group):
    # GENCFG-1654 RX-224

    igroup = db.CURDB.groups.get_group(group.rpartition('_')[0])
    hosts = [portovm.guest_instance(x, db.CURDB).host.name for x in igroup.get_kinda_busy_instances()]
    card = _get_card_as_dict(igroup, is_guest=True)
    return card, hosts


def _get_card_and_hosts_default(group):
    igroup = db.CURDB.groups.get_group(group)
    hosts = [host.name for host in igroup.getHosts()]
    card = _get_card_as_dict(igroup)

    return card, hosts


def _get_card_as_dict(igroup, is_guest=False):
    if not is_guest:
        card = igroup.card.as_dict()
    else:
        card = igroup.guest_group_card_dict()

    # RX-302
    volumes_objects = gaux.aux_volumes.volumes_as_objects(igroup)
    card['reqs']['volumes'] = [x.to_card_node().as_dict() for x in volumes_objects]

    # RX-447
    card['owners_abc_roles'] = gaux.aux_abc.abc_roles_to_nanny_json(igroup)

    # RX-511
    card['resolved_owners'] = gaux.aux_staff.unwrap_dpts(igroup.card.owners)

    # RX-493
    card['resources'] = dict(ninstances=len(igroup.get_kinda_busy_instances()))

    return common.recurse_fix_json_types(card)


class MongoCache(object):
    def __init__(self, collection, readonly, upsert=False):
        self.collection = collection
        self.readonly = readonly
        self.bulk_op = None
        self.upsert = upsert
        self.count = 0

    def contains_commit(self, commit):
        for _ in self.collection.find({'commit': commit}, limit=1):
            return True
        return False

    def write(self, find_doc, replace_doc):
        if self.readonly:
            return

        if not self.bulk_op:
            self.bulk_op = self.collection.initialize_unordered_bulk_op()

        if self.upsert:
            self.bulk_op.find(find_doc).upsert().update({'$set': replace_doc})
        else:
            self.bulk_op.insert(replace_doc)

        self.count += 1

        if self.count >= 200:
            self.commit()
            self.count = 0

    def commit(self):
        if self.readonly:
            logging.info('storing data to %s', self.collection)
        else:
            if self.bulk_op:
                start = time.time()
                try:
                    self.bulk_op.execute()
                except pymongo.errors.BulkWriteError as bwe:
                    import pprint
                    pprint.pprint(bwe.details)
                    raise

                self.bulk_op = None
                logging.info('bulk write to %s took [%.2f]', self.collection, time.time() - start)


def commit_released(commit):
    commit = str(commit)
    for _ in _db['commits'].find({'commit': commit, 'test_passed': True}):
        return True
    return False


def update_constants(commit):
    gencfg_constants.update({'type': 'itypes'}, {'$set': {'value': _load_itypes(), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'ctypes'}, {'$set': {'value': _load_ctypes(), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'metaprjs'}, {'$set': {'value': _load_metaprjs(), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'hbf_ranges'}, {'$set': {'value': _load_hbf_ranges(), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'dispenser_projects'}, {'$set': {'value': _load_dispenser_projects(), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'staff_users'}, {'$set': {'value': list(_load_staff_users()), 'commit': commit}}, upsert=True)
    gencfg_constants.update({'type': 'staff_groups'}, {'$set': {'value': list(_load_staff_groups()), 'commit': commit}}, upsert=True)

    gencfg_constants.update({'type': 'abc_services'}, {'$set': compress_for_mongo(_load_abc_services(), commit)}, upsert=True)
    gencfg_constants.update({'type': 'hbf_macroses'}, {'$set': compress_for_mongo(_load_hbf_macroses(), commit)}, upsert=True)


def compress_for_mongo(obj, commit):
    encoded_blob = base64.b64encode(zlib.compress(json.dumps(list(obj))))
    return {'value': encoded_blob, 'compression': 'zlib_b64', 'commit': commit}


def populate(options):
    db_parser = DbParser('./', options)
    if not options.dry_run:
        update_constants(int(db_parser.commit))

    gencfg_trunk_cache_v1 = MongoCache(collection=get_gencfg_trunk(v2=False), readonly=options.dry_run, upsert=True)
    gencfg_trunk_cache_v2 = MongoCache(collection=get_gencfg_trunk(v2=True), readonly=options.dry_run, upsert=True)

    gencfg_groups_cache_v2 = MongoCache(collection=get_gencfg_groups(v2=True), readonly=options.dry_run, upsert=True)

    for group_result in db_parser.iterate_groups():
        logging.info('processing %s', group_result['group_name'])

        if options.dry_run:
            if options.dump_dir:
                with open(os.path.join(options.dump_dir, group_result['group_name']), 'w') as f:
                    f.write(ujson.dumps(group_result['parsed'], sort_keys=True, indent=4))
            else:
                sys.stdout.write(ujson.dumps(group_result['parsed'], sort_keys=True, indent=4))

        gencfg_trunk_cache_v1.write({
            'group': group_result['wrapped_trunk_v1']['group'],
            'commit': group_result['wrapped_trunk_v1']['commit'],
        }, group_result['wrapped_trunk_v1'])

        gencfg_trunk_cache_v2.write({
            'group': group_result['wrapped_trunk_v2']['group'],
            'commit': group_result['wrapped_trunk_v2']['commit'],
        }, group_result['wrapped_trunk_v2'])

        gencfg_groups_cache_v2.write({
            'group': group_result['wrapped_groups']['group'],
            'commit': group_result['wrapped_groups']['commit'],
        }, group_result['wrapped_groups'])

    gencfg_trunk_cache_v1.commit()
    gencfg_trunk_cache_v2.commit()
    gencfg_groups_cache_v2.commit()


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dry-run', action='store_true', default=False)
    parser.add_argument('--dump-dir', type=str, default=None, help='Optional. Dump data to specified dir in <dry_run> mode')
    parser.add_argument('--groups', type=str, default=None, help='Optional. Dry run only specified groups')
    return parser.parse_args()


def main():
    logging.basicConfig(format='[%(asctime)s] %(message)s', level=logging.DEBUG)
    options = parse_args()
    for attempt in range(3):
        try:
            logging.info('starting dry run' if options.dry_run else 'populating mongo cache')
            populate(options)
            break
        except (pymongo.errors.AutoReconnect, pymongo.errors.BulkWriteError) as ex:
            logging.exception(ex)
            time.sleep(5)
    else:
        raise Exception('mongo is not available')


if __name__ == '__main__':
    main()

