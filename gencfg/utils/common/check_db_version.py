#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import threading
import re
from argparse import ArgumentParser
import time
import urllib2
import random
from collections import defaultdict

import gencfg
from gaux.aux_utils import print_progress
from core.db import CURDB
import core.argparse.types as argparse_types


class EMode(object):
    """Check db version for basesearchers/ints/intl2/mmeta (for non basesearchers check if config version is same)"""
    BASE = 'base'
    INT = 'int'
    INTL2 = 'intl2'
    ALL = [BASE, INT, INTL2]


def parse_cmd():
    parser = ArgumentParser(description="Check database versions of running instances")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, dest="intlookups", default=None,
                        help="Optional. List of intlookups")
    parser.add_argument('-m', '--mode', type=str, default=EMode.BASE,
                        choices=EMode.ALL,
                        help='Optional. Check mode: one of {} ({} by default)'.format(','.join(EMode.ALL), EMode.BASE))
    parser.add_argument("-s", "--sas-config", type=argparse_types.sasconfig, dest="sasconfig", default=None,
                        help="Optional. Sas config")
    parser.add_argument("-t", "--attempts", type=int, dest="attempts", default=1,
                        help="Optional. Number of attempts")
    parser.add_argument("-w", "--workers", type=int, dest="workers", default=10,
                        help="Optional. Number of workers")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.intlookups is None and options.sasconfig is None:
        raise Exception("You must specify at least one of --intlookups or --sas-config version")

    if options.intlookups is None:
        options.intlookups = []
    if options.sasconfig:
        options.intlookups.extend(map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), options.sasconfig))

    return options


class ThreadedDownloader(threading.Thread):
    def __init__(self, tid, mode, manager):
        threading.Thread.__init__(self)
        self.tid = tid
        self.manager = manager
        self.mode = mode
        if mode == EMode.BASE:
            self.pattern = re.compile('IndexDir ([^\n]+)')
        elif mode in (EMode.INT, EMode.INTL2):
            self.pattern = re.compile('ConfigVersion ([^\n]+)')
        else:
            raise Exception('Unknown mode <{}>'.format(mode))

    def download(self, instance, attempts):
        url = 'http://%s:%s/yandsearch?info=getconfig' % (instance.host.name, instance.port)
        body = ''
        for i in range(attempts):
            try:
                body = urllib2.urlopen(url, timeout=1).read()
                break
            except:
                time.sleep(random.uniform(0.0, 1.0))

        m = re.search(self.pattern, body)
        if m is None:
            self.manager.result[instance] = 'unknown'
        else:
            if self.mode == EMode.BASE:
                self.manager.result[instance] = m.group(1).split('-')[-1]
            else:
                self.manager.result[instance] = m.group(1)

    def run(self):
        while True:
            try:
                instance = self.manager.instances.pop()
            except IndexError:
                return
            self.download(instance, self.manager.attempts)


class Downloader(object):
    def __init__(self, workers, mode, attempts):
        self.workers = map(lambda x: ThreadedDownloader(x, mode, self), range(workers))
        self.attempts = attempts

    def download(self, instances):
        total_instances = len(instances)
        self.instances = instances
        self.result = dict()
        for worker in self.workers:
            worker.start()

        while len(instances):
            print_progress(total_instances - len(instances), total_instances)
            time.sleep(1.)
        print_progress(total_instances, total_instances, stopped=True)

        for worker in self.workers:
            worker.join()

        return self.result


def main(options):
    if options.mode == EMode.BASE:
        instances = sum(map(lambda x: x.get_base_instances(), options.intlookups), [])
    elif options.mode == EMode.INT:
        instances = sum(map(lambda x: x.get_int_instances(), options.intlookups), [])
    elif options.mode == EMode.INTL2:
        instances = sum(map(lambda x: x.get_intl2_instances(), options.intlookups), [])

    downloader = Downloader(options.workers, options.mode, options.attempts)

    print "Downloading:"
    stats = downloader.download(instances)

    print "Show stats:"
    reverse_stats = defaultdict(list)
    for k, v in stats.iteritems():
        reverse_stats[v].append(k)
    for k in sorted(reverse_stats.keys()):
        print "    %s (%s total): %s" % (k, len(reverse_stats[k]), ', '.join(map(lambda x: '%s:%s' % (x.host.name, x.port), reverse_stats[k][:10])))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
