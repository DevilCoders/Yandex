#!/usr/bin/pymds


import argparse
from collections import defaultdict
import logging
import random
import socket
from subprocess import Popen, PIPE
import sys
from time import time, sleep

from infra.yasm.yasmapi import GolovanRequest
from mds.admin.library.python.sa_scripts.utils import detect_environment

logger = logging.getLogger('ns_link_usage')
HOSTNAME = socket.gethostname()
ENV = detect_environment()


def setup_logging(log_file='/var/log/sa/ns_link_usage.log'):
    _format = logging.Formatter("[%(asctime)s] [%(name)s] %(levelname)s: %(message)s")
    _handler = logging.FileHandler(log_file)
    _handler.setFormatter(_format)
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger().addHandler(_handler)


def batch(iterable, n=1):
    l = len(iterable)
    for ndx in range(0, l, n):
        yield iterable[ndx: min(ndx + n, l)]


def get_stats(host, namespaces, days):
    res = defaultdict(list)
    signal = 'conv(elliptics-server-commands.READ_NEW.disk.outside.size_dmmm, Mi)'
    signal_pattern = "itype=mdsstorage;ctype={env};prj={ns}:" + signal
    signals = {}
    for ns in namespaces:
        signals[signal_pattern.format(ns=ns, env=ENV)] = ns
    period = 86400
    et = time()
    st = et - period * days

    for x in batch(signals.keys(), 200):
        data = dict(
            GolovanRequest(
                host=host,
                period=period,
                st=st,
                et=et,
                fields=x,
                max_retry=4,
                load_segments=3000,
                read_from_stockpile=True,
            )
        )

        for ts in sorted(data):
            values = data[ts]
            for signal, val in values.iteritems():
                if val:
                    ns = signals[signal]
                    res[ns].append(float(val) / period)

    return res


def get_host_namespaces(ignore_namespaces):
    nss = set()
    ignore = set(['lrc-8-2-2', 'None', 'TIMEOUT'])
    if ignore_namespaces:
        ignore.update(set(ignore_namespaces.split(',')))

    p = Popen(['timeout', '180', '/usr/bin/elliptics-node-info.py'], stdout=PIPE, stderr=PIPE)
    out, err = p.communicate()
    code = p.returncode
    if err:
        raise RuntimeError("Failed to read /usr/bin/elliptics-node-info.py: {}".format(err))
    elif code == 124:
        raise RuntimeError("Failed to read /usr/bin/elliptics-node-info.py. Timeout")
    elif code != 0:
        raise RuntimeError(
            "Failed to read /usr/bin/elliptics-node-info.py. Exit code {}".format(code)
        )
    out = out.strip()
    for line in out.split('\n'):
        if line:
            ns = line.split()[-2]
            # namespace=avatars-yapic
            nss.add(ns.split('=')[-1])

    nss.difference_update(ignore)
    return list(nss)


def avg(data):
    return round(sum(data) / len(data), 1)


def monrun(stats, args):
    warn_limit_mi = int(args.monrun_warn)
    crit_limit_mi = int(args.monrun_crit)

    code = 0
    res = ''
    for ns, data in stats:
        if avg(data) > crit_limit_mi:
            code = 2
            res += "{}: {} MiB/s; ".format(ns, avg(data))
        elif avg(data) > warn_limit_mi:
            if code != 2:
                code = 1
            res += "{}: {} MiB/s; ".format(ns, avg(data))

    if not res:
        res = 'Ok'
    print "{};{}".format(code, res)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--hostname', default=HOSTNAME, help='Defaut hostname: {}'.format(HOSTNAME))
    parser.add_argument('-d', '--days', default=7, help="(default: %(default)s)", type=int)
    parser.add_argument('-n', '--namespaces')
    parser.add_argument('-i', '--ignore_namespaces', default='')
    parser.add_argument('-m', '--monrun', action='store_true')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument(
        '--monrun_warn',
        default=40,
        help="Warn limit per ns in MiB/s (default: %(default)s)",
        type=int,
    )
    parser.add_argument(
        '--monrun_crit',
        default=50,
        help="Warn limit per ns in MiB/s (default: %(default)s)",
        type=int,
    )
    parser.add_argument(
        '-s', '--sleep', default=0, help="Random sleep for (default: %(default)s)", type=int
    )
    args = parser.parse_args()
    return args


def main():
    args = get_args()
    setup_logging()
    logger.debug('Start with arguments: {}'.format(args))

    try:
        if args.hostname != HOSTNAME and args.namespaces is None:
            raise RuntimeError("Use flag -n, --namespaces for this hostname")

        sleep(random.randint(0, args.sleep))
        if args.namespaces:
            namespaces = args.namespaces.split(',')
        else:
            namespaces = get_host_namespaces(args.ignore_namespaces)

        stats = get_stats(args.hostname, namespaces, int(args.days))

        stats = sorted(stats.items(), key=lambda item: avg(item[1]), reverse=True)
        if args.monrun:
            monrun(stats, args)
        else:
            print "Stats for last {} days:".format(args.days)
            for ns, data in stats:
                msg = "Namespace: {}, read avg speed {} MiB/s".format(ns, avg(data))
                if args.debug:
                    debug_data = ["{} MiB/s".format(round(x, 1)) for x in data]
                    msg += ", {}".format(debug_data)
                logger.info(msg)
                print msg
    except Exception as e:
        logger.exception("Script failed")
        if args.monrun:
            if "elliptics-node-info" in str(e) or "HTTP Error 429" in str(e):
                print "0;Ok, but there are some issues"
            else:
                print "2;{}".format(e)
        else:
            print e
        sys.exit()


if __name__ == '__main__':
    main()
