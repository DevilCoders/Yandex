#!/skynet/python/bin/python
"""
    Import data to sqlite table from mongo (for better future analysis)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from collections import defaultdict
import sqlite3
import gaux.aux_clickhouse

import gencfg
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types
from core.instances import Instance
from core.igroups import IGroup
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB


SIGNALS = ['instance_cpuusage']
STEPSIZE = 3600


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


def step(startt, endt, signals, groups_by_host_port, conn):
    print 'Geting data from <{}> to <{}>'.format(time.strftime('%Y-%m-%d %H:%M', time.localtime(startt)), time.strftime('%Y-%m-%d %H:%M', time.localtime(endt)))

    records = []

    # get data from clickhouse
    clickhouse_startt = time.strftime("%Y-%m-%d", time.localtime(startt))
    clickhouse_endt = time.strftime("%Y-%m-%d", time.localtime(endt))
    clickhouse_query = "SELECT host, port, ts, {} from instanceusage_aggregated_2m where ts > {} and ts < {} and eventDate >= '{}' and eventDate <= '{}' and port <> 65535".format(
        ', '.join(signals), startt, endt, clickhouse_startt, clickhouse_endt)
    for elem in gaux.aux_clickhouse.run_query(clickhouse_query):
        host = elem[0]
        port = int(elem[1])
        ts = elem[2]
        signals = elem[3:]
        groupname = groups_by_host_port.get((host, port), 'UNKNOWN')

        record = [host, port, groupname, ts] + list(signals)
        records.append(record)

    # write data to sqlite
    c = conn.cursor()
    for record in records:
        c.execute("INSERT INTO data VALUES (%s)" % ",".join("?" * len(record)), tuple(record))
    conn.commit()


def main(options):
    groups_by_host_port = dict(map(lambda x: ((x.host.name, x.port), x.type), CURDB.groups.get_all_instances()))

    if os.path.exists(options.output_file):
        os.remove(options.output_file)
    conn = sqlite3.connect(options.output_file)
    c = conn.cursor()

    column_names = ['host', 'port', 'groupname', 'ts'] + options.signals
    column_types = ['text', 'integer', 'text', 'integer'] + ['real'] * len(options.signals)
    column_descriptions = ",".join(map(lambda (x, y): "%s %s" % (x, y), zip(column_names, column_types)))

    c.execute('CREATE TABLE data ({})'.format(column_descriptions))
    c.execute('CREATE index groupname_index ON data(groupname)')

    for startt in range(options.startt, options.endt, STEPSIZE):
        endt = min(options.endt, startt + STEPSIZE)
        step(startt, endt, options.signals, groups_by_host_port, conn)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
