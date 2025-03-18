#!/skynet/python/bin/python
"""Dump group card/instances info as json"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import simplejson
import datetime

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.card.types import ByteSize


def get_parser():
    parser = ArgumentParserExt(description='Dump gencfg groups in json format (for former load by gencfg2)')
    parser.add_argument('-o', '--output-dir', type=str, default=None,
                        help='Obligatory. Output directory')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, required=True,
                        help='Obligatory. List of groups to process')

    return parser


def normalize(options):
    if options.output_dir is None:
        options.output_dir = os.path.join(CURDB.get_path(), 'groups_as_json')


def _transform(obj):
    if isinstance(obj, (int, float, str, bool)):
        return obj
    elif isinstance(obj, ByteSize):
        return obj.value
    elif obj is None:
        return obj
    elif isinstance(obj, datetime.date):
        return 0
    elif isinstance(obj, list):
        return [_transform(x) for x in obj]
    elif isinstance(obj, dict):
        result = dict()
        for k, v in obj.iteritems():
            result[k] = _transform(v)
        return result
    else:
        result = dict()
        for k, v in obj.__dict__.iteritems():
            result[k] = _transform(v)
        return result

def group_as_json(group):
    return {
        'name': group.card.name,
        'parent': None if group.card.master is None else group.card.master.card.name,
        'donor': None if group.card.host_donor is None else group.card.host_donor,
        'description': group.card.description,
        'owners': group.card.owners,
        'tags': _transform(group.card.tags),
        'reqs': _transform(group.card.reqs),
        'legacy': _transform(group.card.legacy),
        'properties': _transform(group.card.properties),
    }


def group_instances_as_json(group):
    if len(group.card.intlookups) == 0: # no intlookups
        if group.card.searcherlookup_postactions.custom_tier.enabled:
            tier = CURDB.tiers.get_tier(group.card.searcherlookup_postactions.custom_tier.tier_name)
            shard_name = tier.get_shard_id_for_searcherlookup(0)
        else:
            shard_name = None
        return [dict(host=x.host.name, port=x.port, power=x.power, shard=shard_name) for x in group.get_kinda_busy_instances()]
    else:
        intlookups = [CURDB.intlookups.get_intlookup(x) for x in group.card.intlookups]
        if group.card.name not in [x.base_type for x in intlookups]:  # have intlookups but we are not basesearchers
            return [dict(host=x.host.name, port=x.port, power=x.power, shard=None) for x in group.get_kinda_busy_instances()]
        else:
            result = []
            for intlookup in intlookups:
                for shard_id in xrange(intlookup.get_shards_count()):
                    if intlookup.tiers:
                        shard_name = intlookup.get_primus_for_shard(shard_id)
                    else:
                        shard_name = None

                    for instance in intlookup.get_base_instances_for_shard(shard_id):
                        result.append(dict(host=instance.host.name, port=instance.port, power=instance.power, shard=shard_name))
            return result


def main(options):
    if not os.path.exists(options.output_dir):
        os.makedirs(options.output_dir)

    for group in options.groups:
        subdir = os.path.join(options.output_dir, group.card.name)

        if not os.path.exists(subdir):
            os.makedirs(subdir)

        with open(os.path.join(subdir, 'card'), 'w') as f:
            f.write(simplejson.dumps(group_as_json(group)))
        with open(os.path.join(subdir, 'instances'), 'w') as f:
            f.write(simplejson.dumps(group_instances_as_json(group)))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
