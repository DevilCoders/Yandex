#!/skynet/python/bin/python
"""
    Every host in our cluster consists of multiple (1 or more) storages, which are located on differend hard drives. In this util we are trying to find out, how
    hard drives is divided between storages (based on mount/mdstat info). Currently there are at most two storages on host: <ssd> and <hdd> , corresponding to mount
    points of hdd and ssd drives respectively
"""

import os
import sys
import tempfile

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
import re
import json

import gencfg
from core.argparse.parser import ArgumentParserExt
from gaux.aux_utils import run_command
from gaux.aux_storages import detect_configuration

class EActions(object):
    DETECT = "detect"
    CHECKSPEED = "checkspeed"
    ALL = [DETECT, CHECKSPEED]

def get_parser():
    parser = ArgumentParserExt(description = "Detect disk storages configuration")
    parser.add_argument("-a", "--action", type=str, required=True,
        help="Obligatory. Action to execute: %s" % ",".join(EActions.ALL))
    parser.add_argument("-v", "--verbose", action="count", default=0,
        help="Optional. Increase output verbosity (maximum 2)")

    return parser

FIO_TPL="""
[fio_test]
bs=%(bs)s
filename=%(disk)s
rw=%(mode)s
direct=1
buffered=0
ioengine=libaio
iodepth=1
numjobs=%(numjobs)s
norandommap
runtime=%(runtime)s
time_based
"""

def measure_speed(options):
    # detect partitions
    partitions = detect_partitions(options)
    partitions = filter(lambda x: re.match('^sd[a-z]$', x.name), partitions)
    if options.verbose > 0:
        print "Partitions:"
        for partition  in partitions:
            print "    %s" % str(partition)

    fio_configs = [
        ('rr_single', 4 * 1024, 'randread', 1, 15),
        ('rr_multi', 4 * 1024, 'randread', 16, 15),
        ('sr', 5 * 1024 * 1024, 'read', 1, 15),
    ]

    for partition in partitions:
        for signal_name, bs, mode, numjobs, runtime in fio_configs:
            config_body = FIO_TPL % {
                'bs': bs,
                'disk': '/dev/%s' % partition.name,
                'mode': mode,
                'numjobs': numjobs,
                'runtime': runtime,
            }

            fid, fname = tempfile.mkstemp()
            with open(fname, 'w') as f:
                f.write(config_body)

            # run test
            args = ["sudo", "/var/tmp/fio", fname, "--output-format=json"]
            code, out, err = run_command(args)

            # calculate result
            jsoned = json.loads(out)

            iops = 0
            for i in xrange(numjobs):
                iops += jsoned['jobs'][i]['read']['bw']

            if mode == 'randread':
                setattr(partition, signal_name, int(iops / 4))
            else:
                setattr(partition, signal_name, int(iops / 1024))

            if options.verbose > 0:
                print "Parttition <%s> (<%s>) signal <%s>: %s" % (partition.name, partition.disk_name, signal_name, getattr(partition, signal_name))

    for partition in partitions:
        print "%s: %s %s %s" % (partition.disk_name, partition.rr_single, partition.rr_multi, partition.sr)

    return partitions

def main(options):
    if options.action == EActions.DETECT:
        return detect_configuration(options.verbose)
    elif options.action == EActions.CHECKSPEED:
        return measure_speed(options)

def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
