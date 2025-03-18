#!/skynet/python/bin/python
import os
import sys
import json

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import gencfg
from core.db import CURDB
import gaux.aux_hbf


BASE_GROUPS = (
    'MAN_WEB_GEMINI_BASE',
    'SAS_WEB_GEMINI_BASE',
    'VLA_WEB_GEMINI_BASE',
)


def get_instance_shard_number_mapping(group_name):
    group = CURDB.groups.get_group(group_name)
    intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])

    mapping = {}
    for shard_id in xrange(intlookup.get_shards_count()):
        for instance in intlookup.get_base_instances_for_shard(shard_id):
            if group.card.properties.mtn.use_mtn_in_config:
                hostname = gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
            else:
                hostname = instance.host.name

            key = (hostname, int(instance.port))
            mapping[key] = shard_id
    return mapping


def search_map():
    mapping = {}
    for group in BASE_GROUPS:
        mapping.update(get_instance_shard_number_mapping(group))

    instances = [
        {
            "host": host,
            "search_port": port,
            "indexer_port": port + 2,
            "shard_min": shard_number_to_range(shard_number)[0],
            "shard_max": shard_number_to_range(shard_number)[1],
        }
        for (host, port), shard_number in mapping.items()
    ]
    return {
        'services': {
            'gemini': {
                "shard_by": "url_hash",
                "replicas": {
                    "default": instances,
                }
            }
        }
    }


def shard_number_to_range(number):
    magic_number = 1574
    number = int(number)
    if number < magic_number:
        a = number * 31
        return a, a + 30
    else:
        a = magic_number + 30 * number
        return a, a + 29


def main():
    target_file = sys.argv[1]
    with open(target_file, 'w') as f:
        json.dump(search_map(), f)


if __name__ == '__main__':
    main()
