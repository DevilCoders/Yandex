#!/skynet/python/bin/python
"""
    Intlookup modification script. Contains only small variety of action for intlookup:
        - build new intlookup from all group hosts
        - remove intlookup
        - make intlookup empty
        - rename intlookup
        - touch intlookup
        - split intlookup into several intlookup (one intlookup per tier)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import tempfile

import gencfg
from core.db import CURDB
from gaux.aux_utils import raise_extended_exception
from gaux.aux_shared import calc_total_shards, check_single_instance_on_host, check_single_shard, check_trivial_brigade
import config
import utils.pregen.generate_custom_intlookup
import show_stats
from core.instances import TIntl2Group, TMultishardGroup

import alloc_hosts
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

# TODO: move generate_custom intlookup here
# TODO: add rename command

RESET_METHODS = [
    "reset_brigades",
    "reset_brigade_groups",
    "remove_intlookup",
]


class EActions(object):
    BUILD = "build"
    REMOVE = "remove"
    RESET = "reset"
    TOUCH = "touch"
    RENAME = "rename"
    SPLIT = "split" # split intlookup into several intlookup one per tier
    CHANGETIER = "changetier"
    ALL = [BUILD, REMOVE, RESET, TOUCH, RENAME, SPLIT, CHANGETIER]

class Options(object):
    def __init__(self):
        self.action = None
        self.group = None
        self.verbose = False
        self.rewrite_intlookup = False
        self.reallocate_hosts = False
        self.recluster_web = False

    @staticmethod
    def get_parser():
        parser = ArgumentParserExt(usage="""
