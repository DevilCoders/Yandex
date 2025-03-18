#!/skynet/python/bin/python

import os
import sys
import gaux.aux_staff

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from collections import defaultdict, OrderedDict
from argparse import ArgumentParser
import copy

import gencfg
import core.argparse.types as argparse_types
import utils.api.searcherlookup_groups_instances

from diffbuilder_types import mkindent, get_field_diff, NoneSomething, EDiffTypes, TDiffEntry, TBasicValueType, \
    EBasicTypes


def convert_power(v):
    oldpower = v.oldvalue
    newpower = v.newvalue

    if oldpower > 0.:
        percents = '%+d%%' % ((newpower - oldpower) * 100 / oldpower)
    else:
        percents = '+inf%'

    return '%+d (%s)' % (newpower - oldpower, percents)


class TGroupDiffEntry(TDiffEntry):
    PRETTY_PRINT_KEYS = OrderedDict([
        (('card', 'power'), ('Changed power', convert_power)),
        (('card', 'owners'), ('Owners', lambda x: x)),
        (('card', 'intlookups'), ('Intlookups', lambda x: x)),
        (('card', 'ctype'), ('Tag a_ctype', lambda x: x)),
        (('card', 'itype'), ('Tag a_itype', lambda x: x)),
        (('card', 'prj'), ('Tag a_prj', lambda x: x)),
        (('card', 'metaprj'), ('Tag a_metaprj', lambda x: x)),
        (('card', 'itag'), ('Tag itag', lambda x: x)),
        (('card', 'nonsearch'), ('Prop nonsearch', lambda x: x)),
        (('card', 'extra_disk_size'), ('Prop extra_disk_size', lambda x: x)),
        (('card', 'extra_disk_size_per_instance'), ('Prop extra_disk_size_per_instance', lambda x: x)),
        (('card', 'expires'), ('Expiration date', lambda x: x)),
        (('card', 'memory_per_instance'), ('Memory per instance', lambda x: x)),
    ])

    def __init__(self, diff_type, status, watchers, tasks, generated_diff, props=None):
        super(TGroupDiffEntry, self).__init__(diff_type, status, watchers, tasks, [], generated_diff, props=props)

    def __replace_keys_values(self, d):
        for keypath, (replkey, replfunc) in TGroupDiffEntry.PRETTY_PRINT_KEYS.iteritems():
            subdict = d
            for index, subkey in enumerate(keypath):
                if subkey not in subdict:
                    break
                if index == len(keypath) - 1:
                    subdict[replkey] = replfunc(subdict[subkey])
                    subdict.pop(subkey)
                else:
                    subdict = subdict[subkey]

    def report_text(self):
        copyd = copy.deepcopy(self.generated_diff)
        if "mastername" in copyd:
            pretty_name = "Group %s(%s)" % (copyd["groupname"], copyd["mastername"])
            copyd.pop("mastername")
        else:
            pretty_name = "Group %s" % (copyd["groupname"])
        copyd.pop("groupname")

        self.__replace_keys_values(copyd)

        d = {
            pretty_name: copyd,
        }

        return self.recurse_report_text(d)


