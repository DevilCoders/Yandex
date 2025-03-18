#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time

import gencfg
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt
from gaux.aux_mongo import get_mongo_collection

TIMESTEP = 60 * 60


def get_parser():
    parser = ArgumentParserExt(description="Print usage graph of multiple groups")
    parser.add_argument("-g", "--groups", type=argparse_types.comma_list, required=True,
                        help="Obligatory. List of groupnames")
    parser.add_argument("-i", "--signal", type=str, required=True,
                        help="Obligatory. Signal name")
    parser.add_argument("-s", "--startt", type=argparse_types.xtimestamp, required=True,
                        help="Obligatory. Start time")
    parser.add_argument("-e", "--endt", type=argparse_types.xtimestamp, required=True,
                        help="Obligatory. End time")
    parser.add_argument("--median-border", type=int, default=50,
                        help="Optional. Median value (in range [0, 100])")
    parser.add_argument("--median-width", type=int, default=1,
                        help="Optional. Number of hours to to calculate median")
    parser.add_argument("--compress", action="store_true", default=False,
                        help="optional. Compress output (make less points)")

    return parser


def normalize(options):
    if options.median_border < 0 or options.median_border > 100:
        raise Exception("Option --median-border must be in range [0, 100]")
    if options.median_width <= 0:
        raise Exception("Option --median-width must be > 0")

    options.startt -= options.startt % TIMESTEP
    options.endt = options.endt + TIMESTEP - options.endt % TIMESTEP


def medianate(data, median_border, median_width):
    result = []
    for i in range(len(data)):
        starti = i - median_width + 1
        if starti < 0:
            starti = 0

        endi = starti + median_width
        if endi > len(data):
            endi = len(data)

        result.append(sorted(data[starti:endi])[(endi - starti) * median_border / 100])

    return result


def compress(timed_data):
    return timed_data[0::6]


def main(options):
    mongocoll = get_mongo_collection('instanceusagegraphs')

    mongo_flt = {'ts': {'$gt': options.startt, '$lt': options.endt}, 'groupname': {'$in': options.groups}}
    mongo_proj = {'ts': 1, options.signal: 1, '_id': 0}

    print mongo_flt

    by_ts_result = [0.0] * ((options.endt - options.startt) / TIMESTEP)
    for elem in mongocoll.find(mongo_flt, mongo_proj):
        i = (elem['ts'] - options.startt) / TIMESTEP
        by_ts_result[i] += elem.get(options.signal, 0)
    by_ts_result = medianate(by_ts_result, options.median_border, options.median_width)

    result = map(lambda x: (options.startt + TIMESTEP * x, by_ts_result[x]), xrange(len(by_ts_result)))
    if options.compress:
        result = compress(result)

    return result


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)
    for ts, value in result:
        print "%s %s" % (time.strftime("%Y.%m.%d %H:%M", time.localtime(ts)), value)
