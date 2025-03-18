#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.db import CURDB
from core.instances import TMultishardGroup


def get_parser():
    parser = ArgumentParserExt(usage='usage: %(prog)s [options]', prog="PROG")

    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-a", "--action", dest="action", type=str, required=True,
                        choices=["add", "resize", "remove", "rename", "touch"],
                        help="obligatory. Action to perform (add, resize, remove, rename, touch)")
    parser.add_argument("-t", "--tier", dest="tier", type=str, default=None,
                        help="obligatory. Tier name.")
    parser.add_argument("-e", "--description", type=str, default=None,
                        help="Optional. Tier description (for <add> action)")
    parser.add_argument("-n", "--new-tier", dest="new_tier", type=str, default=None,
                        help="obligatory. New tier name (to rename).")

    parser.add_argument("-p", "--primus", dest="primus", type=str, default=None,
                        help="optional. Equal to tier name by default.")
    parser.add_argument("-s", "--shards-count", dest="shards_count", type=int, default=1,
                        help="optional. Shards count. Equals to 1 by default.")
    parser.add_argument("--multi-primus", type=core.argparse.types.comma_list, default=None,
                        help="Optional. List of primuses with 1 shard each (alternative to <--primus --shards-count> options")

    parser.add_argument("-d", "--disk-size", type=int, default=0,
                        help="Optional. Disk size for action <add>")
    parser.add_argument("--fmt", type=str, default=None,
                        help="Optional. Shardid format (for example '%%(primus_name)s-%%(shard_id)04d'")
    return parser


def gen_commit_msg(request):
    if request.action == "add":
        return "Created new tier %s with %s shards" % (request.tier, request.shards_count)

    if request.action == "remove":
        return "Removed tier %s" % request.tier

    if request.action == "rename":
        return "Renamed tier %s to %s" % (request.tier, request.new_tier)

    raise Exception("gen_commit_msg is not implemented for action %s" % request.action)


def parse_json(request):
    parser = get_parser()
    request = parser.parse_json(request)
    return request


def normalize(options):
    obligatory = {
        'add': ['tier'],
        'rename': ['tier', 'new_tier'],
        'resize': ['tier'],
        'remove': ['tier'],
        'touch': [],
    }
    for option in obligatory[options.action]:
        if getattr(options, option) is None:
            raise Exception('Obligatory option %s is missing.' % option)

    if options.action == 'add' and options.primus is None:
        options.primus = options.tier

    if options.action == "resize" and options.multi_primus is not None:
        options.shards_count = len(options.multi_primus)


def main(options):
    normalize(options)

    # TODO: all write-to-disk should be out of main function!

    config = options.db.tiers
    if options.action == 'remove':
        if not config.has_tier(options.tier):
            raise Exception('Tier %s does not exist' % options.tier)
        affected_intlookups = [intlookup for intlookup in options.db.intlookups.get_intlookups()
                               if intlookup.tiers and options.tier in intlookup.tiers]
        if affected_intlookups:
            raise Exception('Tier %s cannot be removed, following intlookups has references to this tier: %s' % \
                             (options.tier, ','.join(map(lambda x: x.file_name, affected_intlookups))))
        config.remove_tier(options.tier)
    elif options.action == 'rename':
        # update tier names in intlookups
        for intlookup in options.db.intlookups.get_intlookups():
            if intlookup.tiers is not None:
                new_tiers = map(lambda x: x if x != options.tier else options.new_tier, intlookup.tiers)
                if new_tiers != intlookup.tiers:
                    intlookup.tiers = new_tiers
                    intlookup.mark_as_modified()
        # update tier names in groups
        for group in options.db.groups.get_groups():
            if group.card.searcherlookup_postactions.custom_tier.enabled == True and group.card.searcherlookup_postactions.custom_tier.tier_name == options.tier:
                group.card.searcherlookup_postactions.custom_tier.tier_name = options.new_tier
                group.mark_as_modified()

        # update tier in tiers.yaml
        if not config.has_tier(options.tier):
            raise Exception('Tier %s does not exist' % options.tier)
        if config.has_tier(options.new_tier):
            raise Exception('Tier %s already exists' % options.new_tier)
        config.rename_tier(options.tier, options.new_tier)
    elif options.action == 'add':
        if config.has_tier(options.tier):
            raise Exception('Tier %s already exists' % options.tier)
        all_primuses = set(sum([tier.get_primuses() for tier in options.db.tiers.get_tiers()], []))
        if options.primus in all_primuses:
            raise Exception('Following primus already exist: %s' % options.primus)
        src = {
            'name': options.tier,
            'description': options.description,
            'primuses': [dict(name=options.primus, shards=range(options.shards_count))],
            'disk_size': options.disk_size,
            'properties': {
                'shardid_format': options.fmt,
            },
        }
        config.add_tier_from_dict(src)
    elif options.action == 'resize':
        if not config.has_tier(options.tier):
            raise Exception('Tier %s does not exists' % options.tier)
        if len(config.get_tier(options.tier).primuses) > 1 and options.multi_primus is None:
            raise Exception('Resizing tier %s with more than one primus' % options.tier)

        oldsize = len(config.get_tier(options.tier).shard_ids)
        newsize = options.shards_count

        # update intlookups, fix size
        for intlookup in options.db.intlookups.get_intlookups():
            if intlookup.tiers == [options.tier]:  # FIXME: intlookup with multple tiers including resized one
                if (oldsize - newsize) % intlookup.hosts_per_group != 0:
                    raise Exception('Resizing tier %s from %s to %s: invalid hosts_per_group %s in intlookup %s' % \
                                     (options.tier, oldsize, newsize, intlookup.hosts_per_group, intlookup.file_name))
                if oldsize > newsize:
                    cursize = 0
                    new_intl2_groups = []
                    for intl2_group in intlookup.intl2_groups:
                        if cursize >= newsize:
                            continue
                        elif cursize + len(intl2_group.multishards) * intlookup.hosts_per_group <= newsize:
                            new_intl2_groups.append(intl2_group)
                        else:
                            intl2_group.multishards = intl2_group.multishards[
                                                      :(newsize - cursize) / intlookup.hosts_per_group]
                            new_intl2_groups.append(intl2_group)

                        cursize += len(intl2_group.multishards) * intlookup.hosts_per_group

                    intlookup.intl2_groups = new_intl2_groups
                else:
                    intlookup.intl2_groups[-1].multishards.extend(
                        map(lambda x: TMultishardGroup(), range((newsize - oldsize) / intlookup.hosts_per_group)))
                intlookup.mark_as_modified()
        # resize tiers after fix intlookups
        if options.multi_primus is not None:
            config.resize_tier(options.tier, newsize, primuses_data=map(lambda x: (x, [0]), options.multi_primus))
        else:
            config.resize_tier(options.tier, newsize)
    elif options.action == 'touch':
        pass
    else:
        raise Exception('Not implemented')


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
    options.db.update(smart=True)