class GroupDiffer(object):
    def __init__(self, oldgroup, newgroup, olddb=None, newdb=None):
        assert (oldgroup is not None or newgroup is not None)

        if oldgroup and newgroup:
            assert (oldgroup.card.name == newgroup.card.name)

        none_params = {
            'get_instances': lambda: [],
            'getHosts': lambda: [],
            'card': NoneSomething({
                'name': None,
                'tags': NoneSomething({'itag': []}),
                'properties': NoneSomething({}),
                'reqs': NoneSomething({'instances': NoneSomething({'memory': NoneSomething({})})}),
                'owners': [],
                'watchers': [],
            }),
        }
        self.oldgroup = oldgroup if oldgroup else NoneSomething(none_params)
        self.newgroup = newgroup if newgroup else NoneSomething(none_params)
        self.olddb = olddb
        self.newdb = newdb

    def get_diff(self):
        result = OrderedDict()
        result["status"], result["groupname"] = self.get_status_diff()

        # add master group name
        gr = self.newgroup if not isinstance(self.newgroup, NoneSomething) else self.oldgroup
        if gr.card.master is not None:
            result["mastername"] = gr.card.master.card.name

        carddiff = self.get_card_diff()
        hostdiff = self.get_host_diff()
        instancediff = self.get_instance_diff()

        if len(carddiff) > 0 or (hostdiff is not None) > 0 or len(instancediff) > 0:
            if len(carddiff) > 0:
                result["card"] = carddiff
            if hostdiff is not None:
                result["hosts"] = hostdiff
            if len(instancediff) > 0:
                result["instances"] = instancediff
            watchers = gaux.aux_staff.unwrap_dpts(self.oldgroup.card.owners) + gaux.aux_staff.unwrap_dpts(self.newgroup.card.owners)
            try:
                watchers += self.oldgroup.card.watchers
            except:
                pass
            try:
                watchers += self.newgroup.card.watchers
            except:
                pass
            return TGroupDiffEntry(EDiffTypes.GROUP_DIFF, True, watchers, [], result,
                                   props={"group": result["groupname"]})
        else:
            return TDiffEntry(EDiffTypes.GROUP_DIFF, False, [], [], [], None)

    def get_status_diff(self):
        if isinstance(self.newgroup, NoneSomething):
            return "REMOVED", self.oldgroup.card.name
        elif isinstance(self.oldgroup, NoneSomething):
            return "ADDED", self.newgroup.card.name
        else:
            return "CHANGED", self.newgroup.card.name

    def get_card_diff(self):  # FIXME: go through all card node and check every field diff
        result = OrderedDict()
        # simple fields difference
        get_field_diff(result, 'owners', self.oldgroup.card.owners, self.newgroup.card.owners)
        get_field_diff(result, 'intlookups', self.oldgroup.card.intlookups, self.newgroup.card.intlookups)
        get_field_diff(result, 'ctype', self.oldgroup.card.tags.ctype, self.newgroup.card.tags.ctype)
        get_field_diff(result, 'itype', self.oldgroup.card.tags.itype, self.newgroup.card.tags.itype)
        get_field_diff(result, 'prj', self.oldgroup.card.tags.prj, self.newgroup.card.tags.prj)
        get_field_diff(result, 'metaprj', self.oldgroup.card.tags.metaprj, self.newgroup.card.tags.metaprj)
        get_field_diff(result, 'itag', getattr(self.oldgroup.card.tags, 'itag', None),
                       getattr(self.newgroup.card.tags, 'itag', None))
        get_field_diff(result, 'nonsearch', self.oldgroup.card.properties.nonsearch,
                       self.newgroup.card.properties.nonsearch)
        get_field_diff(result, 'extra_disk_size', self.oldgroup.card.properties.extra_disk_size,
                       self.newgroup.card.properties.extra_disk_size)
        get_field_diff(result, 'extra_disk_size_per_instance',
                       self.oldgroup.card.properties.extra_disk_size_per_instance,
                       self.newgroup.card.properties.extra_disk_size_per_instance)
        get_field_diff(result, 'expires', str(self.oldgroup.card.properties.expires),
                       str(self.newgroup.card.properties.expires))

        if isinstance(self.oldgroup, NoneSomething):
            oldgroup_mem = 0
        elif self.oldgroup.parent.db.version <= "2.2.21":
            oldgroup_mem = self.oldgroup.card.reqs.instances.memory.value
        else:
            oldgroup_mem = self.oldgroup.card.reqs.instances.memory_guarantee.value

        if isinstance(self.newgroup, NoneSomething):
            newgroup_mem = 0
        elif self.newgroup.parent.db.version <= "2.2.21":
            newgroup_mem = self.newgroup.card.reqs.instances.memory.value
        else:
            newgroup_mem = self.newgroup.card.reqs.instances.memory_guarantee.value

        get_field_diff(result, 'memory_per_instance', oldgroup_mem, newgroup_mem)

        # power difference
        oldpower = int(sum(map(lambda x: x.power, self.oldgroup.get_instances())))
        newpower = int(sum(map(lambda x: x.power, self.newgroup.get_instances())))
        if oldpower != newpower:
            result['power'] = TBasicValueType(EBasicTypes.OLDNEW, oldvalue=oldpower, newvalue=newpower)

        return result

    def get_host_diff(self):
        result = OrderedDict()
        # hosts difference
        oldhosts = set(map(lambda x: x.name, self.oldgroup.getHosts()))
        newhosts = set(map(lambda x: x.name, self.newgroup.getHosts()))
        removedhosts = list(oldhosts - newhosts)
        addedhosts = list(newhosts - oldhosts)

        if len(removedhosts) and len(addedhosts):
            replacements = self.find_hosts_replacement(removedhosts, addedhosts)
            if len(replacements):
                result["replaced"] = map(lambda x: TBasicValueType(EBasicTypes.OLDNEW, oldvalue=x[0], newvalue=x[1]),
                                         replacements)
                result['replaced'] = TBasicValueType(EBasicTypes.ARR, value=result["replaced"])
                removed_repl, added_repl = zip(*replacements)
                removedhosts = filter(lambda x: x not in removed_repl, removedhosts)
                addedhosts = filter(lambda x: x not in added_repl, addedhosts)

        if len(addedhosts):
            result["added"] = TBasicValueType(EBasicTypes.ARR, value=list(addedhosts))
        if len(removedhosts):
            result["removed"] = TBasicValueType(EBasicTypes.ARR, value=list(removedhosts))

        # =========================== GENCFG-1239 START ===================================
        if not isinstance(self.newgroup, NoneSomething):
            same_host_names = sorted(oldhosts & newhosts)
            same_host_pairs = [(self.oldgroup.parent.db.hosts.get_host_by_name(x), self.newgroup.parent.db.hosts.get_host_by_name(x)) for x in same_host_names]
            modifiedhosts = OrderedDict()
            for host_in_old_db, host_in_new_db in same_host_pairs:
                host_diff = OrderedDict()
                for vlan_name in ('vlan688', 'vlan788'):
                    old_value = host_in_old_db.vlans.get(vlan_name, None)
                    new_value = host_in_new_db.vlans.get(vlan_name, None)
                    if old_value != new_value:
                        host_diff[vlan_name] = TBasicValueType(EBasicTypes.OLDNEW, oldvalue=old_value, newvalue=new_value)
                if len(host_diff):
                    modifiedhosts[host_in_old_db.name] = dict(hbf=dict(interfaces=host_diff))
            if len(modifiedhosts):
                result['modified'] = modifiedhosts
        # =========================== GENCFG-1239 FINISH ==================================

        if len(result.keys()) > 0:
            return result
        else:
            return None

    def get_instance_diff(self):
        oldsearcherlookup = self._get_searcherlookup(self.oldgroup.card.name, self.olddb)
        newsearcherlookup = self._get_searcherlookup(self.newgroup.card.name, self.newdb)

        old_instance_data = self._get_instances_data(oldsearcherlookup.get('instances', []))
        new_instance_data = self._get_instances_data(newsearcherlookup.get('instances', []))

        result = OrderedDict()
        for key in old_instance_data:
            left_diff = old_instance_data[key].difference(new_instance_data[key])
            right_diff = new_instance_data[key].difference(old_instance_data[key])
            if left_diff:
                result[key] = {'removed': TBasicValueType(EBasicTypes.ARR, value=list(left_diff))}
            if right_diff:
                result[key] = {'added': TBasicValueType(EBasicTypes.ARR, value=list(right_diff))}
        return result

    def _get_instances_data(self, searcherlookup_instances):
        instance_data = {
            'fqdns': set(),
            'ipv6bb': set(),
            'ipv6fb': set()
        }
        for instance in searcherlookup_instances:
            fqdn = instance.get('hbf', {}).get('interfaces', {}).get('backbone', {}).get('hostname')
            ipv6bb = instance.get('hbf', {}).get('interfaces', {}).get('backbone', {}).get('ipv6addr')
            ipv6fb = instance.get('hbf', {}).get('interfaces', {}).get('fastbone', {}).get('ipv6addr')

            if fqdn:
                instance_data['fqdns'].add(fqdn)
            if ipv6bb:
                instance_data['ipv6bb'].add(ipv6bb)
            if ipv6fb:
                instance_data['ipv6fb'].add(ipv6fb)
        return instance_data

    def _get_searcherlookup(self, group_name, db):
        searcherlookup = {}
        if group_name is not None and db is not None:
            searcherlookup = utils.api.searcherlookup_groups_instances.jsmain({
                'groups': group_name,
                'db': db
            })[group_name]
        return searcherlookup

    def _get_hosts_shards(self, hosts, intlookups, db):
        hosts = set(hosts)
        shardsdata = defaultdict(set)
        for intlookup in map(lambda x: db.intlookups.get_intlookup(x), intlookups):
            for shard_id in range(intlookup.get_shards_count()):
                primus = intlookup.get_primus_for_shard(shard_id)
                for instance in intlookup.get_base_instances_for_shard(shard_id):
                    if instance.host.name in hosts:
                        shardsdata[instance.host.name].add(primus)
        for host in shardsdata:
            shardsdata[host] = ''.join(sorted(shardsdata[host]))
        return shardsdata

    def find_hosts_replacement(self, oldhosts, newhosts):
        newdb_host_shards = self._get_hosts_shards(newhosts, self.newgroup.card.intlookups, self.newgroup.parent.db)
        olddb_host_shards = self._get_hosts_shards(oldhosts, self.oldgroup.card.intlookups, self.oldgroup.parent.db)

        byhash = defaultdict(list)
        for k, v in newdb_host_shards.iteritems():
            byhash[v].append(k)

        replacement_pairs = []
        for host, hdata in olddb_host_shards.iteritems():
            if hdata in byhash:
                if len(byhash[hdata]):
                    replacement_pairs.append((host, byhash[hdata].pop()))

        return replacement_pairs


def parse_cmd():
    parser = ArgumentParser(description="Get diff for group between tags")
    parser.add_argument("-o", "--old-db", type=argparse_types.gencfg_db, required=True,
                        help="Obligatory. Path or svn url to old db")
    parser.add_argument("-n", "--new-db", type=argparse_types.gencfg_db, required=True,
                        help="Obligatory. Path or svn url to new db")
    parser.add_argument("-g", "--group", type=str, required=True,
                        help="Obligatory. Group name")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    oldgroup = options.old_db.groups.get_group(options.group, raise_notfound=False)
    newgroup = options.new_db.groups.get_group(options.group, raise_notfound=False)

    return GroupDiffer(oldgroup, newgroup).get_diff()


if __name__ == '__main__':
    options = parse_cmd()

    result = main(options)

    print result[-1]
