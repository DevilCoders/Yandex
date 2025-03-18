#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

# DEPRECATED. Will be split int alloc_hosts.py and alloc_intlookup.py

import os
import sys
import tempfile

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser
from core.card.node import CardNode

import utils.pregen.find_machines as find_machines
from core.db import CURDB
from core.intlookups import Intlookup
import utils.pregen.generate_custom_intlookup as generate_custom_intlookup
from gaux.aux_utils import raise_extended_exception
import config
from gaux.aux_shared import calc_source_groups, calc_total_shards, check_single_instance_on_host, check_single_shard, \
    check_trivial_brigade, create_group_host_filter
from show_stats import brief_statistics


class Options(object):
    def __init__(self):
        self.card = None
        self.recluster_web = None
        self.use_current_hosts = True
        self.prefer_current_hosts = False
        # preserved intlookup and custom intlookup path are ignored if self.apply is True
        self.custom_intlookup_path = None
        self.preserve_intlookup = False
        self.apply = True
        self.verbose = False

    def parse_cmd(self):
        parser = OptionParser(usage='usage: %prog -c cardfile [-R] [-v] or\n       %prog -g group [-R] [-u [-p]] [-v]')

        parser.add_option("-g", "--group", type="str", dest="group", default=None,
                          help="optional. Group name to reallocate existing group.")
        parser.add_option("-c", "--cardfile", type="str", dest="cardfile", default=None,
                          help="optional. Cardfile source for nonexisting group.")
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
        parser.add_option("-I", "--preserve-intlookup", dest="preserve_intlookup", default=False,
                          action='store_true', help="optional for debug with no apply. Preserve temporary intlookup.")

        if len(sys.argv) == 1:
            sys.argv.append('-h')
        options, args = parser.parse_args()

        if options.cardfile is not None and options.group is not None:
            raise Exception('Options --group and --cardfile are mutually exclusive')
        if options.cardfile is None and options.group is None:
            raise Exception('--group option or --cardfile option required')

        if options.cardfile is not None:
            if not os.path.exists(options.cardfile):
                raise Exception('Card file "%s" does not exist.' % options.cardfile)
            self.card = CardNode()
            self.card.load_from_file(options.cardfile)
            if CURDB.groups.has_group(self.card.name):
                raise Exception('Group %s already exists. Use --group parameter to recluster existing group.' % self.card.name)
        else:
            if not CURDB.groups.has_group(options.group):
                raise Exception('Group %s does not exist.' % options.group)
            self.card = CURDB.groups.get_group(options.group)
            if self.card.reqs is None:
                raise Exception('Section "reqs" is missing in group %s card file.' % options.group)
            self.use_current_group_hosts = options.use_current_hosts
            if options.prefer_current_hosts:
                self.prefer_current_hosts = True
                if not self.use_current_group_hosts:
                    raise Exception('Option --prefer-current-hosts requires option --use-current-hosts to be set')

        self.recluster_web = options.recluster_web
        self.apply = options.apply
        self.verbose = options.verbose
        self.preserve_intlookup = options.preserve_intlookup


def calc_optimizer_functions(reqs, total_shards):
    result = []
    if reqs.shards.dc_fail.optimize:
        result.append('skip_cluster_score')
    if reqs.shards.queue_fail.optimize:
        result.append('skip_subcluster_score')
    if reqs.shards.switch_fail.optimize:
        result.append('skip_switch_score')
    if reqs.shards.host_fail.optimize:
        result.append('skip_host_score')
    if reqs.shards.half_cluster_off.optimize:
        result.append('skip_switchtype_score')
    if total_shards > 1 or total_shards == 1 and not result:
        result.append('noskip_score')
    return result


