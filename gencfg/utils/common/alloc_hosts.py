#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

from optparse import OptionParser

from core.db import CURDB
import utils.pregen.find_machines
from gaux.aux_utils import raise_extended_exception
from gaux.aux_shared import calc_source_groups, calc_total_shards, check_single_instance_on_host, check_single_shard, \
    check_trivial_brigade, create_group_host_filter


class Options(object):
    def __init__(self):
        self.group = None
        self.recluster_web = None
        self.use_current_hosts = False
        self.prefer_current_hosts = False
        # preserved intlookup and custom intlookup path are ignored if self.apply is True
        self.apply = False
        self.verbose = False

    @staticmethod
    def from_cmd():
        parser = OptionParser(usage='usage: %prog -c cardfile [-R] [-v] or\n       %prog -g group [-R] [-u [-p]] [-v]')
        parser.add_option("-g", "--group", type="str", dest="group", default=None,
                          help="optional. Group name to reallocate existing group.")
        parser.add_option("-R", "--recluster-web", dest="recluster_web", default=False,
                          action='store_true', help="optional. Use MSK/AMS/..._WEB group as a source group")
        parser.add_option("-u", "--use-current-hosts", dest="use_current_hosts", default=True,
                          action='store_true', help="optional. Use current group hosts (for existing groups only).")
        parser.add_option("-p", "--prefer-current-hosts", dest="prefer_current_hosts", default=False,
                          action='store_true', help="optional. Prefer current group hosts (for existing groups only).")
        parser.add_option("-y", "--apply", dest="apply", default=False,
                          action='store_true', help="optional. Apply changes.")
        parser.add_option("-v", "--verbose", dest="verbose", default=False,
                          action='store_true', help="optional. Explain what is being done.")

        if len(sys.argv) == 1:
            sys.argv.append('-h')
        options, args = parser.parse_args()

        result = Options()
        result.group = options.group
        result.recluster_web = options.recluster_web
        result.use_current_hosts = options.use_current_hosts
        result.prefer_current_hosts = options.prefer_current_hosts
        # preserved intlookup and custom intlookup path are ignored if self.apply is True
        result.apply = options.apply
        result.verbose = options.verbose
        return result

    @staticmethod
    def from_json(src):
        result = Options()
        for key in src:
            setattr(result, key, src[key])
        return result

    def normalize(self):
        if self.group is None:
            raise Exception('--group option required')

        self.card = CURDB.groups.get_group(self.group)
        if self.card.reqs is None:
            raise Exception('Section "reqs" is missing in group %s card file.' % self.group)
        if self.prefer_current_hosts and not self.use_current_hosts:
            raise Exception('Option --prefer-current-hosts requires option --use-current-hosts to be set')


def main(options):
    options.normalize()

    card = CURDB.groups.get_group(options.group)

    # check implementation constraints
    if card.reqs is None:
        raise Exception('Cannot %s for group "%s": group card "reqs" section is missing.' % (options.action, card.name))
    intlookup = card.reqs.intlookup
    if not intlookup:
        raise Exception("Group %s intlookup is not set" % card.name)
    if card.get_busy_hosts():
        raise Exception('%s is not permitted, because some group hosts are used in intlookups %s' % (options.action, ','.join(card.intlookups)))

    funcs = CURDB.get_base_funcs()(card)
    reqs = card.reqs
    check_single_instance_on_host(options.action, card, intlookup=None)
    check_trivial_brigade(options.action, card, intlookup=None)
    check_single_shard(options.action, card, intlookup=None)
    total_shards = calc_total_shards(card)

    src_groups = calc_source_groups(card, recluster_web=options.recluster_web, verbose=options.verbose)
    host_filter = create_group_host_filter(card)
    src_hosts = []
    preferred_hosts = []
    if options.use_current_hosts and CURDB.groups.has_group(card.name):
        src_hosts = CURDB.groups.get_group(card.name).getHostNames()
        if options.prefer_current_hosts:
            preferred_hosts = src_hosts

    min_instances = total_shards * reqs.shards.min_replicas
    reqs.instances.add_field('min_instances', min_instances)
    min_instances_power = total_shards * reqs.shards.min_power
    reqs.instances.add_field('min_instances_power', min_instances_power)
    min_skip_instances = reqs.shards.dc_fail.min_replicas * total_shards
    min_skip_instances_power = reqs.shards.dc_fail.min_power * total_shards
    max_hosts_per_switch = reqs.hosts.max_per_switch
    max_hosts_per_queue = reqs.hosts.max_per_queue

    # call find_machines
    opt = utils.pregen.find_machines.Options()
    opt.min_instances = min_instances
    opt.min_power = min_instances_power
    opt.min_skip_instances = min_skip_instances
    opt.min_skip_power = min_skip_instances_power
    opt.instance_power_func = lambda host: funcs.instancePower(CURDB, host, funcs.instanceCount(CURDB, host))
    opt.host_filter = host_filter
    opt.src_groups = src_groups
    opt.src_hosts = src_hosts
    opt.preferred_hosts = preferred_hosts
    opt.max_hosts_per_switch = max_hosts_per_switch
    opt.max_hosts_per_queue = max_hosts_per_queue
    opt.verbose = options.verbose
    try:
        if options.verbose:
            print 'Running find_machines...'
        hosts = utils.pregen.find_machines.find_machines(opt)
    except Exception as e:
        raise_extended_exception(e, 'Could not find suitable machines for group\nReason: %s')

    print 'Found hosts: %s' % ','.join(sorted([host.name for host in hosts]))
    if options.verbose:
        if CURDB.groups.has_group(card.name):
            old_hosts = set(CURDB.groups.get_group(card.name).getHosts())
            added_hosts = hosts - old_hosts
            removed_hosts = old_hosts - hosts
            if added_hosts:
                print 'Added hosts: %s' % ','.join(sorted([host.name for host in added_hosts]))
            if removed_hosts:
                print 'Removed hosts: %s' % ','.join(sorted([host.name for host in removed_hosts]))
            if not added_hosts and not removed_hosts:
                print 'No changes in group host list.'

    if not options.apply:
        print 'Changes are not saved. Use --apply or -y option to save changes.'
    else:
        group = CURDB.groups.get_group(card.name)
        for host in group.getHosts():
            if host not in hosts:
                CURDB.groups.move_host(host, '%s_RESERVED' % str(host.location).upper())
        for host in hosts:
            if not group.hasHost(host):
                CURDB.groups.move_host(host, group.card.name)

    return hosts


if __name__ == '__main__':
    options = Options.from_cmd()
    main(options)
    if options.apply:
        if options.verbose:
            print 'Saving groups info...'
        CURDB.groups.update()
        if options.verbose:
            print 'Done'
