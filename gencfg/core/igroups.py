#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'contrib')))

import hashlib
import copy
from cStringIO import StringIO
from gaux.aux_performance import perf_timer
from datetime import date
import gc
import re
import cPickle
from collections import defaultdict
import time
import md5

from multiprocessing import Process, cpu_count, Queue, Manager
from functools import partial

from core.card.node import ECardFileProto, CardNode, Scheme
from card.types import Date
from config import SCHEME_LEAF_DOC_FILE
from core.ghi import GHI
from gaux.aux_utils import indent, check_group_name
from gaux.aux_colortext import red_text
from gaux.aux_igroup import guess_group_location, guess_group_dc
from gaux.aux_portovm import gen_guest_group_name, gen_vm_host_name, guest_instance
from gaux.aux_mongo import get_next_hbf_project_id
from gaux.aux_dispenser import check_projects_allocated
from gaux.aux_reserve import get_reserve_group
from core.igroup_postactions import TPostActions
from core.igroup_triggers import TGroupTriggers
import core.igroup_validators
from core.card.node import LeafExtendedInfo
import card.types
from core.exceptions import TValidateCardNodeError
from core.settings import SETTINGS
import gaux.aux_staff


GROUP_CARD_FILE = 'card.yaml'


def split_groups(groups, num, total_processes):
    fst = len(groups) * num / total_processes
    lst = len(groups) * (num + 1) / total_processes
    return groups[fst:lst]


def validate_groups(scheme, smart, skip_leaves, num, total_processes, get_groups, queue, project_keys, skip_check_dispernser=False):
    try:
        groups = get_groups()
        if smart:
            groups = filter(lambda x: x.modified, groups)
        groups = split_groups(groups, num, total_processes)

        if len(groups):
            validator = core.igroup_validators.TValidator(groups[0].parent.db)
        else:
            validator = None

        for group in groups:
            if hasattr(group.card, 'dispenser') and not skip_check_dispernser:
                if group.card.dispenser.project_key is not None and group.card.dispenser.project_key not in project_keys:
                    project_keys[group.card.dispenser.project_key] = num
                    check_projects_allocated(group.parent.db, group.card.dispenser.project_key, True)

            group_leaves = filter(lambda x: isinstance(x, LeafExtendedInfo),
                                  group.card.save_extended_info(group.parent.SCHEME))

            for leaf in group_leaves:
                if tuple(leaf.path) in skip_leaves:
                    continue

                # validate old style
                scheme_leaf_node = scheme.resolve_scheme_path(leaf.path)
                validate_info = scheme_leaf_node.card_type.validate_for_update(group, leaf.value)
                if validate_info.status == card.types.EStatuses.STATUS_FAIL:
                    queue.put(TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason)))
                    raise TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason))

                # validate new style
                validate_info = validator.validate_group_card_field(group, group.card, leaf.path, leaf.value)
                if validate_info.status == core.igroup_validators.EValidateStatus.STATUS_FAIL:
                    queue.put(TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason)))
                    raise TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason))

            # validate group (other than single property checks)
            validate_info = core.igroup_validators.TValidator.validate(group)
            if validate_info.status == core.igroup_validators.EValidateStatus.STATUS_FAIL:
                queue.put(TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason)))
                raise TValidateCardNodeError("Group %s: %s" % (group.card.name, validate_info.reason))
    except Exception as e:
        queue.put(e)
        raise


# special object for custom instance power key
class CIPEntry(object):
    __slots__ = ['host', 'port']

    def __init__(self, host_or_instance, port=None):
        if port is not None:
            self.host = host_or_instance
            self.port = port
        else:
            self.host = host_or_instance.host
            self.port = host_or_instance.port

    def __eq__(self, obj):
        return id(self.host) == id(obj.host) and self.port == obj.port

    def __hash__(self):
        return hash(id(self.host)) ^ self.port