Usage: %(prog)s -a rename -s <oldname> -d <newname> [-y]
                -a remove -s <intlookup> -y
                -a touch -s <intlookup> [-y]
                -a reset (-g group [-s intlookup1,...,intlookupN] | -s intlookup1,...,intlookupN) [-e reset_method]""")
        parser.add_argument("-a", "--action", type=str, required=True,
                            choices=EActions.ALL,
                            help="Obligatory. Action to perform. Available actions: %s" % ",".join(EActions.ALL))
        parser.add_argument("-g", "--group", type=str, default=None,
                            help="Obligatory. Working group")
        parser.add_argument("-v", "--verbose", action="store_true", default=False,
                            help="Obligatory. Print what is being done")
        parser.add_argument("-y", "--apply", action="store_true", default=False,
                            help="Obligatory. Apply changes")
        parser.add_argument("-W", "--rewrite_intlookup", action="store_false", default=False,
                            help="Obligatory. Rewrite intlookup")
        parser.add_argument("-A", "--reallocate_hosts", action="store_false", default=False,
                            help="Obligatory. Reallocate hosts")
        parser.add_argument("-R", "--recluster_web", action="store_false", default=False,
                            help="Obligatory. Recluster web when reallocating hosts")
        parser.add_argument("-F", "--free_hosts", action="store_true", default=False,
                            help="Optional. Free hosts when remove intlookup")
        parser.add_argument("-r", "--recursive", action="store_true", default=False,
                            help="Optional. Apply recursively")
        parser.add_argument("-s", "--src-intlookup", type=str, default=None,
                            help="Optional. Src intlookup name (for rename action)")
        parser.add_argument("-d", "--dst-intlookup", type=str, default=None,
                            help="Optional. Destination intlookup name (for rename action)")
        parser.add_argument("-e", "--reset-method", type=str, default="reset_brigade_groups",
                            choices=RESET_METHODS,
                            help="Optional. Reset method (one of %s)" % ",".join(RESET_METHODS))
        parser.add_argument("-t", "--tier", type=argparse_types.tier, default=None,
                            help='Optional. Tier name for action <{}>'.format(EActions.CHANGETIER))

        return parser

    @staticmethod
    def from_cmd():
        parser = Options.get_parser()

        if len(sys.argv) == 1:
            sys.argv.append('-h')
        options = parser.parse_args()

        result = Options()
        for key, value in options.__dict__.items():
            setattr(result, key, value)

        return result

    @staticmethod
    def from_json(src):
        parser = Options.get_parser()
        result = parser.parse_json(src)
        return result


def normalize(options):
    if options.action == EActions.REMOVE and ((options.group is None) ^ (options.src_intlookup is None)) == False:
        raise Exception("You must specify exactly on of <--src-intlookup> <--group> option with action <remove>")

    if options.group:
        options.group = CURDB.groups.get_group(options.group)

    if options.group is not None:
        if options.group.card.reqs is None:
            raise Exception('Script does not support non-automatic groups')
    else:
        if options.action not in [EActions.TOUCH, EActions.RENAME, EActions.REMOVE, EActions.SPLIT, EActions.CHANGETIER]:
            raise Exception("Group is required")

    if options.action == EActions.TOUCH:
        options.apply = True

    if options.action == EActions.BUILD and not options.group.card.reqs.intlookup:
        raise Exception("Group %s intlookup is not set" % options.group.card.name)

    if options.action == EActions.BUILD and CURDB.intlookups.has_intlookup(
            options.group.card.reqs.intlookup) and not options.rewrite_intlookup:
        if not CURDB.intlookups.get_intlookup(options.group.card.reqs.intlookup).is_empty():
            raise Exception("Intlookup %s exists and is not empty" % options.group.card.reqs.intlookup)

    if options.action == EActions.RENAME and (not options.src_intlookup or not options.dst_intlookup):
        raise Exception("You should specify both --src-intlookup and --dst-intlookup with action EActions.RENAME")

    if options.action == EActions.SPLIT:
        if options.src_intlookup is None:
            raise Exception("Yout should specify --src-intlookup option")
        src_intlookup = CURDB.intlookups.get_intlookup(options.src_intlookup)
        if len(src_intlookup.tiers) == 1 and src_intlookup.file_name.endswith(src_intlookup.tiers[0]):
            raise Exception("Intlookup <%s> with tier <%s> is already in splitted state")

    if options.action == EActions.CHANGETIER:
        if options.src_intlookup is None:
            raise Exception("You should specify --src-intlookup option")

        options.src_intlookup = CURDB.intlookups.get_intlookup(options.src_intlookup)

        if options.src_intlookup.hosts_per_group != 1:
            raise Exception('Can not change tier for groups with number of hosts per group not equal to <1>')
        if options.tier is None:
            raise Exception('You must specify <--tier> option')


def parse_json(request):
    return Options.from_json(request)


def gen_commit_msg(request):
    if request.action == "build":
        if request.reallocate_hosts:
            return "Allocated group %s hosts and build intlookup" % request.group
        else:
            return "Build group %s intlookup" % request.group

    if request.action == "remove":
        if request.free_hosts:
            return "Removed group %s intlookup and freed hosts" % request.group
        else:
            return "Removed group %s intlookup" % request.group

    if request.action == "reset":
        return "Reseted group %s intlookup" % request.group

    raise Exception("gen_commit_msg not implemented for action %s" % request.action)


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


def build_intlookup(group, options):
    tmp_intlookup_name = None
    reqs = group.card.reqs

    if options.rewrite_intlookup:
        for intlookup in group.card.intlookups:
            CURDB.intlookups.get_intlookup(intlookup).reset()
    if options.reallocate_hosts:
        alloc_opts = {
            "group": group.card.name,
            "recluster_web": options.recluster_web,
            "use_current_hosts": True,
            "prefer_current_hosts": False,
            "apply": options.apply,
            "verbose": options.verbose,
        }
        alloc_opts = alloc_hosts.Options.from_json(alloc_opts)
        alloc_hosts.main(alloc_opts)

    try:
        action = 'build_intlookup'
        check_single_instance_on_host(action, group, intlookup=None)
        check_trivial_brigade(action, group, intlookup=None)
        check_single_shard(action, group, intlookup=None)
        total_shards = calc_total_shards(group)

        optimizer_functions = calc_optimizer_functions(reqs, total_shards)

        if tmp_intlookup_name is None:
            handle, tmp_intlookup_name = tempfile.mkstemp(prefix=config.TEMPFILE_PREFIX, suffix='.tmp',
                                                          dir=CURDB.INTLOOKUP_DIR)
            os.close(handle)
            os.unlink(tmp_intlookup_name)  # we need unique name, not file
        tmp_intlookup_name = os.path.basename(tmp_intlookup_name)

        shards_count_str = ''
        if reqs.shards.tiers:
            shards_count_str += '+' + '+'.join(reqs.shards.tiers)
        if reqs.shards.fake_shards:
            shards_count_str += '+' + str(reqs.shards.fake_shards)
        assert shards_count_str
        shards_count_str = shards_count_str[1:]

        # TODO: maybe use temporary file for output
        opt = utils.pregen.generate_custom_intlookup.Options()
        opt.base_type = group.card.name
        opt.brigade_size = 1
        opt.bases_per_group = 1
        opt.ints_per_group = 0
        opt.shards_count = shards_count_str
        opt.optimizer_functions = optimizer_functions
        opt.output_file = tmp_intlookup_name
        opt.multi_queue = False
        opt.temporary = not options.apply
        try:
            if options.verbose:
                print 'Running generate_custom_intlookup...'
            if options.verbose:
                print 'Using following optimizer functions: %s' % ', '.join(optimizer_functions)
            utils.pregen.generate_custom_intlookup.generate_custom_intlookup(opt)
        except Exception as e:
            raise_extended_exception(e, 'Could not generate intlookup\nReason: %s')

        # CURDB.groups.update_instances()
        print 'Intlookup statistics:'
        # TODO: use some common function that precalculates stuff in a single place
        show_swtypes = reqs.shards.half_cluster_off.optimize or reqs.shards.half_cluster_off.min_power or reqs.shards.half_cluster_off.min_replicas
        tmp_intlookup = CURDB.intlookups.get_intlookup(tmp_intlookup_name)
        show_stats.brief_statistics(tmp_intlookup, show_swtypes)

        if options.apply:
            if CURDB.intlookups.has_intlookup(reqs.intlookup):
                assert (CURDB.intlookups.get_intlookup(reqs.intlookup).is_empty())
                CURDB.intlookups.remove_intlookup(reqs.intlookup)
            CURDB.intlookups.rename_intlookup(tmp_intlookup_name, reqs.intlookup)
    finally:
        if CURDB.intlookups.has_intlookup(tmp_intlookup_name):
            CURDB.intlookups.remove_intlookup(tmp_intlookup_name)


def remove_intlookup(options):
    if not options.apply:
        raise Exception("Option apply=false is not supported")

    if options.group is not None:
        if options.free_hosts:
            slave_hosts = set(sum([slave.getHosts() for slave in options.group.slaves], []))
            if slave_hosts and not options.recursive:
                raise Exception("Group %s has non-empty slave groups and flag 'recursive' is not set" % options.group.card.name)
        if options.recursive:
            del_intlookups = list(set(sum([x.intlookups for x in [options.group] + options.group.slaves], [])))
        else:
            del_intlookups = options.group.card.intlookups[:]
        for intlookup in del_intlookups:
            CURDB.intlookups.remove_intlookup(intlookup)
        if options.free_hosts:
            options.group.free_hosts()
    else:
        CURDB.intlookups.remove_intlookup(options.src_intlookup)


def reset_intlookups(options):
    if options.src_intlookup is not None:
        src_intlookups = options.src_intlookup.split(',')

        if options.group is not None:
            diff_intlookups = set(src_intlookups) - set(options.group.card.intlookups)
            if len(diff_intlookups) > 0:
                raise Exception("Group %s does not contain intlookups %s" % (options.group.card.name, ",".join(diff_intlookups)))

        src_intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x), src_intlookups)
    else:
        src_intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x), options.group.card.intlookups)
        if options.recursive:
            for slave in options.group.slaves:
                options.intlookups.extend(CURDB.intlookups.get_intlookup(slave.intlookups))

    for intlookup in src_intlookups:
        intlookup.mark_as_modified()
        if options.reset_method == "reset_brigades":
            intlookup.reset(0)
        elif options.reset_method == "reset_brigade_groups":
            intlookup.reset(1)
        elif options.reset_method == "remove_intlookup":
            CURDB.intlookups.remove_intlookup(intlookup.file_name)
        else:
            raise Exception("Unknown reset_method %s" % options.reset_method)

def split_intlookup(options):
    src_intlookup = CURDB.intlookups.get_intlookup(options.src_intlookup)

    for tier_name in src_intlookup.tiers:
        dst_intlookup = CURDB.intlookups.create_empty_intlookup("%s.%s" % (src_intlookup.file_name, tier_name))

        dst_intlookup.hosts_per_group = src_intlookup.hosts_per_group
        dst_intlookup.tiers = [tier_name]
        dst_intlookup.base_type = bytes(src_intlookup.base_type)
        dst_intlookup.intl2_groups.append(TIntl2Group())

        first_shard_id, last_shard_id = src_intlookup.get_tier_shards_range(tier_name)
        first_multishard_id = first_shard_id / src_intlookup.hosts_per_group
        last_multishard_id = last_shard_id / src_intlookup.hosts_per_group
        dst_intlookup.intl2_groups[0].multishards = src_intlookup.get_multishards()[first_multishard_id:last_multishard_id]
        dst_intlookup.brigade_groups_count = len(dst_intlookup.intl2_groups[0].multishards)

        dst_intlookup.mark_as_modified()

    CURDB.intlookups.remove_intlookup(src_intlookup.file_name)


def change_tier(options):
    intlookup = options.src_intlookup
    tier = options.tier

    old_size = intlookup.get_shards_count()
    new_size = tier.get_shards_count()

    if old_size > new_size:
        print 'SHRINKING to {}'.format(new_size)
        intlookup.intl2_groups = intlookup.shrink_intl2groups_for_tier(0, new_size)
    else:
        for i in xrange(new_size - old_size):
            intlookup.intl2_groups[-1].brigade_groups.append(TMultishardGroup())

    intlookup.tiers = [tier.name]
    intlookup.brigade_groups_count = new_size

    intlookup.mark_as_modified()

def main(options):
    normalize(options)
    if options.action == EActions.REMOVE:
        remove_intlookup(options)
    elif options.action == EActions.BUILD:
        build_intlookup(options.group, options)
    elif options.action == EActions.RESET:
        reset_intlookups(options)
    elif options.action == EActions.TOUCH:
        CURDB.intlookups.get_intlookups()
    elif options.action == EActions.RENAME:
        CURDB.intlookups.get_intlookup(options.src_intlookup)
        CURDB.intlookups.rename_intlookup(options.src_intlookup, options.dst_intlookup)
        # will be rewritten with the following CURDB.update() call
    elif options.action == EActions.SPLIT:
        split_intlookup(options)
    elif options.action == EActions.CHANGETIER:
        change_tier(options)
    else:
        assert False


if __name__ == '__main__':
    options = Options.from_cmd()
    main(options)
    if not options.apply:
        print 'Changes are not saved. Use --apply or -y option to save changes.'
    else:
        CURDB.update(smart=True)
