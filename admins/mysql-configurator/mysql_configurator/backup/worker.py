"""
Worker run dumper and proxy it output to pusher
MySQLDump dumper split every table in separate file
XtraBackup stream all dbs in tar stream
"""

import logging
from subprocess import CalledProcessError, check_output

from .compressor import Gzip, Pigz, Pzstd, Zstd
from .dumper import MySQLDump, XtraBackup
from .pusher import FTPPusher, S3Pusher


class Worker(object):
    """
    Worker class see module docstring
    """

    BACKUP_FAILED_FLAG = "/var/tmp/mysql-backup-data-failed"
    ERROR_LOG_LINE = "The process has finished, but no success line seen"

    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)

        if conf.backup.transport == "s3cmd":
            self.log.debug("Use transport s3cmd")
            pusher = S3Pusher(conf.backup)
        else:
            self.log.debug("Use transport ftp (python ftplib)")
            pusher = FTPPusher(conf.backup)

        compression_level = conf.backup.compression_level or 6
        stop_replica = conf.backup.stop_replica
        self.command = conf.backup.command

        if conf.backup.compression == "gzip":
            pusher.setcompressor(Gzip(level=compression_level))
        elif conf.backup.compression == "pigz":
            pusher.setcompressor(Pigz(level=compression_level))
        elif conf.backup.compression == "zstd":
            pusher.setcompressor(Zstd(level=compression_level))
        elif conf.backup.compression == "pzstd":
            pusher.setcompressor(Pzstd(level=compression_level))

        if conf.backup.style == "xtrabackup":
            self.log.debug("Use XtraBackup dumper")
            self.dumper = XtraBackup(pusher)
        else:
            self.log.debug("Use MySQLDump dumper")
            self.dumper = MySQLDump(pusher, stop_replica)

    def run(self):
        "run backup"
        self.log.info("Run dumper")
        success = self.dumper.dump()
        if not success:
            with open(self.BACKUP_FAILED_FLAG, "w") as flag:
                # update flag mtime
                flag.write("")
            self.log.error(self.ERROR_LOG_LINE)
        self.action()

    def stop(self):
        "stop backup"

    def action(self):
        "run command after backup"
        if not self.command:
            return

        cmd = self.command.split()
        self.log.info("Run post backup command: %s", self.command)
        try:
            rcode = check_output(cmd)
            self.log.info("Post backup command finished successfully with message: %s", rcode)
        except CalledProcessError as err:
            self.log.warning("Command finished with non-zero exit code %s", err.returncode)
            self.log.debug("Command \'%s\' output messaage: %s", err.output, self.command)
