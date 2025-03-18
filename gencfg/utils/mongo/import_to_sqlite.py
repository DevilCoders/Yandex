#!/skynet/python/bin/python
"""
    Import data to sqlite table from mongo (for better future analysis)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from argparse import ArgumentParser
import pymongo
import msgpack
from collections import defaultdict
import sqlite3

import gencfg
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types
from core.instances import Instance
from core.igroups import IGroup
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB

from mongopack import Serializer

SIGNALS = ['instance_cpu_usage', 'host_cpu_usage', 'host_cpu_usage_rough', 'instance_mem_usage', 'host_mem_usage',
           'host_mem_usage_rough', 'instance_count',
           'host_count', 'host_count_rough', 'instance_power', 'host_power', 'host_power_rough']
TIMESTEP = 60 * 60


def get_parser():
    parser = ArgumentParserExt(description="Import data to sqlite tables from mongo")
    parser.add_argument("--startt", type=argparse_types.xtimestamp, required=True,
                        help="Obligatory. Get result since specified timestamp (e.g. '14d' for date 14 days ago, '4h' for date 4 hour ago)")
    parser.add_argument("--endt", type=argparse_types.xtimestamp, required=True,
                        help="Obligatory. Get result since specified timestamp (e.g. '14d' for date 14 days ago, '4h' for date 4 hour ago)")
    parser.add_argument("--signals", type=argparse_types.comma_list, required=True,
                        help="Obligatory. List of signals to load from mongo")
    parser.add_argument("-o", "--output-file", type=str, required=True,
                        help="Obligatory. Sqlite filename")

    return parser


def normalize(options):
    not_found_signals = filter(lambda x: x not in SIGNALS, options.signals)
    if len(not_found_signals) > 0:
        raise Exception("Signals <%s> not found in mongo" % ",".join(not_found_signals))


def main(options):
    groups_by_host_port = dict(map(lambda x: ((x.host.name, x.port), x.type), CURDB.groups.get_all_instances()))

    mongocoll = get_mongo_collection('instanceusage')

    mongoresult = mongocoll.find(
        {'ts': {'$gte': (options.startt / TIMESTEP) * TIMESTEP,
                '$lte': ((options.endt / TIMESTEP) * TIMESTEP + TIMESTEP)}},
        {'host': 1, 'ts': 1, 'binary': 1, '_id': 0}
    )

    records = []
    for elem in mongoresult:
        host = elem['host']
        ts = elem['ts']
        for instancedata in Serializer.deserialize(msgpack.unpackb(elem['binary'])):
            groupname = groups_by_host_port.get((host, instancedata['port']), 'UNKNOWN')
            record = [host, instancedata['port'], groupname, ts] + map(lambda x: instancedata.get(x, 0),
                                                                       options.signals)
            records.append(record)

    if os.path.exists(options.output_file):
        os.remove(options.output_file)

    conn = sqlite3.connect(options.output_file)
    c = conn.cursor()

    # create table
    column_names = ['host', 'port', 'groupname', 'ts'] + options.signals
    column_types = ['text', 'integer', 'text', 'integer'] + ['real'] * len(options.signals)
    column_descriptions = ",".join(map(lambda (x, y): "%s %s" % (x, y), zip(column_names, column_types)))

    c.execute("CREATE TABLE data (%s)" % column_descriptions)

    for record in records:
        c.execute("INSERT INTO data VALUES (%s)" % ",".join("?" * len(column_names)), tuple(record))
    conn.commit()


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
