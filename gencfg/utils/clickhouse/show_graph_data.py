#!/skynet/python/bin/python
"""Show data used to create gencfg/qloud/abc graphs (RX-444)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import Counter

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

from gaux.aux_clickhouse import ts_to_event_date, run_query


QLOUD_NODE_TYPES = ('installType', 'project', 'app', 'environ', 'service')
SUPPORTED_SIGNALS = ('mem_usage', 'cpu_usage', 'cpu_usage_power_units', 'cpu_usage_cores', 'net_rx', 'net_tx')


class EActions(object):
    """Actions for this utility"""
    QLOUD_RAW = "qloud_raw"  # raw data for qloud
    ALL = [QLOUD_RAW]


def get_parser():
    parser = ArgumentParserExt(description='Show graphs data for gencfg/qloud/abc graphs')
    parser.add_argument('-a', '--action', type=str, default = EActions.QLOUD_RAW,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('--qloud-project', type=str, default=None,
                        help='Optional. Qloud project name to get stats for (e. g. <installType=stable> or <installType=stable,project=adaptive-distribution,app=adadis-server>')
    parser.add_argument('-s', '--signal', type=str, required=True,
                        choices=SUPPORTED_SIGNALS,
                        help='Obligatory. Signal name')
    parser.add_argument('--startts', type=argparse_types.xtimestamp, default='4h',
                        help='Optional. Start timestamp (e. g. <1428343437> for seconds since epoch or <2d3h> two days 3 hours ago)')
    parser.add_argument('--endts', type=argparse_types.xtimestamp, default='0h',
                        help='Optional. Finish timestamp (e. g. <1428343437> for seconds since epoch or <2d3h> two days 3 hours ago)')

    return parser


def validate_qloud_project(qloud_project_name):
    node_names = [x.partition('=')[0] for x in qloud_project_name.split(',')]

    # check for dublicates
    dublicate_names = [item for item, count in Counter(node_names).items() if count > 1]
    if len(dublicate_names):
        raise Exception('Found dublicates <{}>'.format(','.join(dublicate_names)))

    # check for unknown node types
    unknown_names = set(node_names) - set(QLOUD_NODE_TYPES)
    if unknown_names:
        raise Exception('Found uknown names <{}> (only <{}> allowed'.format(','.join(unknown_names), ','.join(QLOUD_NODE_TYPES)))

    # check for missing node names, like specified <installType> and <app> and not specified <project>
    for i in xrange(len(node_names)):
        if QLOUD_NODE_TYPES[i] not in node_names:
            raise Exception('Missing node <{}>'.format(QLOUD_NODE_TYPES[i]))


def gen_qloud_project_conditions(qloud_project_name):
    conditions = []
    for node_name, _, node_value in (x.partition('=') for x in qloud_project_name.split(',')):
        conditions.append("{} = '{}'".format(node_name, node_value))

    if len(conditions):
        return 'AND ' + ' AND '.join(conditions)
    else:
        return ''


def main_qloud_raw(options):
    validate_qloud_project(options.qloud_project)

    start_ed = ts_to_event_date(options.startts)
    end_ed = ts_to_event_date(options.endts)

    query = ("SELECT host, instanceId, ts, {signal_name} FROM qloudusage_aggregated_2m "
             "    WHERE eventDate >= '{start_ed}' AND eventDate <= '{end_ed}' AND ts >= {start_ts} AND ts <= {end_ts} "
             "          {project_conditions} "
             "    ORDER BY ts, host, instanceId").format(signal_name=options.signal, start_ed=start_ed, end_ed=end_ed,
                                                         start_ts=options.startts, end_ts=options.endts,
                                                         project_conditions=gen_qloud_project_conditions(options.qloud_project))

    for elem in run_query(query):
        print ';'.join(elem)


def main(options):
    if options.action == EActions.QLOUD_RAW:
        main_qloud_raw(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
