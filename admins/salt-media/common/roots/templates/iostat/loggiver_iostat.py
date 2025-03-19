#!/usr/bin/python

import sys
import os
import time


P = ['num1', 'num2', 'device', 'rd_ios', 'rd_merges', 'rd_sectors', 'rd_ticks', 'wr_ios', 'wr_merges', 'wr_sectors', 'wr_ticks', 'io', 'ticks', 'aveq']


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("--prev_file", help="[ %default ]", default='/tmp/loggiver_iostat.prev')
    parser.add_option("--disks", help="[ %default ]", default='sda sdb sdc sdd')
    parser.add_option("--mode", help="[ 0 - Max;1 - Average; 2 - Sum; 3 - All; Default - %default ]", default=0, type=int)
    parser.add_option("--fields", help="[ Fields in https://www.kernel.org/doc/Documentation/iostats.txt. Default - %default ]", default='10')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def parse_diskstat(stat):
    out = {}
    for line in stat:
        line = line.split('\n')[0].split()
        device = line[2]
        out[device] = {}
        for x in P:
            try:
                out[device][x] = int(line[P.index(x)])
            except ValueError:
                out[device][x] = line[P.index(x)]

    return out


def print_result(result, mode, fields):
    if len(result) == 0:
        sys.exit(0)
    for x in fields.split():
        x = int(x) + 2
        if mode == 0:
            print "%s %s" % (P[x], max(item[P[x]] for item in result.values()))
        elif mode == 1:
            print "%s %s" % (P[x], sum(item[P[x]] for item in result.values()) / len(result))
        elif mode == 2:
            print "%s %s" % (P[x], sum(item[P[x]] for item in result.values()))
        elif mode == 3:
            for disk, values in result.items():
                print "%s.%s %s" % (disk, P[x], values[P[x]])


def main(argv):
    global options
    options = parseArgs(argv)

    f = open('/proc/diskstats', 'r')
    stat = f.readlines()
    time_now = time.time()
    f.close()

    prev_file = options.prev_file
    if os.path.isfile(prev_file):
        time_prev = os.path.getmtime(prev_file)

        f = open(prev_file, 'r')
        stat_prev = f.readlines()
        f.close

        f = open(prev_file, 'w')
        f.writelines(stat)
        f.close()
    else:
        f = open(prev_file, 'w')
        f.writelines(stat)
        f.close()

        sys.exit(0)

    parse = parse_diskstat(stat)
    parse_prev = parse_diskstat(stat_prev)

    disks = options.disks.split()

    time_delta = time_now - time_prev
    result = {}
    for disk in disks:
        if disk in parse and disk in parse_prev:
            result[disk] = {}
            for x in P[3:]:
                result[disk][x] = int(round(((parse[disk][x] - parse_prev[disk][x]) / time_delta)))

    print_result(result, options.mode, options.fields)


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
