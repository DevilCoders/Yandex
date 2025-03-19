"""
Worker run dumper and proxy it output to pusher
MySQLDump dumper split every table in separate file
XtraBackup stream all dbs in tar stream
"""

import logging
from subprocess import Popen, CalledProcessError
from .exceptions import NotRunningException


class Compressor(object):
    """Common compressor implementation"""

    def __init__(self, level=5):
        self.log = logging.getLogger(self.__class__.__name__)
        self.level = level
        self.cmd = None
        self.ext = ".gz"
        self.args = []

    def run(self, pipe_in, pipe_out):
        """
        Run compressor subprocess
        """
        self.log.debug("Run compressor")
        if not self.args:
            raise CalledProcessError(returncode=1, cmd="Command not specified")

        self.cmd = Popen(self.args, stdin=pipe_in, stdout=pipe_out, close_fds=True)

    @property
    def stdin(self):
        "Pusher's underlying stdin"
        if self.cmd is None:
            raise NotRunningException(self.__class__)
        return self.cmd.stdin

    def stop(self):
        """Wait process completion"""
        self.cmd.stdin.close()
        self.cmd.wait()


class Gzip(Compressor):
    "Gzip"
    def __init__(self, *args, **kwargs):
        super(Gzip, self).__init__(*args, **kwargs)
        self.log.info("Init gzip with compression level %d", self.level)
        self.args = ['gzip', '-c', '-f', '-{}'.format(self.level)]

class Pigz(Compressor):
    "Pigz"
    def __init__(self, *args, **kwargs):
        super(Pigz, self).__init__(*args, **kwargs)
        self.log.info("Init pigz with compression level %d", self.level)
        self.args = ['pigz', '-c', '-f', '-{}'.format(self.level)]

class Zstd(Compressor):
    "Zstd see https://ya.cc/1oiOS (CADMIN-5179)"
    def __init__(self, *args, **kwargs):
        super(Zstd, self).__init__(*args, **kwargs)
        self.ext = ".zst"
        self.log.info("Init zstd with compression level %d", self.level)
        self.args = ['zstd', '-c', '-f', '-{}'.format(self.level)]

class Pzstd(Compressor):
    "Pzstd"
    def __init__(self, *args, **kwargs):
        super(Pzstd, self).__init__(*args, **kwargs)
        self.ext = ".zst"
        self.log.info("Init pzstd with compression level %d", self.level)
        self.args = ['pzstd', '-c', '-f', '-{}'.format(self.level)]

class Cat(Compressor):
    "Cat"
    def __init__(self, *args, **kwargs):
        super(Cat, self).__init__(*args, **kwargs)
        self.ext = ""
        self.args = ['cat']