class IGroup(object):
    def __repr__(self):
        return '{}'.format(self.card.name)

    def __init__(self, parent, card, ghi):
        self.card = card

        # some code to work properly with base_funcs from older databases
        if parent.db.version.asint() <= 2002022:
            self.name = self.card.name
            self.legacy = self.card.legacy
            self.reqs = self.card.reqs
            self.properties = self.card.properties
            self.on_update_trigger = self.card.on_update_trigger
            self.tags = self.card.tags
            if parent.db.version <= "2.2.20":
                self.automated = self.card.automated

        self.modified = False

        self.parent = parent
        self.power = .0  # will be initialized later
        self.ghi = ghi
        self.custom_instance_power = dict()
        self.custom_instance_power_used = False
        self.extra_data = {}

        instance_func = self.gen_instance_func()
        self.ghi.add_group(self.card.name, instance_func, self.card.host_donor)

        # some cached stuff (should be removed on group update)
        self.searcherlookup = None

    def gen_instance_func(self, custom_instance_power=None):
        if custom_instance_power is None:
            custom_instance_power = self.custom_instance_power
        self.funcs = self.parent.get_base_funcs()(self)

        instanceCount = self.funcs.instanceCount
        instancePower = self.funcs.instancePower
        instancePort = self.funcs.instancePort
        type_name = self.card.name

        def instance_func(host):
            count = instanceCount(self.parent.db, host)
            powers = instancePower(self.parent.db, host, count)
            result = []
            if custom_instance_power:
                for i in range(count):
                    key = CIPEntry(host, instancePort(i))
                    result.append((host, custom_instance_power.get(key, powers[i]), instancePort(i), type_name, i))
            else:
                for i in range(count):
                    result.append((host, powers[i], instancePort(i), type_name, i))
            return result

        return instance_func

    def load_hosts(self):
        # check if file with instance power present and load instances if needed
        fname = os.path.join(self.parent.db.GROUPS_DIR,
                             self.card.master.card.name if self.card.master else self.card.name,
                             '%s.instances' % self.card.name)
        db_hosts = self.parent.db.hosts
        if os.path.exists(fname):
            self.custom_instance_power_used = True
            for line in open(fname).read().splitlines():
                parts = line.split(' ', 2)
                rawkey, custom_power = parts[:2]
                extra = parts[2] if len(parts) == 3 else None
                host, _, port = rawkey.partition(':')

                host = db_hosts.get_host_by_name(host)
                port = int(port)
                key = CIPEntry(host, port)

                self.custom_instance_power[key] = float(custom_power)
                self.extra_data[key] = extra

        if self.card.host_donor is not None:
            return
        fname = os.path.join(self.parent.db.GROUPS_DIR,
                             self.card.master.card.name if self.card.master else self.card.name,
                             '%s.hosts' % self.card.name)
        if not os.path.exists(fname):
            raise Exception("No hosts file %s for group %s" % (fname, self.card.name))
        hostnames = [x for x in open(fname).read().splitlines() if x]
        if self.parent.db.version <= '2.2.1':  # old non-fqdn lists
            hostnames = map(lambda x: self.parent.db.hosts.fqdn_mapping[x], hostnames)

        hosts = set(self.parent.db.hosts.get_hosts_by_name(hostnames))

        self.ghi.add_group_hosts(self.card.name, list(hosts))

    def get_card_file(self):
        return os.path.join(self.parent.db.GROUPS_DIR, self.card.name, GROUP_CARD_FILE)

    def get_data_files(self):
        if self.card.master is None:
            if self.card.host_donor is None:
                result = [  # os.path.join(self.parent.db.GROUPS_DIR, self.card.name),
                    os.path.join(self.parent.db.GROUPS_DIR, self.card.name, GROUP_CARD_FILE),
                    os.path.join(self.parent.db.GROUPS_DIR, self.card.name, '%s.hosts' % self.card.name)]
            else:
                result = [  # os.path.join(self.parent.db.GROUPS_DIR, self.card.name),
                    os.path.join(self.parent.db.GROUPS_DIR, self.card.name, GROUP_CARD_FILE)]
        else:
            if self.card.host_donor is None:
                result = [
                    os.path.join(self.parent.db.GROUPS_DIR, self.card.master.card.name, '%s.hosts' % self.card.name)]
            else:
                result = []

        if self.custom_instance_power_used:
            result.append(os.path.join(self.parent.db.GROUPS_DIR,
                                       self.card.master.card.name if self.card.master else self.card.name,
                                       '%s.instances' % self.card.name))

        return result

    def __eq__(self, obj):
        if obj is None:
            return False
        assert (isinstance(obj, IGroup))
        return self.card.name == obj.card.name

    def __hash__(self):
        return hash(self.card.name)

    def save_card_to_str(self):
        return self.card.save_to_str(self.parent.SCHEME)

    def save_card_to_obj(self):
        result = self.card.save_to_obj(self.parent.SCHEME)

        def dfs(node):
            # we have actually not dict, but OrderedDict here
            from collections import OrderedDict
            if isinstance(node, OrderedDict):
                return [(key, dfs(value)) for key, value in node.items()]
            return str(node)

        result = dfs(result)
        return result

    def save_card(self):
        # TODO: uncomment
        # self.checkDonor()

        if self.card.master is not None:
            raise Exception("Trying to save slave card for group %s" % self.card.name)

        filename = os.path.join(self.parent.db.GROUPS_DIR, self.card.name, GROUP_CARD_FILE)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))
        self.slaves.sort(cmp=lambda x, y: cmp(x.card.name, y.card.name))

        if self.parent.db.version >= '2.2.44':
            proto = ECardFileProto.JSON
        else:
            proto = ECardFileProto.YAML
        self.card.save_to_file(self.parent.SCHEME, filename, proto=proto)

    def save_hosts(self):
        if self.card.host_donor is not None:
            raise Exception("Trying to save hosts for group %s with host donor %s" % (self.card.name, self.card.host_donor))

        filename = os.path.join(self.parent.db.GROUPS_DIR,
                                self.card.master.card.name if self.card.master else self.card.name,
                                '%s.hosts' % self.card.name)
        if not os.path.exists(os.path.dirname(filename)):
            os.makedirs(os.path.dirname(filename))
        with open(filename, 'wb') as f:
            print >> f, '\n'.join(self.getHostNames())
            f.close()

    def save_custom_instance_power(self):
        if self.custom_instance_power_used:
            filename = os.path.join(self.parent.db.GROUPS_DIR,
                                    self.card.master.card.name if self.card.master else self.card.name,
                                    '%s.instances' % self.card.name)
            if not os.path.exists(os.path.dirname(filename)):
                os.makedirs(os.path.dirname(filename))

            data = []
            for x, y in self.custom_instance_power.iteritems():
                data.append('%s:%s %.3f' % (x.host.name, x.port, y))
                if self.extra_data.get(x):
                    data[-1] += ' {}'.format(self.extra_data[x])
            data.sort()

            with open(filename, 'wb') as f:
                f.write('\n'.join(data))

    def getBasePort(self):
        return self.funcs.instancePort(0)

    def refresh_after_card_update(self):
        # TODO: we need to check non intersecting ports!

        # we could change legacy->funcs->instanceCount parameter
        # so let's reset instances
        instance_func = self.gen_instance_func()
        self.ghi.change_group_instance_func(self.card.name, instance_func)

    # --- slave functions ---

    def slave_pos(self, slave_name):
        pos = 0
        while pos < len(self.slaves):
            if self.slaves[pos].card.name >= slave_name:
                break
            pos += 1
        return pos

    def add_slave(self, slave):
        assert (isinstance(slave, IGroup))
        self.modified = True
        self.slaves.insert(self.slave_pos(slave.card.name), slave)

    def remove_slave(self, slave):
        assert (isinstance(slave, IGroup))
        assert (slave in self.slaves)
        self.modified = True
        self.slaves.pop(self.slave_pos(slave.card.name))

    def getSlaves(self):
        return self.slaves[:]

    def getSlaveNames(self):
        return [slave.card.name for slave in self.slaves]

    # --- hosts functions ---

    def roHosts(self):
        return self.card.host_donor is not None

    def hasHostDonor(self):
        return self.ghi.has_host_donor(self.card.name)

    def clearHosts(self):
        if self.card.properties.untouchable:
            raise Exception("Group %s is untouchable, you can not clear its hosts" % self.card.name)
        self.modified = True
        self.ghi.remove_group_hosts(self.card.name, self.ghi.get_group_hosts(self.card.name))

    def addHost(self, host, run_triggers=True):
        self.modified = True
        self.ghi.add_group_hosts(self.card.name, [host])

        if run_triggers:
            TGroupTriggers.TOnAddHostsTriggers.run(self, host)
            # run trigger in all groups with host donor
            if self.card.master is None:
                master = self
            else:
                master = self.card.master
            for slave in master.slaves:
                if slave.card.host_donor == self.card.name:
                    TGroupTriggers.TOnAddHostsTriggers.run(slave, host)

    def removeHost(self, host, run_triggers=True):
        if self.card.properties.untouchable:
            raise Exception("Group %s is untouchable, you can not remove host %s" % (self.card.name, host.name))

        # triggers should be run in reverse order to addHost triggers
        if run_triggers:
            TGroupTriggers.TOnRemoveHostsTriggers.run(self, host)
            # run trigger in all groups with host donor
            if self.card.master is None:
                master = self
            else:
                master = self.card.master
            for slave in master.slaves:
                if slave.card.host_donor == self.card.name:
                    TGroupTriggers.TOnRemoveHostsTriggers.run(slave, host)

        self.modified = True
        self.ghi.remove_group_hosts(self.card.name, [host])

    def replace_host_in_intlookups(self, oldhost, newhost):
        """
            Replace host in all group intlookups
        """
        old_instances = self.get_host_instances(oldhost)
        new_instances = self.get_host_instances(newhost)

        assert len(old_instances) <= len(new_instances), "Failed to replace host <%s> by <%s> in group <%s>: not equal number of instances (%d and %d)" % \
                (oldhost.name, newhost.name, self.card.name, len(old_instances), len(new_instances))

        if self.custom_instance_power_used:
            for old_instance, new_instance in zip(old_instances, new_instances):
                new_instance.power = old_instance.power

        old_to_new_instances = dict(zip(old_instances, new_instances))

        def old_instance_replacement(instance):
            return old_to_new_instances.get(instance, None)

        for intlookup in map(lambda x: self.parent.db.intlookups.get_intlookup(x), self.card.intlookups):
            intlookup.remove_instances(new_instances)
            intlookup.replace_instances(old_instance_replacement, run_base=True, run_int=True, run_intl2=True)

    def replace_host_in_intlookups_recurse(self, oldhost, newhost):
        """
            Replace host in all group intlookups. Moreover, replace host in all intlookups for group with our group as donor
            Groups like SAS_WEB_BASE are somehow broken. Specifically, this line doesn't work:
                self.replace_host_in_intlookups(oldhost, newhost)
            To replace hosts in such groups move the line under 'else' in 'if self.card.master is None:'.
        """
        self.replace_host_in_intlookups(oldhost, newhost)
        # replace host in our group
        if self.card.master is None:
            affected_slaves = self.card.slaves
        else:
            affected_slaves = filter(lambda x: x.card.host_donor == self.card.name, self.card.master.card.slaves)

        # replace host in slave groups
        for slave_group in affected_slaves:
            slave_group.replace_host_in_intlookups(oldhost, newhost)
            slave_group.mark_as_modified()

    def replace_host(self, oldhost, newhost, run_triggers=True):
        """
            Very common operation - replace host by other host with similar characteristics.
            Do not replace hosts in slave groups, ... <newhost> should not belong to any group (if our group is master)
        """

        self.addHost(newhost, run_triggers=run_triggers)
        if self.card.master is None: # add host to all slaves
            non_donor_slaves = set(filter(lambda x: x.card.host_donor is None, self.card.slaves))
            for slave_group in (non_donor_slaves & set(self.parent.get_host_groups(oldhost))):
                slave_group.addHost(newhost)

        # replace hosts in our intlookup and all groups with our group as donor
        self.replace_host_in_intlookups_recurse(oldhost, newhost)

        # find vm groups
        if self.card.master is None:
            my_master = self
        else:
            my_master = self.card.master

        self.removeHost(oldhost, run_triggers=run_triggers)
        if self.card.master is None: # remove host from all slaves
            non_donor_slaves = set(filter(lambda x: x.card.host_donor is None, self.card.slaves))
            for slave_group in (non_donor_slaves & set(self.parent.get_host_groups(oldhost))):
                slave_group.removeHost(oldhost)

    def hasHost(self, host):
        return self.ghi.is_host_in_group(self.card.name, host)

    def getHostNames(self):
        return sorted(list(set(map(lambda x: x.name, self.ghi.get_group_hosts(self.card.name)))))

    def getHosts(self):
        return list(self.ghi.get_group_hosts(self.card.name))

    def get_hosts(self):
        return set(self.ghi.get_group_hosts(self.card.name))

    def get_busy_hosts(self):
        # ignore slaves
        return set(map(lambda x: x.host, self.get_busy_instances()))

    def get_kinda_busy_hosts(self):
        if len(self.card.intlookups):
            return self.get_busy_hosts()
        else:
            return self.getHosts()

    def get_busy_instances(self):
        busy_instances = {instance
                          for intlookup in self.card.intlookups
                          for instance in self.parent.db.intlookups.get_intlookup(intlookup).get_instances()}
        return [x for x in busy_instances if x.type == self.card.name]
        # return filter(lambda x: x.type == self.card.name, busy_instances)

    def get_kinda_busy_instances(self):
        if len(self.card.intlookups):
            return self.get_busy_instances()
        else:
            return self.get_instances()

    def get_default_port(self):
        port_func = self.card.legacy.funcs.instancePort
        if port_func == 'default':
            return 8041
        elif port_func.startswith('new') or port_func.startswith('old'):
            return int(port_func[3:])
        else:
            raise Exception('Can not convert <{}> to port'.format(port_func))

    # --- donor functions ---

    def get_group_donor(self):
        donor = self.ghi.get_group_donor(self.card.name)
        if donor is not None:
            donor = self.parent.get_group(donor)
        return donor

    def get_group_acceptors(self):
        return [self.parent.get_group(acceptor) for acceptor in self.ghi.get_group_acceptors(self.card.name)]

    # def set_group_donor(self):
    #    raise Exception('Not implemented')

    # def get_group_donor(self, donor):
    #    raise Exception('Not implemented')

    # --- instances functions ---

    def get_instances(self):
        return self.ghi.get_group_instances(self.card.name)

    def get_instance(self, host, port, power=None):
        return self.ghi.get_instance(self.card.name, host, port, power)

    def get_host_instances(self, host):
        if isinstance(host, str):
            host = self.parent.db.hosts.get_host_by_name(host)
        return self.ghi.get_group_host_instances(self.card.name, host)

    def get_int_group_name(self):
        if self.card.name.endswith('_BASE'):
            return self.card.name[:-len('_BASE')] + '_INT'
        elif self.card.name.find('_BASE_') != -1:
            return self.card.name.replace('_BASE_', '_INT_')
        else:
            return self.card.name + '_INT'

    def get_intl2_group_name(self):
        if self.card.name.endswith('_BASE'):
            return self.card.name[:-len('_BASE')] + '_INTL2'
        elif self.card.name.find('_BASE_') != -1:
            return self.card.name.replace('_BASE_', '_INTL2_')
        else:
            return self.card.name + '_INTL2'

    # ======================== TOOLSUP-20920 START ================================
    def get_extended_owners(self):
        """Return all group owners (unfold dpts ot list of users)"""
        all_dpts = {x.name for x in gaux.aux_staff.get_dpts()}

        group_owners = set(copy.copy(self.card.owners))
        group_dpts = group_owners & all_dpts
        group_owners = group_owners - all_dpts
        for dpt in group_dpts:
            dpt_users = {x.name for x in gaux.aux_staff.get_dpt_users(dpt)}
            group_owners |= set(dpt_users)
        return group_owners
    # ======================== TOOLSUP-20290 FINISH ===============================

    def is_int_group(self):
        return self.card.name.endswith('_INT') or self.card.name.find('_INT_') != -1

    def is_intl2_group(self):
        return self.card.name.endswith('_INTL2') or self.card.name.find('_INTL2_') != -1

    def is_expired(self):
        if self.card.properties.expires is None:
            return False
        return self.card.properties.expires.date < date.today()

    def free_hosts(self):
        self.parent.free_group_hosts(self)

    def mark_as_modified(self):
        self.modified = True

    def get_default_reserved_group(self):
        locations = set(location.upper() for location in self.parent.db.hosts.get_all_locations())
        prefix = self.card.name.split('_')[0].upper()
        if prefix in locations:
            result = '%s_RESERVED' % prefix
        else:
            result = 'MSK_RESERVED'
        assert (self.parent.has_group(result))
        return result

    def set_itype(self, itype, run_triggers=True):
        """
            We have to add triggers on setting new itype. Sometimes (when we set itype to portovm), we should automatically create slave group
            if needed and fill it with hosts.
        """

        self.card.tags.itype = itype
        self.modified = True

    def has_portovm_guest_group(self):
        """Check whether group has guest group or not (RX-141)"""
        return self.card.tags.itype in ('portovm' 'psi')

    # ================================== RX-224 START =====================================================
    def guest_group_card_dict(self):
        guest_card_dict = self.card.as_dict()

        # modify guest card
        if self.card.guest.tags.ctype is not None:
            guest_card_dict['tags']['ctype'] = self.card.guest.tags.ctype
        if self.card.guest.tags.itype is not None:
            guest_card_dict['tags']['itype'] = self.card.guest.tags.itype
        if self.card.guest.tags.prj != []:
            guest_card_dict['tags']['prj'] = self.card.guest.tags.prj
        if self.card.guest.tags.metaprj != 'unknown':
            guest_card_dict['tags']['metaprj'] = self.card.guest.tags.metaprj
        guest_card_dict['tags']['itag'] = []
        guest_card_dict['legacy']['funcs']['instancePort'] = 'old{}'.format(self.card.guest.port)
        guest_card_dict['legacy']['funcs']['instanceCount'] = 'default'
        guest_card_dict['owners'] = self.card.owners + [x for x in self.card.guest.owners if x not in self.card.owners]
        guest_card_dict['name'] = '{}_GUEST'.format(guest_card_dict['name'])
        guest_card_dict['master'] = None
        guest_card_dict['host_donor'] = None
        guest_card_dict['watchers'] = []
        guest_card_dict['intlookups'] = []

        # modify guest group properties
        guest_card_dict['properties']['yasmagent_production_group'] = False
        guest_card_dict['properties']['created_from_portovm_group'] = self.card.name
        guest_card_dict['properties']['monitoring_ports_ready'] = True
        guest_card_dict['properties']['monitoring_juggler_port'] = 8048
        guest_card_dict['properties']['monitoring_golovan_port'] = 8047
        guest_card_dict['properties']['cpu_guarantee_set'] = False
        guest_card_dict['properties']['hbf_project_id'] = self.card.guest.hbf_project_id
        guest_card_dict['properties']['nidx_for_group'] = None
        guest_card_dict['properties']['mtn']['tunnels']['hbf_slb_name'] = []
        guest_card_dict['properties']['mtn']['tunnels']['hbf_slb_ipv4_mtu'] = 1450
        guest_card_dict['properties']['mtn']['tunnels']['hbf_slb_ipv6_mtu'] = 1450
        guest_card_dict['properties']['mtn']['tunnels']['hbf_decapsulator_anycast_address'] = '2a02:6b8:0:3400::aaaa'
        guest_card_dict['properties']['mtn']['portovm_mtn_addrs'] = False
        guest_card_dict['properties']['ipip6_ext_tunnel'] = False
        guest_card_dict['properties']['internet_tunnel'] = False
        guest_card_dict['properties']['extra_disk_shards'] = 2
        guest_card_dict['properties']['extra_disk_size'] = 0
        guest_card_dict['properties']['extra_disk_size_per_instance'] = 0

        # modify guest group reqs
        guest_card_dict['reqs']['instances']['disk'] = 0
        guest_card_dict['reqs']['instances']['ssd'] = 0
        guest_card_dict['reqs']['instances']['port'] = 8041
        guest_card_dict['reqs']['instances']['power'] = 0
        guest_card_dict['reqs']['instances']['min_instances'] = 0
        guest_card_dict['reqs']['instances']['net_limit'] = 0
        guest_card_dict['reqs']['instances']['net_guarantee'] = 0
        guest_card_dict['reqs']['hosts']['l3enabled'] = False
        guest_card_dict['reqs']['hosts']['location']['location'] = []
        guest_card_dict['reqs']['shards']['equal_instances_power'] = False
        guest_card_dict['reqs']['shards']['max_replicas'] = 1000
        guest_card_dict['reqs']['shards']['min_power'] = 0
        guest_card_dict['reqs']['shards']['min_replicas'] = 1
        guest_card_dict['reqs']['volumes'] = []

        # modify guest group audit options
        guest_card_dict['audit']['cpu']['service_groups'] = []
        guest_card_dict['audit']['cpu']['service_coeff'] = 1.1333
        guest_card_dict['audit']['cpu']['traffic_coeff'] = 1.5
        guest_card_dict['audit']['cpu']['class_name'] = 'normal'
        guest_card_dict['audit']['cpu']['extra_cpu'] = 0.0
        guest_card_dict['audit']['cpu']['min_cpu'] = 0.0
        guest_card_dict['audit']['cpu']['greedy_limit'] = None
        guest_card_dict['audit']['cpu']['last_modified'] = None

        # modify recluster options
        guest_card_dict['recluster']['cleanup'] = []
        guest_card_dict['recluster']['alloc_hosts'] = []
        guest_card_dict['recluster']['generate_intlookups'] = []

        # modify triggers
        guest_card_dict['triggers']['on_add_host']['method'] = 'default'

        # modify configs
        guest_card_dict['configs']['enabled'] = False
        guest_card_dict['configs']['basesearch']['template'] = None
        guest_card_dict['configs']['basesearch']['custom_name'] = None
        guest_card_dict['configs']['balancer']['module_name'] = None
        guest_card_dict['configs']['balancer']['sub_module_name'] = None
        guest_card_dict['configs']['balancer']['params'] = None
        guest_card_dict['configs']['balancer']['output_file'] = None

        return guest_card_dict
    # ================================== RX-224 FINISH =====================================================

    def find_free_port_for_slave(self):
        min_port = 7000
        max_port = 16000 if self.card.name != 'ALL_DYNAMIC' else 32000
        port_step = 10

        forbidden_ports = [
            10000,  # skynet
            25530,  # issagent
        ]
        used_ports = forbidden_ports[:]
        for other_group in [self] + self.slaves:
            instance_ports = [x.port for x in other_group.get_instances()]
            if not instance_ports:
                continue
            min_other_port = min(instance_ports)
            max_other_port = max(instance_ports + [min_other_port + port_step - 1])

            # align by port_step
            min_aligned_port = (min_other_port / port_step) * port_step
            max_aligned_port = (max_other_port / port_step) * port_step

            used_ports.extend(range(min_aligned_port, max_aligned_port + port_step, port_step))
        used_ports = set(used_ports)

        avail_ports = set(range(min_port, max_port, port_step)) - used_ports
        if len(avail_ports) == 0:
            # have to fallback to allocating using find_most_unused_port
            import utils.common.find_most_unused_port
            util_params = dict(
                port_range=1,
            )
            return utils.common.find_most_unused_port.jsmain(util_params).port
        else:
            return avail_ports.pop()

    def generate_searcherlookup(self, use_cached=True, normalize=True, add_guest_hosts=False):
        if use_cached and self.searcherlookup is not None:
            return self.searcherlookup

        from core.searcherlookup import TBaseSearcherlookup
        self.searcherlookup = TBaseSearcherlookup()

        def add_to_sii(searcherlookup, instances):
            if self.has_portovm_guest_group() and add_guest_hosts:  # RX-141 (add guest instances as well)
                extended_instances = copy.copy(instances) + [guest_instance(x, db=self.parent.db) for x in instances]
            else:
                extended_instances = instances
            searcherlookup.slookup[('none', 'none')].extend(map(lambda x: x.host, extended_instances))
            searcherlookup.ilookup[('none', 'none')].extend(extended_instances)
            searcherlookup.itags_auto['a_tier_none'].extend(extended_instances)

        # fill slookup/ilookup
        if not hasattr(self, "cached_busy_instances"):
            self.cached_busy_instances = self.get_kinda_busy_instances() # do not caclulate get_kinda_busy_instances multiple times
        my_instances = self.cached_busy_instances
        if self.has_portovm_guest_group() and add_guest_hosts:
            my_with_guest_instances = my_instances + [guest_instance(x, db=self.parent.db) for x in my_instances]
        else:
            my_with_guest_instances = copy.copy(my_instances)

        if len(self.card.intlookups) == 0:
            add_to_sii(self.searcherlookup, my_instances)
        else:
            for intlookup in map(lambda x: self.parent.db.intlookups.get_intlookup(x), self.card.intlookups):
                if intlookup.tiers is None:
                    instances = filter(lambda x: x.type == self.card.name, intlookup.get_instances())
                    add_to_sii(self.searcherlookup, instances)
                    continue
                else:
                    # add stags
                    for tier_name in intlookup.tiers:
                        tier = self.parent.db.tiers.get_tier(tier_name)
                        self.searcherlookup.stags[tier].extend(
                            map(lambda x: tier.get_shard_id_for_searcherlookup(x), range(tier.get_shards_count())))

                intl2_instances = filter(lambda x: x.type == self.card.name,
                                         sum(map(lambda x: x.intl2searchers, intlookup.intl2_groups), []))
                add_to_sii(self.searcherlookup, intl2_instances)

                for shard_id in range(intlookup.get_shards_count()):
                    primus_name = intlookup.get_primus_for_shard(shard_id)
                    tier_name = intlookup.get_tier_for_shard(shard_id)
                    basesearchers = filter(lambda x: x.type == self.card.name, intlookup.get_base_instances_for_shard(shard_id))
                    if self.has_portovm_guest_group() and add_guest_hosts:
                        basesearchers.extend([guest_instance(x, db=self.parent.db) for x in basesearchers])

                    self.searcherlookup.slookup[(primus_name, tier_name)].extend(map(lambda x: x.host, basesearchers))
                    self.searcherlookup.ilookup[(primus_name, tier_name)].extend(basesearchers)
                    self.searcherlookup.itags_auto['a_tier_%s' % tier_name].extend(basesearchers)

                    if shard_id % intlookup.hosts_per_group == 0:
                        intsearchers = filter(lambda x: x.type == self.card.name, intlookup.get_int_instances_for_shard(shard_id))
                        if self.has_portovm_guest_group() and add_guest_hosts:
                            intsearchers.extend([guest_instance(x, db=self.parent.db) for x in intsearchers])
                        self.searcherlookup.slookup[('none', 'none')].extend(map(lambda x: x.host, intsearchers))
                        self.searcherlookup.ilookup[('none', 'none')].extend(intsearchers)
                        self.searcherlookup.itags_auto['a_tier_%s' % tier_name].extend(intsearchers)

        # fill geo/dc/aline tag
        instances_by_location = defaultdict(list)
        instances_by_dc = defaultdict(list)
        instances_by_queue = defaultdict(list)
        for instance in my_with_guest_instances:
            instances_by_location[instance.host.location].append(instance)
            instances_by_dc[instance.host.dc].append(instance)
            instances_by_queue[instance.host.queue].append(instance)

        for location, instances in instances_by_location.iteritems():
            self.searcherlookup.itags_auto['a_geo_%s' % location].extend(instances)
        for dc, instances in instances_by_dc.iteritems():
            self.searcherlookup.itags_auto['a_dc_%s' % dc].extend(instances)
        for queue, instances in instances_by_queue.iteritems():
            self.searcherlookup.itags_auto['a_line_%s' % queue].extend(instances)

        # fill topology tags
        repo_tag_name = self.parent.db.get_repo().get_current_tag()
        if repo_tag_name is None:
            repo_tag_name = "trunk-%s" % self.parent.db.get_repo().get_last_commit_id()
        sandbox_task_id = os.environ.get('SANDBOX_TASK_ID')

        self.searcherlookup.itags_auto['a_topology_%s' % repo_tag_name].extend(my_with_guest_instances)
        self.searcherlookup.itags_auto['a_topology_version-%s' % repo_tag_name].extend(my_with_guest_instances)
        if sandbox_task_id is not None:
            self.searcherlookup.itags_auto['a_sandbox_task_%s' % sandbox_task_id].extend(my_with_guest_instances)

        # fill memory limit tags
        if self.parent.db.version <= "2.2.21":
            memory_guarantee = int(self.reqs.instances.memory.value * params.low_limit_perc / 100)
            memory_limit = int(self.reqs.instances.memory.value * params.upper_limit_perc / 100)
        else:
            memory_guarantee = int(self.card.reqs.instances.memory_guarantee.value)
            memory_limit = int(self.card.reqs.instances.memory_guarantee.value + self.card.reqs.instances.memory_overcommit.value)

            tags = [
                'a_topology_cgset-memory.low_limit_in_bytes={}'.format(memory_guarantee),
                'a_topology_cgset-memory.limit_in_bytes={}'.format(memory_limit),
                'cgset_memory_recharge_on_pgfault_1'
            ]
            for tag in tags:
                self.searcherlookup.itags_auto[tag].extend(my_with_guest_instances)

        # fill hwaddr tags
        if self.card.tags.itype == SETTINGS.constants.portovm.itype:
            from gaux.aux_portovm import gen_vm_host_name

            for instance in my_instances:
                hwaddr = "06" + md5.new(gen_vm_host_name(self.parent.db, instance)).hexdigest()[:10].upper()
                self.searcherlookup.itags_auto['itag_hwaddr_%s' % hwaddr].append(instance)

        # fill ipv4 required tag (GENCFG-815)
        if getattr(self.card.properties, 'ipip6_ext_tunnel', False):
            self.searcherlookup.itags_auto['a_ipip6_ext_tunnel'].extend(my_instances)

        def fill_tags(self, card_name, card, suffix=''):
            tags = [
                '{}{}'.format(card_name, suffix),
                'a_topology_group-{}{}'.format(card_name, suffix),
                'a_ctype_{}'.format(card.tags.ctype),
                'a_itype_{}'.format(card.tags.itype),
            ]

            for prj in card.tags.prj:
                tags.append('a_prj_{}'.format(prj))

            if self.parent.db.version >= "2.2.6":
                tags.append('a_metaprj_{}'.format(card.tags.metaprj))
                for itag in card.tags.__dict__.get('itag', []):
                    tags.append('itag_{}'.format(itag))

            return tags

        # fill other tags.
        tags = fill_tags(self, self.card.name, self.card)
        for tag in tags:
            self.searcherlookup.itags_auto[tag].extend(my_instances)

        # fill other tags for guest group.
        if self.has_portovm_guest_group() and hasattr(self.card, 'guest') and add_guest_hosts:
            guest_tags = fill_tags(self, self.card.name, self.card.guest, '_GUEST')
            for tag in guest_tags:
                self.searcherlookup.itags_auto[tag].extend([guest_instance(x, db=self.parent.db) for x in my_instances])

        # run postactions
        self.run_searcherlookup_postactions()

        # create reverted dict
        for itag in sorted(self.searcherlookup.itags_auto):
            for instance in self.searcherlookup.itags_auto[itag]:
                self.searcherlookup.instances[instance].append(itag)

        # drop repeating stuff and sort in some sort order
        if normalize:
            self.searcherlookup.normalize(normalize_instances=False)
            return self.searcherlookup
        else:
            # do not save unnormalized searcherlookup
            result = self.searcherlookup
            self.searcherlookup = None
            return result

    def run_searcherlookup_postactions(self):
        if getattr(self.card, 'searcherlookup_postactions', None) is None:
            return

        TPostActions.process(self, self.searcherlookup)


