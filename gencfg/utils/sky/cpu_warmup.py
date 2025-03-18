#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import pwd

from argparse import ArgumentParser
import subprocess
import time
import socket
import signal

import api.kqueue
import api.cqueue
import api.copier
from kernel.util.errors import formatException

import gencfg
import core.argparse.types as argparse_types

BUNDLE_SKYNET_ID = "rbtorrent:4609dea77a03a874a76f9190f5e76ccfd79148cb"


class Dolbilo(object):
    def __init__(self, bundle_id, queries, workers, user, no_download_bundle):
        self.bundle_id = bundle_id
        self.queries = queries
        self.workers = workers
        self.user = user
        self.no_download_bundle = no_download_bundle

    def cleanup_old_runs(self):
        subprocess.call(["killall basesearch.linux"], shell=True)
        subprocess.call(["killall d-executor.linux"], shell=True)

    def download_base(self, workdir, no_download_bundle):
        try:
            os.mkdir(workdir)
        except:
            pass
        try:
            os.chmod(workdir, 0777)
        except:
            pass

        if no_download_bundle:
            if os.path.exists(os.path.join(workdir, 'basesearch.linux')) and \
                    os.path.exists(os.path.join(workdir, 'database', 'indexinv')):
                return

        copier = api.copier.Copier()
        copier.get(self.bundle_id, workdir).wait()

    def start_basesearch(self, workdir):
        os.chdir(workdir)

        base_p = subprocess.Popen(['./basesearch.%s' % os.uname()[0].lower(), '-d', 'apache.ywsearch.cfg'], stdout=None,
                                  stderr=None)
        for i in range(50):
            time.sleep(1)
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                s.connect(('127.0.0.1', 12345))
                started = True
                break
            except Exception:
                pass
        if not started:
            base_p.kill()
            raise Exception("Basesearch not started")

        return base_p

    def run_dolbilo(self):
        dolbilo_command = ['./d-executor.linux', '-p', './plan.bin', '-H', 'localhost', '-P', '12345',
                           '-m', 'finger', '-c', '-o', './test.out', '-s', str(self.workers), '-Q', str(self.queries)]

        f = open('/var/tmp/aa.txt', 'w')
        print >> f, dolbilo_command
        f.close()

        dolbilo_p = subprocess.Popen(dolbilo_command, stdout=None, stderr=None)
        dolbilo_p.wait()

        return dolbilo_p

    def run(self):
        workdir = os.path.join("/var/tmp", "warmup_%s" % self.user)

        self.cleanup_old_runs()

        self.download_base(workdir, self.no_download_bundle)

        base_p = self.start_basesearch(workdir)

        dolbilo_p = self.run_dolbilo()

        if dolbilo_p.returncode != 0:
            raise Exception("Dolbilo exited with non-zero status <%s>" % dolbilo_p.returncode)
        if base_p.returncode is not None:
            raise Exception("Basesearch exited prematurely with status <%s>" % base_p.returncode)

        os.kill(base_p.pid, signal.SIGKILL)

        return True


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")

    parser.add_argument("-s", "--hosts", type=argparse_types.grouphosts, required=True,
                        help="Obligatory. Comma-separated list of groups or hosts to process (groups converted to list of hosts). Example: MSK_WEB_BASE,ws2-200.yandex.ru,sas1-0200.search.yandex.net")
    parser.add_argument("-q", "--queries", type=int, required=True,
                        help="Obligatory. Number of queries")
    parser.add_argument("-b", "--bundle-skynet-id", type=str, default=BUNDLE_SKYNET_ID,
                        help="Optional. Bundle with database and basesearch")
    parser.add_argument("-w", "--workers", type=int, default=1,
                        help="Optional. Number of parallel threads")
    parser.add_argument("-t", "--timeout", type=int, default=100000,
                        help="Optinal. Maximal wait time (in seconds)")
    parser.add_argument("-u", "--user", type=str, dest="user", default=pwd.getpwuid(os.getuid())[0],
                        help="Optional. User name")
    parser.add_argument("--no-download-bundle", action="store_true", default=False,
                        help="Optional. Do not download bundle if find something looked like bundle in dest dir")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    hosts = map(lambda x: x.name, options.hosts)

    client = api.cqueue.Client('cqudp', netlibus=True)
    iterator = client.run(hosts, Dolbilo(options.bundle_skynet_id, options.queries, options.workers,
                                         options.user, options.no_download_bundle))

    for host, result, failure in iterator.wait(options.timeout):
        if failure is not None:
            if options.verbose_level > 0:
                print "Host <%s> failed" % host
                if options.verbose_level > 1:
                    print formatException(failure)
        else:
            if options.verbose_level > 0:
                print "Host <%s> succeeded" % host


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
