#!/skynet/python/bin/python
"""
    This function creates cache about all instances of specified tag: write in mongo dict for every instance:
        { 'host' : '<hostname>', 'port' : '<portname>', 'major_tag' : '<major_tag>', ...
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import pymongo
import time
import re

import gencfg
from config import TAG_PATTERN
from core.argparse.parser import ArgumentParserExt
from core.db import DB
from gaux.aux_mongo import get_mongo_collection


def get_parser():
    parser = ArgumentParserExt(description="Added to mongodb mapping (tag, instance) -> (group name, other stuff)")
    parser.add_argument("--tag-name", type=str, default=None,
                        help="Optional. Tag name (otherwise last tag from repo will be used")

    return parser


def normalize(options):
    m = re.match(TAG_PATTERN, options.tag_name)
    if m is None:
        raise Exception("Specified tag <%s> does not satisfy regular expression <%s>" % (options.tag_name, TAG_PATTERN))
    options.major_tag = int(m.group(1))
    options.minor_tag = int(m.group(2))


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


def main(options):
    mydb = DB("tag@stable-%s-r%s" % (options.major_tag, options.minor_tag))

    insert_data = []
    for group in mydb.groups.get_groups():
        for instance in group.get_kinda_busy_instances():
            insert_data.append({
                'major_tag': options.major_tag,
                'minor_tag': options.minor_tag,
                'host': instance.host.name,
                'port': instance.port,
                # statistics
                'groupname': instance.type,
                'instancepower': instance.power,
                'hostpower': instance.host.power,
            })

    mongocoll = get_mongo_collection('instanceusagetagcache', timeout=300000)

    # mongocoll.remove(
    #     {'major_tag': options.major_tag, 'minor_tag': options.minor_tag},
    # )

    bulk = mongocoll.initialize_unordered_bulk_op()
    for elem in insert_data:
        bulk.insert(elem)
    bulk.execute()


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    main(options)