class IGroups(object):
    GROUP_CARD_FILE = 'card.yaml'
    GROUP_HOSTS_FILE = 'hosts.txt'

    def __init__(self, db):
        self.db = db

        # TODO: Change place of groups.cache for use in sandbox between reviews
        self.CACHE_FILE = os.path.join(self.db.get_cache_dir_path(), 'groups.cache')
        if self.db.version <= '0.7':
            self.SCHEME_FILE = os.path.join(db.PATH, 'ytils', 'scheme.yaml')
        else:
            self.SCHEME_FILE = os.path.join(db.SCHEMES_DIR, 'group.yaml')
        self.INTERSECT_FILE = os.path.join(db.PATH, 'group_intersect.txt')
        self.SCHEME = Scheme(self.SCHEME_FILE, self.db.version,
                             md_doc_file=os.path.join(db.SCHEMES_DIR, SCHEME_LEAF_DOC_FILE))

        # read current data
        scheme_hash = hashlib.md5(open(self.SCHEME_FILE).read()).hexdigest()
        cards_hash = {}
        cards_content = {}
        for filename in os.listdir(self.db.GROUPS_DIR):
            if os.path.isdir(os.path.join(self.db.GROUPS_DIR, filename)):
                card_filename = os.path.join(self.db.GROUPS_DIR, filename, GROUP_CARD_FILE)
                if not os.path.exists(card_filename):
                    print red_text("Not found file <%s> in <%s>, skipping this directory ..." % (GROUP_CARD_FILE,
                                                                                                 os.path.join(
                                                                                                     self.db.GROUPS_DIR,
                                                                                                     filename)))
                else:
                    content = open(card_filename).read()
                    cards_hash[filename] = hashlib.md5(content).hexdigest()

        # read cache
        cached_cards_contents = {}
        cached_cards_hash = {}
        rewrite_cache = True
        if os.path.exists(self.CACHE_FILE):
            gc.disable()
            try:
                cache_file = open(self.CACHE_FILE)
                cached_scheme_hash = cPickle.load(cache_file)
                if cached_scheme_hash == scheme_hash:
                    cached_cards_hash = cPickle.load(cache_file)
                    cached_cards_contents = cPickle.load(cache_file)
                    rewrite_cache = cached_cards_hash != cards_hash
            finally:
                gc.enable()

        # apply cache
        for filename in set(cached_cards_hash.keys()) & set(cards_hash.keys()):
            if cards_hash[filename] == cached_cards_hash[filename]:
                cards_content[filename] = cached_cards_contents[filename]

        # process data out of cache
        for filename in set(cards_hash):
            if filename in cached_cards_hash and cached_cards_hash[filename] == cards_hash[filename]:
                cards_content[filename] = cached_cards_contents[filename]
            else:
                contents = CardNode()
                if self.db.version.asint() >= 2002044:
                    proto = ECardFileProto.JSON
                else:
                    proto = ECardFileProto.YAML
                contents.load_from_file(self.SCHEME, os.path.join(self.db.GROUPS_DIR, filename, GROUP_CARD_FILE), proto=proto)
                if contents.name != filename:
                    raise Exception('Master group %s name does not correspond with it\'s filename %s' % (contents.name, filename))
                cards_content[filename] = contents

        # write cache
        if rewrite_cache:
            buf = StringIO()
            cPickle.dump(scheme_hash, buf, cPickle.HIGHEST_PROTOCOL)
            cPickle.dump(cards_hash, buf, cPickle.HIGHEST_PROTOCOL)
            cPickle.dump(cards_content, buf, cPickle.HIGHEST_PROTOCOL)
            buf = buf.getvalue()
            try:
                with open(self.CACHE_FILE, 'wb') as f:
                    f.write(buf)
                    f.close()
            except IOError:
                # permission error?
                pass

        # load groups, unlink masters and slaves
        group_cards = []
        for card_content in cards_content.values():
            master = card_content
            # unlink slaves
            slaves = master.slaves
            master.slaves = [slave.name for slave in slaves]
            master.master = None
            group_cards.append(master)
            for slave in slaves:
                slave.master = master.name
                if slave.slaves:
                    raise Exception('Slave group %s has it\'s own slaves' % slave.name)

            # groups can depend on each other,
            # so we need to use topology sort when adding them
            proceeded = set()
            proceeded.add(master.name)
            relatives = set(slave.name for slave in slaves)
            relatives.add(master.name)

            non_acceptor_slaves = [slave for slave in slaves if not slave.host_donor]
            acceptor_slaves = [slave for slave in slaves if slave.host_donor]
            for slave in non_acceptor_slaves:
                proceeded.add(slave.name)
                group_cards.append(slave)

            for slave in acceptor_slaves:
                if slave.host_donor not in relatives:
                    raise Exception('Group %s has host donor %s which is not group relative' % (slave.name, slave.host_donor))
            cur_slaves, next_slaves = acceptor_slaves, []
            while cur_slaves:
                for slave in cur_slaves:
                    if slave.host_donor in proceeded:
                        proceeded.add(slave.name)
                        group_cards.append(slave)
                    else:
                        next_slaves.append(slave)
                cur_slaves, next_slaves = next_slaves, []

        self.ghi = GHI(self.db)
        self.groups = {}
        for group_card in group_cards:
            if group_card.name in self.groups:
                raise Exception('Group name "%s" is not unique' % group_card.name)
            self.groups[group_card.name] = IGroup(self, group_card, self.ghi)
        for group in self.groups.values():
            if group.card.master is not None:
                group.card.master = self.groups[group.card.master]
            if self.db.version.asint() <= 2002022:
                group.master = group.card.master
            group.slaves = [self.groups[slave] for slave in sorted(group.card.slaves)]
            group.card.slaves = group.slaves  # ?????
        for group in self.groups.values():
            self.groups[group.card.name].load_hosts()
        self.groupfiles = dict(map(lambda x: (x.card.name, x.get_data_files()), self.groups.values()))
        self.groupdirs = dict(map(lambda x: (x.card.name, os.path.join(self.db.GROUPS_DIR, x.card.name)),
                                  filter(lambda x: x.card.master is None, self.groups.values())))
        del group_cards  # object is actually destroyed in IGroup constructor

        # check if donors valid
        for group in self.groups.values():
            donor_names = [group.card.name]
            while group.card.host_donor:
                donor_names.append(group.card.host_donor)
                if len(donor_names) != len(set(donor_names)):
                    raise Exception("Cycled donor groups: %s" % donor_names)
                group = self.groups[group.card.host_donor]

        # need for smart updating (when we remove group, we do not mark it anywhere
        self.removed_some_group = False

    def get_scheme(self):
        return self.SCHEME

    def get_base_funcs(self):
        return self.db.get_base_funcs()

    def update(self, smart=False):
        self.__run_on_update_triggers(smart=smart)

        self.__check()
        self.check_group_cards_on_update(smart)

        if smart:
            for group in self.groups.values():
                if group.modified and group.card.master:
                    group.card.master.modified = True
                if group.modified:
                    for slave in group.slaves:
                        slave.modified = True

        self.__write_existing_groups(smart)
        self.__add_and_remove_files()

        self.removed_some_group = False


    @perf_timer
    def __run_on_update_triggers(self, smart=False):
        # contents of some groups constructed automatically (based on contents of other groups)
        # go through all that groups and run trigger
        for group in self.groups.values():
            # update CIP
            if (not smart) or (smart and group.modified):
                TGroupTriggers.TOnUpdateGroupTriggers.run(group)
                if group.custom_instance_power_used:
                    group.custom_instance_power = dict(map(lambda x: (CIPEntry(x), x.power), group.get_instances()))

            # apply trigger
            if group.card.on_update_trigger:
                d = {}
                exec group.card.on_update_trigger in d
                d['trigger'](group)

            # apply host_donors
            host_donors = getattr(group.card, 'host_donors', [])
            if host_donors:
                if group.card.on_update_trigger:
                    raise Exception('Cannot simultaneously use host_donors and on_update_trigger')

                groups = [self.get_group(group_name) for group_name in host_donors]
                if smart:
                    if not (group.modified or any(donor.modified for donor in groups)):
                        continue

                new_hosts = set(sum([donor.getHosts() for donor in groups], []))
                current_hosts = set(group.getHosts())
                if new_hosts == current_hosts:
                    return
                for host in new_hosts - current_hosts:
                    group.addHost(host)
                for host in current_hosts - new_hosts:
                    group.removeHost(host)

    @perf_timer
    def __check(self):
        self.check_group_intersections()
        # TODO: this check is too slow, will do it on update()
        # self.check_instances_intersections(host_groups)

    def check_group_intersections(self):
        msg = []
        # check if master groups do not intersect
        group_by_host = dict()
        for group in self.groups.values():
            if group.card.master is not None:
                continue
            if group.card.properties.background_group:
                continue

            for host in group.getHosts():
                if host in group_by_host:
                    msg.append('Found host {} in multiple master groups: {} {}'.format(host.name, group.card.name, group_by_host[host].card.name))
                else:
                    group_by_host[host] = group

        # check if slaves hosts as subset of master group hosts
        for group in self.groups.values():
            if group.card.master is not None:
                continue
            if group.card.properties.background_group:
                continue

            group_hosts = set(group.getHosts())
            for slave in group.card.slaves:
                if slave.card.host_donor is not None:
                    continue
                slave_group_hosts = set(slave.getHosts())
                extra_slave_hosts = slave_group_hosts - group_hosts
                for host in extra_slave_hosts:
                    msg.append('Found host {} in slave group {}, while not found in master group {}'.format(host.name, slave.card.name, group.card.name))

        if msg:
            print 'Groups intersection.failed:'
            print indent('\n'.join(msg))
            raise Exception('check_group_intersections failed')

    @perf_timer
    def check_instances_intersections(self, smart):
        # this is faster than reseting each group instances (which is done on next step)
        self.ghi.reset_all_groups_instances()
        # will update actual functions
        for group in self.groups.values():
            group.refresh_after_card_update()

        if not smart:
            check_hosts = self.db.hosts.get_hosts()
        else:
            check_hosts = set()
            for group in self.db.groups.get_groups():
                if not group.modified:
                    continue
                for host in group.getHosts():
                    check_hosts.add(host)

        success = True
        groups_by_instances_intersection = set()

        for host in check_hosts:
            groups = self.get_host_groups(host)
            ports = {}
            if len(groups) == 1:
                continue
            for group in groups:
                if group.card.master is not None and group.card.properties.share_master_ports:  # FIXME: temporary solution to prevent slave group port change
                    continue

                funcs = group.funcs
                n_instances = funcs.instanceCount(self.db, host)
                for i in range(n_instances):
                    port = funcs.instancePort(i)
                    if port not in ports:
                        ports[port] = group.card.name
                    else:
                        if success:  # do not spam this message
                            print 'Two different instances on the same port: %s:%s:%s and %s:%s:%s' % \
                                  (host.name, port, ports[port], host.name, port, group.card.name)
                        groups_by_instances_intersection.add((ports[port], group.card.name))
                        success = False
        if not success:
            raise Exception('Found some instances belongs to multiple groups. Group pairs: %s' % ("; ".join(map(lambda (x, y): "(%s, %s)" % (x, y), groups_by_instances_intersection))))

    def check_group_cards_on_update(self, smart, skip_leaves=[], skip_check_dispenser=False):
        # TODO(shotinleg): For fill dispenser projects
        skip_check_dispenser = True

        # have to load to cache all entities
        self.db.precalc_caches(quiet=True)

        # validate group card
        total_processes = cpu_count()/2
        queue = Queue()
        scheme = self.get_scheme()._scheme
        manager = Manager()
        project_keys = manager.dict()

        processes = []
        for num in range(total_processes):
            processes.append(Process(
                target=validate_groups,
                args=(scheme, smart, skip_leaves, num, total_processes, self.get_groups, queue, project_keys, skip_check_dispenser)
            ))

        for pr in processes:
            pr.start()

        for pr in processes:
            pr.join()

            code = pr.exitcode
            if not queue.empty() and code:
                raise queue.get()
            elif queue.empty() and code:
                raise Exception('validate_groups finished with code {}'.format(code))

    @perf_timer
    def __write_existing_groups(self, smart):
        for group in self.groups.values():
            if smart and not group.modified:
                continue
            group.modified = False

            if group.card.master is None:
                group.save_card()
            if group.card.host_donor is None:
                group.save_hosts()
            if group.custom_instance_power_used:
                group.save_custom_instance_power()

    @perf_timer
    def __add_and_remove_files(self):
        # find new files
        add_files = []
        remove_files = []
        remove_dirs = []
        for group in self.groups.values():
            add_files.extend(set(group.get_data_files()) - set(self.groupfiles.get(group.card.name, [])))
            remove_files.extend(set(self.groupfiles.get(group.card.name, [])) - set(group.get_data_files()))
            if group.card.master is not None and group.card.name in self.groupdirs:
                remove_dirs.append(self.groupdirs[group.card.name])
        for name in set(self.groupfiles.keys()) - set(self.groups.keys()):
            remove_files.extend(self.groupfiles[name])
            if name in self.groupdirs:
                remove_dirs.append(self.groupdirs[name])

        if add_files or remove_files:
            db_repo = self.db.get_repo()
            db_repo.add(add_files)
            db_repo.rm(remove_files)
            db_repo.rm(remove_dirs)

        self.groupfiles = dict(map(lambda x: (x.card.name, x.get_data_files()), self.groups.values()))
        self.groupdirs = dict(map(lambda x: (x.card.name, os.path.join(self.db.GROUPS_DIR, x.card.name)),
                                  filter(lambda x: x.card.master is None, self.groups.values())))

    # --- GROUP FUNCTIONS ---

    def get_group_names(self):
        return self.groups.keys()

    def get_groups(self):
        return self.groups.values()

    def get_group(self, gname, raise_notfound=True):
        if gname not in self.groups:
            if raise_notfound:
                raise Exception("Group '%s' does not exist" % gname)
            else:
                return None
        return self.groups[gname]

    def add_group(self, name, description=None, owners=None, watchers=None, instance_count_func=None,
                  instance_power_func=None, instance_port_func=None, master=None, donor=None,
                  tags=None, expires=None):
        tmp_filename = None
        try:
            check_group_name(name)
            if name in self.groups:
                raise Exception("Group '%s' already exists" % name)
            if name.endswith('_GUEST'):
                raise Exception('Group <{}> ends with <_GUEST> which is forbidden'.format(name))
            if master is not None:
                assert (isinstance(master, str))
                if master not in self.groups:
                    raise Exception("Master group '%s' does not exist" % master)
                master = self.groups[master]
                if master.card.master is not None:
                    raise Exception("Master group '%s' is a slave" % master.card.name)

            card_file = os.path.join(self.db.get_path(), 'schemes', 'group', 'empty_legacy_group.yaml')

            template = open(card_file).read()

            card_node = CardNode()
            card_node.load_from_file(self.SCHEME, card_file, proto=ECardFileProto.YAML)
            if name is not None:
                card_node.name = name
            if description is not None:
                card_node.description = description
            if owners is not None:
                card_node.owners = owners
            if watchers is not None:
                card_node.watchers = watchers
            if instance_count_func is not None:
                card_node.legacy.funcs.instanceCount = instance_count_func
            if instance_power_func is not None:
                card_node.legacy.funcs.instancePower = instance_power_func
                m = re.match('exactly(\d+)', instance_power_func)
                if m:
                    power_per_instance = int(m.group(1))
                    card_node.reqs.instances.power = power_per_instance
                    card_node.properties.cpu_guarantee_set = True
                    card_node.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
            if instance_port_func is not None:
                card_node.legacy.funcs.instancePort = instance_port_func
            if tags:
                for key, value in tags.items():
                    card_node.tags[key] = value

            # =================================== RX-539 START =========================================
            card_node.properties.created_at = Date.create_from_duration(0)
            # =================================== RX-539 FINISH ========================================
            if expires:
                card_node.properties.expires = Date.create_from_duration(expires)

            if master is not None:
                if isinstance(master, str):
                    master = self.get_group(master)
                assert (isinstance(master, IGroup))

                # set inherited properties equivalent to master values
                card_node.properties.nonsearch = master.card.properties.nonsearch
                card_node.properties.yasmagent_production_group = master.card.properties.yasmagent_production_group
                card_node.properties.yasmagent_prestable_group = master.card.properties.yasmagent_prestable_group

            if donor is not None:
                if master is None:
                    raise Exception("Master group cannot have host donor")
                relatives = set(slave.card.name for slave in master.slaves)
                relatives.add(master.card.name)
                if donor not in relatives:
                    raise Exception("Invalid host donor group value %s" % donor)
                card_node.host_donor = donor

            group = IGroup(self, card_node, self.ghi)
            group.modified = True
            group.card.master = master
            if self.db.version <= "2.2.22":
                group.master = group.card.master
            group.slaves = group.card.slaves  # ???

            # initialize instance of newly created group and all relatives to check if there is intersection
            if group.card.master is not None:
                for other_group in [group.card.master] + group.card.master.card.slaves:
                    self.ghi.init_group_instances(self.ghi.groups[other_group.card.name])
            self.ghi.init_group_instances(self.ghi.groups[group.card.name])


            if master is not None:
                master.add_slave(group)
            self.groups[group.card.name] = group
            # ... skip adding instances ?..
            # for compliance with old code

            group.src_template = template

            # NOCDEV-479 START
            group.card.properties.hbf_project_id = get_next_hbf_project_id(group.card.properties.hbf_range)
            # NOCDEV-479 FINISH

            # manipulate with cpu guarantee START
            if group.card.properties.fake_group:
                group.card.legacy.funcs.instancePower = zero
            group.card.properties.cpu_guarantee_set = True
            group.card.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
            # manipulate with cpu guarantee FINISH

            return self.get_group(group.card.name)
        finally:
            if tmp_filename:
                os.unlink(tmp_filename)

    def _load_group_card(self, group):
        if self.db.version.asint() >= 2002044:
            proto = ECardFileProto.JSON
        else:
            proto = ECardFileProto.YAML

        if group.card.master is None:
            cardfile = os.path.join(self.db.GROUPS_DIR, group.card.name, GROUP_CARD_FILE)

            card_node = CardNode()
            card_node.load_from_file(self.SCHEME, cardfile, proto=proto)

            return card_node, card_node.slaves
        else:
            cardfile = os.path.join(self.db.GROUPS_DIR, group.card.master.card.name, GROUP_CARD_FILE)

            card_node = CardNode()
            card_node.load_from_file(self.SCHEME, cardfile, proto=proto)
            card_node = filter(lambda x: x.name == group.card.name, card_node.slaves)[0]

            return card_node, []

    def copy_group(self, srcname, dstname, parent_name=None, slaves_mapping=None, donor_name=None,
                   description=None, owners=None, watchers=None, instance_count_func=None,
                   instance_power_func=None, instance_port_func=None, tags=None, expires=None):
        assert (parent_name is None or slaves_mapping is None)
        assert (not (parent_name is None and donor_name is not None))

        def guess_and_update_location(card, dstname):
            if len(card.reqs.hosts.location.location):
                guessed_loc = guess_group_location(dstname)
                if guessed_loc:
                    card.reqs.hosts.location.location = [guessed_loc]
                else:
                    card.reqs.hosts.location.location = []
            if len(card.reqs.hosts.location.dc):
                guessed_dc = guess_group_dc(dstname)
                if guessed_dc:
                    card.reqs.hosts.location.dc = [guessed_dc]
                else:
                    card.reqs.hosts.location.dc = []


        if donor_name is not None:
            avail_donors = [parent_name] + map(lambda x: x.card.name, filter(lambda y: y.card.host_donor is None,
                                                                             self.get_group(parent_name).slaves))
            if donor_name not in avail_donors:
                raise Exception("Bad donor name %s: must be one of %s" % (donor_name, ",".join(avail_donors)))

        new_card, new_slave_cards = self._load_group_card(self.get_group(srcname))

        new_card.name = dstname
        if parent_name is not None:
            parent_group = self.get_group(parent_name)
            new_card.master = parent_group
            # FIXME: avoid code dublication
            # =================================== RX-539 START =========================================
            new_card.properties.created_at = Date.create_from_duration(0)
            # =================================== RX-539 FINISH ========================================
            new_card.properties.nonsearch = parent_group.card.properties.nonsearch
            new_card.properties.yasmagent_production_group = parent_group.card.properties.yasmagent_production_group
            new_card.properties.yasmagent_prestable_group = parent_group.card.properties.yasmagent_prestable_group
            new_card.properties.full_host_group = False
            new_card.properties.fake_group = False
        else:
            new_card.master = None
        new_card.host_donor = donor_name
        new_card.slaves = []

        if description is not None:
            new_card.description = description
        if owners is not None:
            new_card.owners = owners
        if watchers is not None:
            new_card.watchers = watchers
        if instance_count_func is not None:
            new_card.legacy.funcs.instanceCount = instance_count_func
        if instance_power_func is not None:
            new_card.legacy.funcs.instancePower = instance_power_func
            m = re.match('exactly(\d+)', instance_power_func)
            if m:
                power_per_instance = int(m.group(1))
                new_card.reqs.instances.power = power_per_instance
                new_card.properties.cpu_guarantee_set = True
                new_card.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
        if instance_port_func is not None:
            new_card.legacy.funcs.instancePort = instance_port_func
        if tags:
            for key, value in tags.items():
                new_card.tags[key] = value
        if expires:
            new_card.properties.expires = Date.create_from_duration(expires)

        # if location/dc is defined in copy group, guess right location for newly created group
        guess_and_update_location(new_card, dstname)

        group = IGroup(self, new_card, self.ghi)
        group.modified = True
        group.slaves = new_card.slaves
        if self.db.version <= "2.2.22":
            group.master = group.card.master

        # initialize instance of newly created group and all relatives to check if there is intersection
        if group.card.master is not None:
            for other_group in [group.card.master] + group.card.master.card.slaves:
                self.ghi.init_group_instances(self.ghi.groups[other_group.card.name])
        self.ghi.init_group_instances(self.ghi.groups[srcname])
        self.ghi.init_group_instances(self.ghi.groups[group.card.name])

        if parent_name is not None:
            self.get_group(parent_name).slaves.append(group)
        if donor_name is None:
            olduntouchable = group.card.properties.untouchable
            group.card.properties.untouchable = False
            group.clearHosts()
            group.card.properties.untouchable = olduntouchable

        group.card.intlookups = []
        self.groups[group.card.name] = group

        if slaves_mapping is not None:
            dt = dict(map(lambda x: (x.split(':')[0], x.split(':')[1]), slaves_mapping.split(',')))

            new_slave_cards.sort(cmp=lambda x, y: cmp(x.host_donor is not None,
                                                      y.host_donor is not None))  # add firstly groups without host donors
            for slave_card in new_slave_cards:
                if slave_card.name not in dt:
                    print red_text("Slave group %s not found in mapping %s" % (slave_card.name, slaves_mapping))
                    continue

                guess_and_update_location(slave_card, dt[slave_card.name])
                slave_card.name = dt[slave_card.name]
                if slave_card.host_donor == srcname:
                    slave_card.host_donor = dstname
                elif slave_card.host_donor is not None:
                    if slave_card.host_donor == srcname:
                        slave_card.host_donor = dstname
                    else:
                        slave_card.host_donor = dt[slave_card.host_donor]
                slave_card.master = group

                # update slave group properties
                if description is not None:
                    slave_card.description = description
                if owners is not None:
                    slave_card.owners = copy.copy(owners)

                # =================================== RX-539 START =========================================
                slave_card.properties.created_at = Date.create_from_duration(0)
                # =================================== RX-539 FINISH ========================================

                slave_group = IGroup(self, slave_card, self.ghi)
                slave_group.card.intlookups = []
                slave_group.slaves = []
                slave_group.card.properties.hbf_project_id = get_next_hbf_project_id(slave_group.card.properties.hbf_range)
                if self.db.version <= "2.2.22":
                    slave_group.master = slave_group.card.master

                self.groups[slave_group.card.name] = slave_group
                group.slaves.append(slave_group)

        # NOCDEV-479 START
        group.card.properties.hbf_project_id = get_next_hbf_project_id(group.card.properties.hbf_range)
        # NOCDEV-479 FINISH

        # RX-86 START
        group.card.properties.monitoring_ports_ready = False
        group.card.properties.monitoring_juggler_port = None
        group.card.properties.monitoring_golovan_port = None
        # RX-86 FINISH

        return group

    def get_reserved_groups(self):
        p = re.compile("^[A-Z]+_RESERVED$")

        reserved_groups = filter(lambda x: p.match(x.card.name), self.get_groups())

        return reserved_groups

    def move_hosts_to_reserved(self, hosts):
        mapping = defaultdict(list)

        for host in hosts:
            reserve_group_name = get_reserve_group(host)
            mapping[reserve_group_name].append(host)
            # mapping['%s_RESERVED' % (host.location.upper())].append(host)

        for groupname, hosts in mapping.iteritems():
            self.move_hosts(hosts, groupname)

    def manipulate_with_slave(self, srcgroup, mastergroup, donorgroup, keephosts, instance_port_func):
        """
            This function is used to move slave group from one master to another or add master-group as slave to someone or
            extract slave group from some master. Params mastergroup, donorgroup could be None, keep_hosts is always True or False.
            Possible actions:
                1. Move slave to another master (mastergroup is not None). If keephosts == True, move hosts from old master to new one.
                2. Move master group to slave. Always add hosts to master group. If keep_hosts == False, move them to reserved.
                3. Extract slave from master. If keep_hosts == True, remove this hosts from master
                4. Remove host_donor flag
        """

        # set modified all affected groups
        srcgroup.mark_as_modified()
        if srcgroup.card.master is not None:
            srcgroup.card.master.mark_as_modified()
        if mastergroup is not None:
            mastergroup.mark_as_modified()
        if donorgroup is not None:
            donorgroup.mark_as_modified()

        if mastergroup is None:  # extract slave from master
            assert srcgroup.card.master is not None, "Group <%s> does not have master" % (
                srcgroup.card.master.card.name)
            srcgroup_acceptors = srcgroup.get_group_acceptors()
            assert len(srcgroup_acceptors) == 0, "Group <%s> is host donor for %s" % (
                srcgroup.card.name, ",".join(map(lambda x: x.name, srcgroup_acceptors)))
            assert (donorgroup is None), "Specified donorgroup <%s> while not specifying mastergroup" % donorgroup

            mastergroup = srcgroup.card.master
            mastergroup.slaves.remove(srcgroup)
            srcgroup.card.master = None
            srcgroup.card.host_donor = None

            hosts_to_move = srcgroup.getHosts()
            self.ghi.remove_group(srcgroup.card.name)
            self.ghi.add_group(srcgroup.card.name, srcgroup.gen_instance_func(), srcgroup.card.host_donor)
            if keephosts:
                self.move_hosts(hosts_to_move, srcgroup)

        else:  # move master or slave group to <mastergroup>
            # assert len(srcgroup.slaves) == 0, "Group <%s> has more than 0 slaves" % (srcgroup.card.name)

            if srcgroup.card.master is None:  # move master as a slave
                srcgroup.card.master = mastergroup
                mastergroup.slaves.append(srcgroup)

                if keephosts:
                    for host in srcgroup.getHosts():
                        mastergroup.addHost(host)
                else:
                    self.move_hosts_to_reserved(srcgroup.getHosts())
                    if donorgroup is not None:
                        srcgroup.card.host_donor = donorgroup.card.name
                        self.ghi.remove_group(srcgroup.card.name)
                        self.ghi.add_group(srcgroup.card.name, srcgroup.gen_instance_func(), srcgroup.card.host_donor)

                # move all srcgroup slaves to new master
                for slavegroup in srcgroup.slaves:
                    slavegroup.card.master = mastergroup
                    mastergroup.slaves.append(slavegroup)
                srcgroup.slaves = []
                srcgroup.card.slaves = srcgroup.slaves

            else:  # move slave group
                srcgroup_acceptors = srcgroup.get_group_acceptors()
                assert len(srcgroup_acceptors) == 0, "Group <%s> is host donor for %s" % (
                    srcgroup.card.name, ",".join(map(lambda x: x.card.name, srcgroup_acceptors)))

                hosts_to_move = srcgroup.getHosts()
                same_master = (srcgroup.card.master == mastergroup)

                if srcgroup.card.host_donor is not None:
                    srcgroup.card.host_donor = None
                    self.ghi.remove_host_donor(srcgroup)  # FIXME: this is not good
                self.remove_slave_hosts(hosts_to_move, srcgroup)
                srcgroup.card.master.slaves.remove(srcgroup)
                srcgroup.card.master = mastergroup
                mastergroup.slaves.append(srcgroup)

                donorname = donorgroup.card.name if donorgroup else None

                self.ghi.remove_group(srcgroup.card.name)
                srcgroup.card.host_donor = donorname
                self.ghi.add_group(srcgroup.card.name, srcgroup.gen_instance_func(), srcgroup.card.host_donor)

                if keephosts:
                    if not same_master:
                        self.move_hosts(hosts_to_move, srcgroup.card.master)
                    self.add_slave_hosts(hosts_to_move, srcgroup)
                else: # fix location properties of group
                    if mastergroup.card.reqs.hosts.location.location:
                        srcgroup.card.reqs.hosts.location.location = copy.copy(mastergroup.card.reqs.hosts.location.location)
                    if mastergroup.card.reqs.hosts.location.dc:
                        srcgroup.card.reqs.hosts.location.dc = copy.copy(mastergroup.card.reqs.hosts.location.dc)

                # fix other properties
                srcgroup.card.properties.nonsearch = mastergroup.card.properties.nonsearch
                srcgroup.card.properties.yasmagent_prestable_group = mastergroup.card.properties.yasmagent_prestable_group
                srcgroup.card.properties.yasmagent_production_group = mastergroup.card.properties.yasmagent_production_group


        if instance_port_func is not None:
            srcgroup.card.legacy.funcs.instancePort = instance_port_func

        if (len(srcgroup.card.intlookups) > 0) and (not keephosts):
            for intlookup_name in srcgroup.card.intlookups:
                self.db.intlookups.remove_intlookup(intlookup_name)

    def move_from_slave(self, srcgroup):
        assert srcgroup.card.master is not None, "group <%s> does not have master" % srcgroup.card.master.card.name
        assert srcgroup.card.host_donor is None, "Group <%s> has non-empty donor" % srcgroup.card.name

        mastergroup = srcgroup.card.master
        for host in srcgroup.getHosts():
            mastergroup.removeHost(host)
        srcgroup.card.master = None
        mastergroup.slaves = filter(lambda x: x != srcgroup, mastergroup.slaves)

    def add_master_group_by_card(self, card):
        check_group_name(card.name)
        if card.name in self.groups:
            raise Exception('Group "%s" already exists' % card.name)
        if card.reqs.brigades.ints is not None:
            raise Exception('add_master_group_by_card for group with ints is not currently implemented')
        card.add_field('master', None)
        group = IGroup(self, card, self.ghi)
        group.modified = True
        if self.db.vesion <= "2.2.22":
            group.master = group.card.master
        self.groups[group.card.name] = group
        return self.get_group(group.card.name)

    def has_group(self, name):
        return name in self.groups

    def remove_group(self, group, acceptor=None, recursive=False):
        if isinstance(group, str):
            group = self.groups[group]

        if recursive:
            while group.slaves:
                for slave in copy.copy(group.slaves):
                    if not slave.get_group_acceptors():
                        self.remove_group(slave, recursive=recursive)

        if group.slaves:
            raise Exception("Cannot delete non empty group %s. Try use recursive flag." % group.card.name)

        if group.getHosts() and not acceptor and not group.card.master and not group.hasHostDonor():
            print 'Warning: host acceptor group is not set. All hosts go to RESERVED groups'
            acceptor = 'RESERVED'

        if group.card.master and acceptor:
            raise Exception("Cannot move hosts from slave group %s. Do not use acceptor group param." % group.card.name)

        if group.hasHostDonor() and acceptor:
            raise Exception("Cannot move hosts from group %s which has host donor. Remove hosts from host donor group %s instead." % (group.card.name, group.card.host_donor))

        intlookups = set(group.card.intlookups)
        if intlookups:
            for intlookup in intlookups:
                self.db.intlookups.remove_intlookup(intlookup)

        if acceptor:
            assert (group.card.master is None)
            hosts = group.getHosts()
            for host in hosts:
                if acceptor != 'RESERVED':
                    self.move_host(host, acceptor)
                else:
                    self.move_host(host, '%s_RESERVED' % host.location.upper())

        if group.card.master:
            if not group.hasHostDonor():
                hosts = group.getHosts()
                for host in hosts:
                    group.removeHost(host)
            group.card.master.remove_slave(group)

        self.ghi.remove_group(group.card.name)
        del self.groups[group.card.name]

        self.removed_some_group = True

    def rename_group(self, old_name, new_name):
        check_group_name(new_name)
        if new_name in self.groups:
            raise Exception('Group %s already exists' % new_name)
        if new_name.endswith('_GUEST'):
            raise Exception('Rename to group <{}>, ending with <_GUEST> is forbidden'.format(new_name))

        group = self.groups[old_name]
        group.modified = True

        # load intlookups
        intlookups = []
        for intlookup_file in group.card.intlookups:
            intlookup = self.db.intlookups.get_intlookup(intlookup_file)
            intlookups.append(intlookup)

        self.groups.pop(group.card.name)
        group.card.name = new_name
        self.groups[group.card.name] = group
        acceptors = self.ghi.get_group_acceptors(old_name)
        for acceptor in acceptors:
            self.groups[acceptor].card.host_donor = new_name
            if self.db.version <= "2.2.22":
                self.groups[acceptor].host_donor = new_name
        self.ghi.rename_group(old_name, new_name)

        if intlookups:
            for intlookup in intlookups:
                intlookup.mark_as_modified()
                if intlookup.base_type == old_name:
                    intlookup.base_type = new_name

                # sometimes intlookup can not share instances with group
                for instance in intlookup.get_used_instances():
                    if instance.type == old_name:
                        instance.type = new_name

                self.db.intlookups.rename_group(intlookup.file_name, old_name, new_name)

        TGroupTriggers.TOnRenameGroupTriggers.run(group, old_name)

    # --- HOST FUNCTIONS ---
    def move_host(self, host, acceptor):
        self.move_hosts([host], acceptor)

    def move_hosts(self, hosts, acceptor):
        if acceptor:
            if isinstance(acceptor, str):
                acceptor = self.get_group(acceptor)

            if acceptor.card.master:
                raise Exception("Can not move host to slave group %s" % acceptor.name)
            if acceptor.card.host_donor is not None:
                raise Exception("Group %s has hosts donor group %s. Move host straight to donor group" % (acceptor.card.name, acceptor.card.host_donor))

        hosts_by_group = defaultdict(list)
        for host in hosts:
            host_groups = self.get_host_groups(host)
            for group in host_groups:
                hosts_by_group[group].append(host)

        for group, group_hosts in hosts_by_group.items():
            if (not group.card.properties.background_group):
                self._check_can_remove_hosts(group_hosts, group)
        if acceptor:
            self._check_can_add_hosts(hosts, acceptor)

        # sort groups to remove hosts (first groups with donor and master, then with master only)
        groups_to_remove = hosts_by_group.keys()
        groups_to_remove.sort(key=lambda x: -(x.card.host_donor is not None) - (x.card.master is not None))
        for group in groups_to_remove:
            group_hosts = hosts_by_group[group]
            if (not group.card.properties.background_group):
                self.remove_hosts(group_hosts, group)
        if acceptor:
            self.add_hosts(hosts, acceptor)

    def free_group_hosts(self, group):
        if group.card.host_donor:
            raise Exception("Cannot free hosts of acceptor group. You need to free hosts of group %s" % group.card.host_donor)
        if group.card.master is not None:
            self.remove_slave_hosts(group.getHosts(), group)
        else:
            hosts = group.getHosts()
            hosts = set(hosts)

            for g in group.card.slaves + [group]:
                if g.card.host_donor:
                    continue
                g_hosts = list(set(g.getHosts()) & hosts)
                self._check_can_remove_hosts(g_hosts, g)

            for g in group.card.slaves + [group]:
                if g.card.host_donor:
                    continue
                g_hosts = list(set(g.getHosts()) & hosts)
                self.remove_hosts(g_hosts, g)

            by_location = defaultdict(set)
            for h in hosts:
                by_location[h.location].add(h)
            for l, l_hosts in by_location.items():
                self.add_hosts(list(l_hosts), self.get_group(('%s_RESERVED' % l).upper()))

    def add_slave_host(self, host, acceptor):
        self.add_slave_hosts([host], acceptor)

    def add_slave_hosts(self, hosts, acceptor):
        if not acceptor.card.master:
            raise Exception("No master for group %s" % acceptor.card.name)

        self._check_can_add_hosts(hosts, acceptor)
        self.add_hosts(hosts, acceptor)

    def remove_slave_host(self, host, group):
        self.remove_slave_hosts([host], group)

    def remove_slave_hosts(self, hosts, group):
        """Remove hosts from group (mainly for slaves, but for master groups works as well"""
        if group.card.host_donor is not None:
            raise Exception('Can not remove hosts from group <{}> with host donor <{}>'.format(group.card.name, group.card.host_donor))

        for slave_group in group.card.slaves:
            if slave_group.card.host_donor is None:
                slave_hosts = list(set(hosts) & set(slave_group.getHosts()))
                self.remove_slave_hosts(slave_hosts, slave_group)

        # first, remove hosts from intlookups
        for intlookup in map(lambda x: self.db.intlookups.get_intlookup(x), group.card.intlookups):
            intlookup.remove_hosts(hosts, group)

        # second, remove hosts from intlookups of groups with group as donor
        for slave_group in (x for x in self.db.groups.get_groups() if x.card.host_donor is not None and x.card.host_donor == group.card.name):
            for intlookup in map(lambda x: self.db.intlookups.get_intlookup(x), group.card.intlookups):
                intlookup.remove_hosts(hosts, slave_group)

        self._check_can_remove_hosts(hosts, group)
        self.remove_hosts(hosts, group)

    def _check_can_remove_hosts(self, hosts, group):
        hosts = set(hosts)

        # TODO: check master intersections

        # check if hosts are in group
        group_hosts = set(group.getHosts())
        bad_hosts = hosts - group_hosts
        if bad_hosts:
            raise Exception("Cannot remove hosts from group %s, following hosts are not in group: %s" % (group.card.name, ','.join(sorted([host.name for host in bad_hosts]))))

    def _check_can_add_hosts(self, hosts, group):
        hosts = set(hosts)

        # TODO: check master intersections

        if group.card.master:
            master_hosts = set(group.card.master.getHosts())
            bad_hosts = hosts - master_hosts
            if bad_hosts:
                raise Exception('Cannot add hosts to group %s, following hosts are not in master group %s: %s' % \
                                 (group.card.name, group.card.master.card.name,
                                  ','.join(sorted([host.name for host in bad_hosts]))))

        group_hosts = set(group.getHosts())
        bad_hosts = group_hosts & hosts
        if bad_hosts:
            raise Exception('Cannot add hosts to group %s, following hosts are already in group (%d total): %s' % \
                             (group.card.name, len(bad_hosts),
                              ','.join(sorted([host.name for host in list(bad_hosts)[:100]]))))

    def add_hosts(self, hosts, group):
        """
            Basic add host function. Should be used with caution (not to add hosts to new group before removing from old one)
        """

        assert (group.card.host_donor is None)
        for host in hosts:
            group.addHost(host)

    def remove_hosts(self, hosts, group):
        """
            Basic remove host function. Should be used with caution (not to remove host from group before adding to new one)
        """

        # remove hosts from intlookups
        for intlookup in map(lambda x: self.db.intlookups.get_intlookup(x), group.card.intlookups):
            intlookup.remove_hosts(hosts, group)

        # remove hosts from group
        if group.card.host_donor is None:
            for host in hosts:
                group.removeHost(host)

    def get_hosts_raw(self, data):
        hosts = set()
        for s in data.split():
            if s[0] not in ['+', '-']:
                raise Exception("Group <<%s>> must start with + or -" % s)
            plus = s[0] == '+'

            gname = s[1:]
            if gname in self.groups:
                if plus:
                    hosts |= set(self.groups[gname].getHosts())
                else:
                    hosts -= set(self.groups[gname].getHosts())
            else:
                if gname in self.db.hosts.hosts:
                    if plus:
                        hosts.add(self.db.hosts.get_host_by_name(gname))
                    else:
                        hosts.discard(self.db.hosts.get_host_by_name(gname))
                else:
                    raise Exception("Unknown host <<%s>>" % gname)
        return list(hosts)

    # instances functions

    def get_all_instances(self):
        return sum([group.get_instances() for group in self.groups.values()], [])

    def get_instance_from_intlookup(self, data):
        host, port, power, group = data.split(':')
        group = self.get_group(group)

        instance = group.get_instance(str(host), int(port), float(power))
        return instance

    def get_instance_by_N(self, host_name, group, N):
        group = self.get_group(group)
        port = group.funcs.instancePort(N)
        instance = group.get_instance(host_name, port)
        return instance

    def get_host_groups(self, host):
        if isinstance(host, str):
            host = self.db.hosts.get_host_by_name(host)
        return [self.groups[group] for group in self.ghi.get_host_groups(host)]

    def get_host_master_group(self, host, fail_on_error=False):
        """
            Return <master> group for host. Should be exactly one master group for every host (sometimes this happens and indicates error)

            :type host: core.hostsinfo.Host
            :type fail_on_error: bool

            :param host: host to find master group for:
            :param fail_on_error: whether throw exception or not if not exactly one master group found
            :return (core.igroups.IGroup or None): master group
        """

        master_groups = []
        for group in self.get_host_groups(host):
            if group.card.master is not None:
                continue
            if group.card.on_update_trigger is not None:
                continue
            if group.card.properties.background_group:
                continue
            master_groups.append(group)

        if fail_on_error and len(master_groups) > 0:
            raise Exception("Host %s has %d master groups: %s" % (host.name, len(master_groups), ",".join(map(lambda x: x.name, master_groups))))
        if len(master_groups) == 0:
            master_groups.append(None)

        return master_groups[0]

    def get_host_instances(self, host):
        if isinstance(host, str):
            host = self.db.hosts.get_host_by_name(host)
        return self.ghi.get_host_instances(host)

    def fast_check(self, timeout):
        # if we were created => we are ok
        # but, as noone check logic, let's try to get real objects
        self.get_groups()
