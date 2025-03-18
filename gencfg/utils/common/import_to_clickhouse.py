#!/skynet/python/bin/python
"""
    Import gencfg db data to clickhouse
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import urllib2
import datetime
import time

import gencfg
import config
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from core.argparse.types import gencfg_db
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen
from gaux.aux_colortext import red_text
import pytz


sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'putilin_graphs')))
from zoom_data import run_query


def ts_to_event_date(ts):
    """Convert ts to datetime (for clickhouse queries)"""
    return datetime.datetime.fromtimestamp(int(ts), tz=pytz.timezone("Europe/Moscow")).strftime('%Y-%m-%d')


def is_commit_uploaded(commit):
    """Check if commit is in database already"""
    now = int(time.time())
    start_ed = ts_to_event_date(now - 24 * 60 * 60)
    end_ed = ts_to_event_date(now)

    query = "SELECT count(*) FROM group_allocated_resources WHERE eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' and commit = {commit}".format(
            start_ed=start_ed, end_ed=end_ed, commit=commit
    )

    result = run_query(query)

    return len(result) > 0


def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in xrange(0, len(l), n):
        yield l[i:i + n]


class TTableData(object):
    def __init__(self, ts, commit, table_name, table_fields):
        self.ts = ts
        self.commit = commit
        self.table_name = table_name
        self.table_fields = table_fields
        self.data = []

    def add_data(self, elem):
        assert(len(self.table_fields) == len(elem)), "Table elems mismatch: fields <%s> and data <%s>" % (self.table_fields, elem)
        self.data.append(elem)

    def show(self, verbose_level=0):
        s = ["    Insert <%s> elems to table <%s>; ts = %s, commit = %s:" % (len(self.data), self.table_name, self.ts, self.commit)]

        if verbose_level > 1:
            rows = [self.table_fields] + self.data
            lengths = []
            for lst in map(list, zip(*rows)):
                lengths.append(max(map(lambda x: len(str(x)), lst)))
            for row in rows:
                line = ' '.join(str(col) + ' ' * (lengths[i] - len(str(col))) for i, col in enumerate(row))
                s.append("        %s" % line)

        return "\n".join(s)

    def update(self):
        # XXX: ad-hoc
        values = [
            "({ts}, '{event_date}', {commit}, {other})".format(
                ts=int(self.ts),
                event_date=datetime.datetime.fromtimestamp(int(self.ts)).strftime('%Y-%m-%d'),
                commit=self.commit,
                other=", ".join(fields)
            )
            for fields in self.data
        ]
        query_fields = ", ".join(("ts", "eventDate", "commit") + self.table_fields)
        for values_chunk in chunks(values, 1000):
            print "INSERTing to {} {} records; ts={} commit={}".format(self.table_name, len(values_chunk), self.ts, self.commit)
            query = "INSERT INTO {} ({}) VALUES {}".format(self.table_name, query_fields, ", ".join(values_chunk))
            print run_query(query)


class EActions(object):
    SHOW = "show"
    UPDATE = "update"
    ALL = [SHOW, UPDATE]


def get_parser():
    parser = ArgumentParserExt("""Import gencfg db data to clickhouse""")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices = EActions.ALL,
                        help="Obligatory. Action to execute: one of <%s>" % ",".join(EActions.ALL))
    parser.add_argument("--db", type=gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity (maximum is 2)")

    return parser


def get_qloud_allocated_resources_for_install_type(data):
    QLOUD_NODE_TYPES = ('installType', 'project', 'app', 'environ', 'service')

    def gen_key(key, l):
        if  l:
            generated_key = ['{}={}'.format(QLOUD_NODE_TYPES[x], key[x]) for x in range(l)]
            generated_key = 'qloud:{}'.format(','.join(generated_key))
        else:
            generated_key = 'qloud:'

        return generated_key


    result = []
    for key_length in range(len(QLOUD_NODE_TYPES) + 1):
        cpu_by_key = defaultdict(float)
        memory_by_key = defaultdict(float)
        net_by_key = defaultdict(float)
        for full_key, cpu_usage, memory_usage, net_usage in data:
            cpu_by_key[gen_key(full_key, key_length)] += cpu_usage
            memory_by_key[gen_key(full_key, key_length)] += memory_usage
            net_by_key[gen_key(full_key, key_length)] += net_usage
        for key in memory_by_key:
            result.append((key, cpu_by_key[key], memory_by_key[key], net_by_key[key]))

    # for key, cpu, memory in result:
    #     print 'Group {}: {:.3f} cpu {:.3f} memory'.format(key, cpu, memory)

    return result


def get_qloud_allocated_resources(options):
    """Get allocated resources for qloud (QLOUD-2364)"""
    DATA = [
        ('test', SETTINGS.services.qloud.allocated_resources.test.url),
        ('prestable', SETTINGS.services.qloud.allocated_resources.prestable.url),
        ('stable', SETTINGS.services.qloud.allocated_resources.stable.url),
    ]

    result = []
    for install_type, url in DATA:
        headers = {'Authorization' : 'OAuth {}'.format(config.get_default_oauth())}
        rq = urllib2.Request(url, None, headers)

        try:
            install_type_data = retry_urlopen(2, rq)
        except Exception as e:
            pass
        else:
            install_type_data = install_type_data.strip().split('\n')
            install_type_data = [x.strip().split('\t') for x in install_type_data]
            install_type_data = install_type_data[1:]
            install_type_data = [([install_type] + x[:4], float(x[6]) * 40, float(x[8]) / 1024. / 1024. / 1024, float(x[13])) for x in install_type_data]

            result.extend(get_qloud_allocated_resources_for_install_type(install_type_data))

    result = [dict(group=x, cpu_guarantee=y, memory_guarantee=z, net_guarantee=t) for (x, y, z, t) in result]
    result = dict(
        objects=result,
        table_name='group_allocated_resources',
        fields=[
            ('group', lambda x: "'{}'".format(x['group'])),
            ('memory_guarantee', lambda x: '{:.3f}'.format(x['memory_guarantee'])),
            ('cpu_guarantee', lambda x: '{:.3f}'.format(x['cpu_guarantee'])),
            ('net_guarantee', lambda x: '{:.0f}'.format(x['net_guarantee'])),
            ('n_instances', lambda x: '1'),
        ]
    )

    return result


def main(options):
    if options.action in (EActions.SHOW, EActions.UPDATE):
        last_commit = options.db.get_repo().get_last_commit()
        commit_id = last_commit.commit
        ts = last_commit.date

        FILLERS = [
            {
                "objects" : options.db.groups.get_groups(),
                "table_name" : "group_allocated_resources",
                "fields" : [
                    ("group", lambda group: "'{}'".format(group.card.name)),
                    ("memory_guarantee", lambda group: "%.2f" % (len(group.get_kinda_busy_instances()) * group.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024)),
                    ("cpu_guarantee", lambda group: "%.2f" % sum(map(lambda instance: instance.power, group.get_kinda_busy_instances()))),
                    ("net_guarantee",lambda group: '%.0f' % (len(group.get_kinda_busy_instances()) * group.card.reqs.instances.net_guarantee.value)),
                    ("n_instances", lambda group: str(len(group.get_kinda_busy_instances()))),
                    ("host_memory_total", lambda group: '{:.2f}'.format(sum(x.memory for x in group.getHosts()))),
                    ("host_cpu_total", lambda group: '{:.2f}'.format(sum(x.power for x in group.getHosts()))),
                ],
            },
            get_qloud_allocated_resources(options),  # QLOUD-2364
            {
                "objects" : options.db.hosts.get_hosts(),
                "table_name" : "host_resources",
                "fields" : [
                    ("host", lambda host: "'{}'".format(host.name)),
                    ("cpu_power", lambda host: "%.2f" % host.power),
                    ("cpu_cores", lambda host: "%d" % host.ncpu),
                ],
            },
#            {
#               "objects" : options.db.groups.get_all_instances(),
#               "table_name" : "instance_resources",
#               "fields" : [
#                   ("hostname", lambda instance: instance.host.name),
#                   ("port", lambda instance: instance.port),
#                   ("power", lambda instance: instance.power),
#                   ("group", lambda instance: instance.type),
#               ],
#           },
        ]

        tables = []
        for filler in FILLERS:
            field_names, field_funcs = zip(*filler["fields"])
            table = TTableData(ts, commit_id, filler["table_name"], field_names)
            for elem in filler["objects"]:
                table.add_data(map(lambda f: f(elem), field_funcs))
            tables.append(table)

        if options.action == EActions.SHOW:
            if options.verbose > 0:
                print "Generated data for <%s> tables:" % (len(tables))
                for table in tables:
                    print table.show(options.verbose)
        elif options.action == EActions.UPDATE:
            if is_commit_uploaded(commit_id):
                print red_text('Commit {} already uploaded to clickhouse'.format(commit_id))
            else:
                if options.verbose > 0:
                    print "Inserting data from <%s> tables to clickhouse:" % (len(tables))
                for table in tables:
                    table.update()

        return tables
    else:
        raise Exception("Unknown action <%s>" % options.action)

def jsmain(d):
    options = get_parser().parse_json(d)

    return main(options)

if __name__ == '__main__':

    options = get_parser().parse_cmd()
    main(options)