def alloc_group(options):
    tmp_intlookup_path = options.custom_intlookup_path if not options.apply else None

    try:
        card = options.card

        # check implementation constraints
        action = os.path.basename(__file__).split('.')[0]
        if card.reqs is None:
            raise Exception('Cannot %s for group "%s": group card "reqs" section is missing.' % (action, card.name))
        funcs = CURDB.get_base_funcs()(card)

        reqs = card.reqs
        check_single_instance_on_host(action, card, intlookup=None)
        check_trivial_brigade(action, card, intlookup=None)
        check_single_shard(action, card, intlookup=None)
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
        opt = find_machines.Options()
        opt.min_instances = min_instances
        opt.min_power = min_instances_power
        opt.min_skip_instances = min_skip_instances
        opt.min_skip_power = min_skip_instances_power
        opt.instance_power_func = lambda host: funcs.instancePower(host, funcs.instanceCount(host))
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
            hosts = find_machines.find_machines(opt)
        except Exception as e:
            raise_extended_exception(e, 'Could not find suitable machines for group\nReason: %s')

        if options.verbose:
            print 'Found hosts: %s' % ','.join(sorted([host.name for host in hosts]))
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

        # create group, update group hosts
        if CURDB.groups.has_group(card.name):
            group = CURDB.groups.get_group(card.name)
        else:
            group = CURDB.groups.add_master_group_by_card(card)
        for host in group.getHosts():
            if host not in hosts:
                CURDB.groups.move_host(host, '%s_RESERVED' % str(host.location).upper())
        for host in hosts:
            if not group.hasHost(host):
                CURDB.groups.move_host(host, group.card.name)

        optimizer_functions = calc_optimizer_functions(reqs, total_shards)

        if tmp_intlookup_path is None:
            handle, tmp_intlookup_path = tempfile.mkstemp(prefix=config.TEMPFILE_PREFIX, suffix='.tmp',
                                                          dir=config.INTLOOKUP_DIR)
            os.close(handle)

        shards_count_str = ''
        if reqs.shards.tiers:
            shards_count_str += '+' + '+'.join(reqs.shards.tiers)
        if reqs.shards.fake_shards:
            shards_count_str += '+' + str(reqs.shards.fake_shards)
        assert shards_count_str
        shards_count_str = shards_count_str[1:]

        # TODO: maybe use temporary file for output
        opt = generate_custom_intlookup.Options()
        opt.base_type = group.card.name
        opt.brigade_size = 1
        opt.bases_per_group = 1
        opt.ints_per_group = 0
        opt.shards_count = shards_count_str
        opt.optimizer_functions = optimizer_functions
        opt.output_file = os.path.basename(tmp_intlookup_path)
        opt.multi_queue = False
        try:
            if options.verbose:
                print 'Running generate_custom_intlookup...'
            if options.verbose:
                print 'Using following optimizer functions: %s' % ', '.join(optimizer_functions)
            generate_custom_intlookup.generate_custom_intlookup(opt)
        except Exception as e:
            raise_extended_exception(e, 'Could not generate intlookup\nReason: %s')

        # CURDB.groups.update_instances()
        print 'Intlookup statistics:'
        show_swtypes = reqs.shards.half_cluster_off.optimize or reqs.shards.half_cluster_off.min_power or reqs.shards.half_cluster_off.min_replicas
        brief_statistics(tmp_intlookup_path, show_swtypes)

        if not options.apply:
            if not options.preserve_intlookup:
                os.unlink(tmp_intlookup_path)
        else:
            # TODO: update searcherlookup
            if options.verbose:
                print 'Saving groups info...'
            CURDB.groups.update()
            if options.verbose:
                print 'Done'

            # rename intlookup
            # TODO: use svn.exist() instead
            final_intlookup_path = os.path.join(config.INTLOOKUP_DIR, reqs.intlookup)
            exists = os.path.exists(final_intlookup_path)
            os.rename(tmp_intlookup_path, final_intlookup_path)
            if not exists:
                CURDB.get_repo().add([final_intlookup_path])

            # rename file_name section in intlookup
            intlookup = Intlookup(final_intlookup_path)
            intlookup.file_name = reqs.intlookup
            # intlookup.write_intlookup_to_file()
    finally:
        if tmp_intlookup_path is not None and os.path.exists(tmp_intlookup_path) and not options.preserve_intlookup:
            os.unlink(tmp_intlookup_path)


if __name__ == '__main__':
    options = Options()
    options.parse_cmd()
    alloc_group(options)
    if not options.apply:
        print 'Changes are not saved. Use --apply or -y option to save changes.'
    else:
        gencfg.update_all()
