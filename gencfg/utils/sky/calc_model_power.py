#!/skynet/python/bin/python
import api.cqueue
from kernel.util.errors import formatException
import os
import sys
import threading
from collections import defaultdict
import subprocess
import time
import tempfile

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg
from config import TEMPFILE_PREFIX


class JobFile(object):
    def __init__(self):
        self.filename = 'calc_model_power.jobs'
        self.jobs = defaultdict(list)
        self.active_jobs = defaultdict(list)
        self.lock = threading.RLock()
        if os.path.exists(self.filename):
            globals = {}
            execfile(self.filename, globals, globals)
            if 'jobs' in globals:
                assert (isinstance(globals['jobs'], dict))
                for name, hosts in globals['jobs'].items():
                    assert (isinstance(hosts, list))
                    for host in hosts:
                        self.add_job(name, host)

    def is_empty(self):
        self.lock.acquire()
        try:
            return not os.path.exists(self.filename)
        finally:
            self.lock.release()

    def __str__(self):
        return 'jobs = %s\nactive jobs = %s' % (repr(dict(self.jobs)), repr(dict(self.active_jobs)))

    def flush(self):
        self.lock.acquire()
        try:
            with open(self.filename, 'w') as f:
                # join jobs and active_jobs
                obj = dict([(key, self.jobs[key] + self.active_jobs[key]) for key in
                            self.jobs.keys() + self.active_jobs.keys()])
                print >> f, '%s = %s' % ('jobs', repr(obj))
                f.close()
        finally:
            self.lock.release()

    def reset(self):
        self.lock.acquire()
        try:
            self.jobs = defaultdict(list)
            self.active_jobs = defaultdict(list)
            os.unlink(self.filename)
            self.flush()
        finally:
            self.lock.release()

    def start_job(self, name):
        self.lock.acquire()
        try:
            jobs = self.jobs[name]
            if not jobs:
                return None
            job = jobs[0]
            self.jobs[name] = jobs[1:]
            self.active_jobs[name].append(job)
            # no need to flush
            # self.flush()
            return job
        finally:
            self.lock.release()

    def end_job(self, name, host):
        self.lock.acquire()
        try:
            if host not in self.active_jobs[name]:
                print 'Warning: no such job "%s" for "%s"' % (name, host)
                return
            index = self.active_jobs[name].index(host)
            self.active_jobs[name].pop(index)
            self.flush()
        finally:
            self.lock.release()

    def add_job(self, name, host):
        assert (isinstance(name, str))
        assert (isinstance(host, str))
        self.lock.acquire()
        try:
            if host in self.jobs[name]:
                print 'Warning: job "%s" for "%s" is already in list' % (name, host)
                return
            self.jobs[name].append(host)
            self.flush()
        finally:
            self.lock.release()


basepath = '/var/tmp/sepe-model-test'


def sky_run(host, obj, timeout=60):
    client = api.cqueue.Client('cqudp', netlibus=True)
    results = list(client.run([host], obj).wait(timeout))
    if not results:
        raise Exception('Timeout on %s' % host)
    for host, result, failure in results:
        if failure:
            raise Exception('host %s: %s' % (host, formatException(failure)))
        total_result = result
    return total_result


class HostCleaner(object):
    def run(self):
        subprocess.call(['rm', '-r', basepath])
        subprocess.call(['mkdir', basepath])


def clean_host(host):
    return sky_run(host, HostCleaner())


def subprocess_output(cmd):
    hfile, filename = tempfile.mkstemp(prefix=TEMPFILE_PREFIX)
    ret = subprocess.call(cmd, stdout=hfile)
    os.close(hfile)
    output = open(filename).read()
    os.unlink(filename)
    return ret, output


class Sharer(object):
    shared_sources = dict()

    def __init__(self, path):
        self.path = path

    def run(self):
        os.chdir(os.path.dirname(self.path))
        ret, output = subprocess_output(['sky', 'share', os.path.basename(self.path)])
        if ret:
            raise Exception('Cmd failed ret=%s' % ret)
        return output.rstrip('\n')


def share(path):
    if ':' in path:
        host, local_path = path.split(':', 1)
        assert (os.path.isabs(local_path))
    else:
        host = 'localhost'
        local_path = os.path.abspath(path)
    norm_path = '%s:%s' % (host, local_path)
    if norm_path not in Sharer.shared_sources:
        output = sky_run(host, Sharer(local_path), timeout=30 * 60)
        Sharer.shared_sources[norm_path] = output
    return Sharer.shared_sources[norm_path]


