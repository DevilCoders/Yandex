"""
Puahers send data to backup storage
"""
import os
import ftplib
import logging
from threading import Thread, Event
from collections import namedtuple, defaultdict
from datetime import datetime
from subprocess import Popen, PIPE
from .utils import ThreadLogger, check_output, backup_cname
from .compressor import Cat


class S3Pusher(object):
    """S3cmd pusher"""
    appendable = False

    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.info("s3cmd pusher init")
        self.args = ['s3cmd', '--no-progress', 'put', '-']

        self.cmd = None
        self.compressor = None
        self.thread_logger = ThreadLogger(self.log)
        self.thread_logger.start()

        date = datetime.now().strftime("%F")
        if conf.s3cmd.prefix:
            self.path_prefix = os.path.join(conf.s3cmd.prefix, date)
        else:
            self.path_prefix = os.path.join("backup/mysql", backup_cname(), date)

        self.path_prefix = "s3://{0}".format(
            os.path.join(conf.s3cmd.bucket, self.path_prefix)
        )

    def reset(self, filename, append=False):
        """
        Split input stream into new filename
        """
        append = append   # s3cmd dont support append

        args = self.args[:]
        ext = self.compressor.ext if self.compressor else ""
        path = os.path.join(self.path_prefix, "{0}{1}".format(filename, ext))
        self.log.debug("Upload to %s", path)
        args.append(path)

        if self.cmd:
            if self.compressor:
                self.compressor.stop()
            self.cmd.stdin.close()
            self.cmd.wait()

        self.cmd = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)
        if self.compressor:
            self.compressor.run(PIPE, self.cmd.stdin)
        self.thread_logger.reset_streams(
            stdout=self.cmd.stdout.fileno(),
            stderr=self.cmd.stderr.fileno(),
        )

    @property
    def stdin(self):
        "Pusher's underlying stdin"
        if self.compressor:
            return self.compressor.stdin
        return self.cmd.stdin

    def setcompressor(self, compressor):
        """
        Add proxy compressor
        """
        self.compressor = compressor

    def stop(self):
        """Stop S3 uploader process"""
        if self.cmd:
            if self.compressor:
                self.compressor.stop()
            self.cmd.stdin.close()
            self.cmd.wait()
        self.thread_logger.stop()

    def list(self, subdir):
        """list remote content"""
        path = os.path.join(self.path_prefix, subdir)
        if path[-1] != '/':
            path += '/'

        self.log.debug("List remote %s", path)
        args = self.args[:2]
        args.extend(['ls', path])
        s3ls = Popen(args, stdout=PIPE, stderr=PIPE)
        logs = check_output(s3ls, "Can not list remote logs")

        remote_logs = defaultdict(int)

        for line in logs.splitlines():
            line = line.split()
            try:
                name = line[-1].split("/")[-1]
                size = int(line[2])
                remote_logs[name] = size
            except ValueError:
                self.log.debug("Skip line %s", line)
        return remote_logs


class FTPPusher(object):
    """ftplib pusher"""
    appendable = True

    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.info("ftp pusher init")

        self._stdin = None
        self.compressor = None

        self.client = _FTPClient(conf)
        self.client.start()

    def reset(self, filename, append=False):
        """
        Split input stream into new filename
        """
        ext = self.compressor.ext if self.compressor else ""
        path = os.path.join("{0}{1}".format(filename, ext))
        self.log.debug("Upload to %s", path)

        if self._stdin:
            if self.compressor:
                self.compressor.stop()
            self.client.wait_ftp()

        if not self.compressor:
            self.compressor = Cat()
        self.compressor.run(PIPE, PIPE)
        self._stdin = self.compressor.stdin

        if append:
            self.client.reset_streams(self.compressor.cmd.stdout, path, "APPE")
        else:
            self.client.reset_streams(self.compressor.cmd.stdout, path)

    @property
    def stdin(self):
        "Pusher's underlying stdin"
        return self.compressor.stdin

    def setcompressor(self, compressor):
        """
        Add proxy compressor
        """
        self.compressor = compressor

    def stop(self):
        """Stop S3 uploader process"""
        if self.compressor:
            self.compressor.stop()
        self.client.stop()
        self.client.join()

    def list(self, subdir):
        """list remote content"""
        self.log.debug("List remote %s", subdir)
        return self.client.list(subdir)

    def ensure_path(self, subdir):
        """list remote content"""
        path = os.path.join(self.client.path_prefix, subdir)
        self.log.debug("Ensure path on ftp server %s", path)
        self.client.ensure_path(subdir)


class _FTPClient(Thread):
    """
    Run ftplib client in thread
    """
    FTPFlags = namedtuple("FTPFlags", "reset wait stop")

    def __init__(self, conf, *args, **kwargs):
        self.log = logging.getLogger(self.__class__.__name__)

        self.flags = self.FTPFlags(reset=Event(), wait=Event(), stop=Event())
        self.cmd = "STOR"

        date = datetime.now().strftime("%F")
        if conf.ftp.prefix:
            self.path_prefix = os.path.join(conf.ftp.prefix, date)
        else:
            self.path_prefix = os.path.join("mysql", backup_cname(), date)

        self.client = ftplib.FTP(conf.ftp.host)
        self.client.connect()
        self.client.login()

        self.ensure_path(self.path_prefix)
        self.client.cwd(self.path_prefix)

        # initiaize thread
        self._stdin = None
        self.filename = None
        super(_FTPClient, self).__init__(*args, **kwargs)


    def ensure_path(self, pathname, silent=True):
        """Ensure path on ftp server"""
        path = []
        # ensure path
        for part in pathname.split("/"):
            if not part:
                continue
            path.append(part)
            try:
                self.client.mkd(os.path.join(*path))
            except Exception as exc:  # pylint: disable=broad-except
                if not silent:
                    self.log.error("ERRRRRRR from ftp.mkd, %s", exc)

    def start(self):
        """Demonize client"""
        self.setDaemon(True)
        super(_FTPClient, self).start()

    def stop(self):
        """Stop ftp client"""
        self.flags.stop.set()
        self.flags.reset.set()

    def run(self):
        while not self.flags.stop.wait(0.01):
            self.flags.wait.set()
            if self.flags.reset.wait():
                if self.flags.stop.isSet():
                    break
                self.flags.reset.clear()
                self.flags.wait.clear()

                input_obj = self._stdin
                fname = self.filename
                try:
                    self.client.storbinary(
                        "{0} {1}".format(self.cmd, fname),
                        input_obj
                    )
                except:  # pylint: disable=bare-except
                    self.log.exception("Failed to upload %s", fname)

    def reset_streams(self, stdin, filename, cmd="STOR"):
        """reset input streams"""
        self.close_stdin()
        self._stdin = stdin
        self.filename = filename
        self.cmd = cmd
        self.flags.reset.set()


    def close_stdin(self):
        """Close input buffer"""
        if self._stdin:
            self._stdin.close()
            self._stdin = None

    def wait_ftp(self):
        """Wait ftp upload"""
        self.flags.wait.wait()

    def list(self, subdir):
        """List content in subdir"""
        files = {}
        log = self.log
        def callback(line):
            """Callback for ftp.list"""
            line_arr = line.split()
            try:
                files[line_arr[-1]] = int(line_arr[4])
            except (ValueError, IndexError) as exc:
                log.warn("FTP.list skip line with err '%s': %s", exc, line)

        self.client.retrlines("LIST {0}".format(subdir), callback)
        return files
