#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB
from gaux.aux_shared import check_single_instance_on_host, check_single_shard, check_trivial_brigade
from config import TEMPFILE_PREFIX
import alloc_group
import tempfile

action = os.path.basename(__file__).split('.')[0]


def parse_cmd():
    parser = OptionParser(usage='usage: %prog -g group -r replicas')

    parser.add_option("-g", "--group", type="str", dest="group", default=None,
                      help="obligatory. Altered group")
    parser.add_option("-r", "--replicas", type="int", dest="replicas", default=None,
                      help="optional. Number of replicas for shard. If option in not set then it's taken from group card.")
    parser.add_option("-R", "--recluster-web", dest="recluster_web", default=False,
                      action='store_true', help="optional. Use MSK/AMS/..._WEB group as a source group")
    parser.add_option("-y", "--apply", dest="apply", default=False,
                      action='store_true', help="optional. Apply changes.")
    parser.add_option("-v", "--verbose", dest="verbose", default=False,
                      action='store_true', help="optional. Explain what is being done.")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options, args = parser.parse_args()

    if options.group is None:
        raise Exception("Specify altered group (--group param).")
    options.group = CURDB.groups.get_group(options.group)
    if options.group.reqs is None:
        raise Exception('Cannot %s for group "%s": group card "reqs" section is missing.' % (action, group.card.name))

    if options.replicas is None:
        options.replicas = options.group.reqs.shards.min_replicas
    else:
        if options.replicas <= 0:
            raise Exception('Invalid replicas value')

    return options


if __name__ == '__main__':
    tmp_intlookup_path = None
    try:
        options = parse_cmd()

        group = options.group
        intlookup = CURDB.intlookups.get_intlookup(group.reqs.intlookup)
        check_single_instance_on_host(action, group, intlookup)
        check_trivial_brigade(action, group, intlookup)
        check_single_shard(action, group, intlookup)

        reqs = group.reqs
        old_replicas = len(intlookup.get_base_instances_for_shard(0))
        if old_replicas == options.replicas:
            print 'There is already %s replicas' % options.replicas
            sys.exit(0)

        old_hosts = set(CURDB.groups.get_group(group.card.name).getHosts()) if CURDB.groups.has_group(
            group.card.name) else set()

        opt = alloc_group.Options()
        opt.card = group
        opt.card.reqs.shards.min_replicas = options.replicas
        opt.recluster_web = options.recluster_web
        opt.use_current_hosts = True
        opt.prefer_current_hosts = True
        if not options.apply:
            handle, tmp_intlookup_path = tempfile.mkstemp(prefix=TEMPFILE_PREFIX, suffix='.tmp',
                                                          dir=CURDB.INTLOOKUP_DIR)
            opt.custom_intlookup_path = tmp_intlookup_path
            os.close(handle)
            os.unlink(tmp_intlookup_path)
            opt.preserve_intlookup = True
        else:
            opt.custom_intlookup_path = None
            opt.preserve_intlookup = False
        opt.apply = options.apply
        opt.verbose = options.verbose
        try:
            if options.verbose:
                print 'Running alloc_group...'
            alloc_group.alloc_group(opt)
        except Exception as e:
            raise

        new_hosts = set(CURDB.groups.get_group(group.card.name).getHosts())
        if new_hosts == old_hosts:
            print 'No changes in group host list.'
            # this is only possible when we try to decrease number of replicas
            assert (old_replicas > options.replicas)
            print 'There could be other group requirements (i.e. min replicas on DC fail, etc.) that make it impossible to decrease number of replicas.'
        else:
            old_power = sum([instance.power for instance in intlookup.get_used_base_instances()], .0)
            if options.apply:
                intlookup = CURDB.intlookups.get_intlookup(group.reqs.intlookup)
            else:
                intlookup = CURDB.intlookups.get_intlookup(tmp_intlookup_path)
            new_power = sum([instance.power for instance in intlookup.get_used_base_instances()], .0)
            diff_power = new_power - old_power
            diff_power_percentage = int(100 * diff_power / old_power)
            cpu_diff = sum([host.power for host in old_hosts], .0) - sum([host.power for host in new_hosts], .0)
            mem_diff = sum([host.memory for host in old_hosts], .0) - sum([host.memory for host in new_hosts], .0)

            added_hosts = new_hosts - old_hosts
            removed_hosts = old_hosts - new_hosts
            if added_hosts:
                print 'Added hosts: %s' % ','.join(sorted([host.name for host in added_hosts]))
            if removed_hosts:
                print 'Removed hosts: %s' % ','.join(sorted([host.name for host in removed_hosts]))
            # print 'Group power change: %s abs, %s percent' % (diff_power, diff_power_percentage)
            print 'Host pool CPU change: %s%s' % ('+' if cpu_diff >= 0 else '', cpu_diff)
            print 'Host pool memory change: %s%s' % ('+' if mem_diff >= 0 else '', mem_diff)

        if not options.apply:
            print 'Changes are not saved. Use --apply or -y option to save changes.'
            os.unlink(tmp_intlookup_path)
            tmp_intlookup_path = None

    finally:
        if tmp_intlookup_path is not None and os.path.exists(tmp_intlookup_path):
            os.unlink(tmp_intlookup_path)