class Copier(object):
    def __init__(self, torrent, dst, filename):
        self.torrent = torrent
        self.dst = dst
        self.filename = filename

    def run(self):
        os.chdir(self.dst)
        ret = subprocess.call(['rm', '-rf', self.filename])
        if ret:
            raise Exception('Cmd failed ret=%s' % ret)
        ret = subprocess.call(['sky', 'get', '-w', self.torrent])
        if ret:
            raise Exception('Cmd failed ret=%s' % ret)


def copy_smth(from_path, to_path):
    torrent = share(from_path)
    if ':' in to_path:
        host, local_path = to_path.split(':', 1)
        assert (os.path.isabs(local_path))
    else:
        host = 'localhost'
        local_path = os.path.abspath(to_path)
    sky_run(host, Copier(torrent, local_path, os.path.basename(from_path)), timeout=30 * 60)


def copy_db(host):
    copy_smth('ws32-014:/var/tmp/mishex/db', '%s:%s' % (host, basepath))


class OSGetter(object):
    host_os = dict()

    def run(self):
        return os.uname()[0]


def get_os(host):
    if host not in OSGetter.host_os:
        host_os = sky_run(host, OSGetter())
        OSGetter.host_os[host] = host_os
    return OSGetter.host_os[host]


def copy_exe(host):
    host_os = get_os(host)
    if host_os != 'FreeBSD':
        raise Exception('Unsupported OS: %s' % host_os)
    copy_smth('ws32-014:/var/tmp/mishex/exe', '%s:%s' % (host, basepath))


def copy_plan(host):
    host_os = get_os(host)
    if host_os != 'FreeBSD':
        raise Exception('Unsupported OS: %s' % host_os)
    copy_smth('ws32-014:/var/tmp/mishex/plan', '%s:%s' % (host, basepath))


class BaseSrchRunner(object):
    # requirements that differs from default base search:
    # filename = srch-base
    # loadlog goes to ./loadlog
    # passagelog goes to ./passagelog

    pidfile = 'killpid'

    def __init__(self, start, kill):
        self.start = start
        self.kill = kill

    def run(self):
        if self.kill:
            os.chdir(os.path.join(basepath, 'exe'))
            if not os.path.exists(self.pidfile):
                return
            pid = open(self.pidfile).read().rstrip()
            try:
                subprocess.call(['kill', '-9', str(pid)])
            except Exception:
                pass

        if self.start:
            os.chdir(os.path.join(basepath, 'exe'))
            ret = subprocess.call(
                ['./srch-base', '-p', '8030', '-V', 'Loadlog=/dev/null', '-V', 'PassageLog=/dev/null', '-V',
                 'IndexDir=../db', 'apache.ywsearch.cfg'])
            # in normal case ret is 1 !!!
            if ret not in [0, 1]:
                raise Exception('Cmd failed ret=%s' % ret)
            ret, output = subprocess_output(['top', '-n', '-Uskynet'])
            if ret:
                raise Exception('Cmd failed ret=%s' % ret)
            lines = output.split('\n')
            lines = [x for x in lines if x.find('srch-base') != -1]
            if len(lines) != 1:
                raise Exception('Could not find process')
            pid = int(lines[0].split()[0])
            f = open(self.pidfile, 'w')
            print >> f, '%s' % pid
            f.close()

            # wait until start
            while subprocess.call(['wget', '-q', '-O', '-', 'http://localhost:8030/yandsearch?text=1&ms=proto&hr=da']):
                time.sleep(1)


def run_basesrch(host, start, kill):
    return sky_run(host, BaseSrchRunner(start, kill), timeout=5 * 60)


class DolbiloRunner(object):
    def run(self):
        os.chdir(os.path.join(basepath, 'plan'))
        after_t = .0
        before_t = .0
        count = 6
        for iteration in range(count):
            if iteration == count - 1:
                before_t = time.time()
            subprocess.call(['./d-executor', '-s', '32', '-p', 'plan', '-o', 'output', '-m', 'finger', '-Q', '6000'])
            if iteration == count - 1:
                after_t = time.time()
        qps = 6000.0 / (after_t - before_t)
        return qps


def get_power(host):
    return sky_run(host, DolbiloRunner(), timeout=60 * 15)


if __name__ == '__main__':
    pass
