"""Backup realeted utils"""
import os
import re
import time
import select
import argparse
import logging
import threading
from .constants import HOSTNAME

class NonBlockingReader(object):
    """
    Read data from pipe if it readable
    and return lines
    """
    def __init__(self, fd):
        self._fd = fd
        self._buf = ''

    def fileno(self):
        """return file descriptor"""
        return self._fd

    def readlines(self):
        """collect and return lines"""
        try:
            readable = select.select([self._fd], [], [], 0.0001)[0]
            if not readable:
                return []
            data = os.read(self._fd, 4096)
        except (IOError, OSError, select.error):
            return []
        if not data: # EOF
            if self._buf:
                return self._buf.split('\n')
            return []

        self._buf += data
        if '\n' not in data:
            return []
        tmp = self._buf.split('\n')
        lines, self._buf = tmp[:-1], tmp[-1]
        return lines


class ThreadLogger(threading.Thread):
    """
    Run logger in separate thread
    """

    def __init__(self, logger, *args, **kwargs):

        self.stderr = kwargs.pop("stderr", None)
        self.stdout = kwargs.pop("stdout", None)
        self._reset_stream = threading.Event()
        self._reset_stream.set()

        super(ThreadLogger, self).__init__(*args, **kwargs)

        self._stop = threading.Event()
        self.logger = logger

    def start(self):
        """Demonize logger"""
        self.setDaemon(True)
        super(ThreadLogger, self).start()

    def run(self):
        err_stream = None
        info_stream = None
        while not self._stop.wait(0.01):
            if self._reset_stream.is_set():
                err_stream = self.stderr and NonBlockingReader(self.stderr)
                info_stream = self.stdout and NonBlockingReader(self.stdout)
                self._reset_stream.clear()

            if err_stream:
                for line in err_stream.readlines():
                    self.logger.error(line)
            if info_stream:
                for line in info_stream.readlines():
                    self.logger.debug(line)
        if err_stream:
            for line in err_stream.readlines():
                self.logger.error(line)
        if info_stream:
            for line in info_stream.readlines():
                self.logger.debug(line)

    def stop(self):
        """stop logger activity"""
        self._stop.set()

    def reset_streams(self, stdout=None, stderr=None):
        """reset input streams"""
        self.stdout = stdout
        self.stderr = stderr
        self._reset_stream.set()


def stopwatch():
    """Stopwatch with milliseconds precision"""

    def tick():
        """Tick function"""
        next_start = int(round(time.time() * 1000))
        tick.took = next_start - tick.start
        tick.start = next_start
        return tick.took
    tick.start = int(round(time.time() * 1000))
    return tick


def str2time(str_val):
    """Convert human readable string to seconds"""

    match = re.match(r'^(?P<num>[\d.]+)(?P<spec>[\s\w]*)', str(str_val).strip())
    if not match:
        raise ValueError("can't parse %s" % str_val)

    ago = float(match.group('num'))
    spec = match.group('spec').strip().lower()
    if not spec or spec.startswith('s'):
        pass
    elif spec.startswith('m'):
        ago = ago * 60
    elif spec.startswith('h'):
        ago = ago * 3600
    elif spec.startswith('d'):
        ago = ago * 3600 * 24
    else:
        raise ValueError("cant parse %s" % str_val)

    return ago


def check_output(cmd, expect="Failed to run"):
    """Run command and check it stdout"""
    log = logging.getLogger("check_output")

    (stdout, stderr) = cmd.communicate()
    if not stdout:
        log.error(expect)
        if stderr:
            log.error(stderr)
    return stdout


def backup_cname():
    """Get canonical name for backup directory from hostname"""
    hostname = HOSTNAME.replace(".yandex.net", "").replace(".yandex-team.ru", "")
    hostname = hostname.split(".")
    hostname[0] = hostname[0].rsplit("-", 1)[0]
    return "-".join(hostname)


def render_backup_config(conf):
    """Render backup config"""
    cname_tpl = '{cannonical-cluster-name}'
    cname = backup_cname()
    tpl = conf.backup.s3cmd.prefix
    if tpl and cname_tpl in tpl:
        conf.backup.s3cmd.prefix = tpl.replace(cname_tpl, cname)

    tpl = conf.backup.ftp.prefix
    if tpl and cname_tpl in tpl:
        conf.backup.ftp.prefix = tpl.replace(cname_tpl, cname)

    tpl = conf.backup.zk.lock
    if tpl and cname_tpl in tpl:
        conf.backup.zk.lock = tpl.replace(cname_tpl, cname)

    # fix zookeeper timeout
    # without this fix timeout to zk expires before lock timeout and
    # kazoo.acquire return True
    timeout = conf.backup.zk.timeout
    if timeout > conf.zookeeper.timeout:
        conf.zookeeper.timeout = timeout + 10

    if not conf.backup.api.mon.replica:
        conf.backup.api.mon.replica = "http://localhost:4417/mon/replica"

    return conf


def is_exists_and_younger_than(filename, timestr):
    """Check filename existence and age"""
    if os.path.exists(filename):
        maxdiff = str2time(timestr)
        age = os.stat(filename).st_mtime
        return (time.time() - age) < maxdiff
    return False



class ArgparserListParser(argparse.Action):  # pylint: disable=too-few-public-methods
    """Argparser store list action"""
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if nargs is not None:
            raise ValueError("nargs not allowed")
        super(ArgparserListParser, self).__init__(option_strings, dest, **kwargs)
    def __call__(self, parser, namespace, values, option_string=None):
        if " " in values:
            raise argparse.ArgumentError(self, "A comma-separated list expected")
        hosts = values.strip().split(",")
        setattr(namespace, self.dest, hosts)
