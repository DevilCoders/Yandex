"""
Dumper dump mysql db and provide stream from Dumper.stdout property
"""
from __future__ import print_function

import logging
import os
import re
import sys
import time
# from threading import Thread
from subprocess import PIPE, Popen

from mysql_configurator.mysql import MySQL

from .utils import NonBlockingReader, stopwatch


class MySQLDump(object):
    """mysqldump dumper"""

    def __init__(self, pusher, stop_replica):
        self.log = logging.getLogger(self.__class__.__name__)
        self.args = ['/usr/bin/mysqldump', '--master-data',
                     '--single-transaction', '--loose-ignore-create-error',
                     '-A', '-R', '--skip-innodb-optimize-keys']
        self._id = 0
        self.dbname = ""
        self.fname = ""
        self.pusher = pusher
        self.stop_replica = stop_replica

    def dump(self):
        """run mysqldump process"""
        tick = stopwatch()
        self.pusher_file("intro")
        regexp = re.compile(
            r"^--\s*(?:"
            "Current Database: `(?P<dbname>.*?)`"
            "|"
            # pylint: disable=line-too-long
            r"(?:Temporary |Final )?(?:Table|table|view) structure for (?:table|view) `(?P<table_name>.*?)`"
            # pylint: enable=line-too-long
            "|"
            r"Dumping (?P<routines>routines) for database"
            "|"
            "Dump (?P<completed>completed)"
            ")"
        )

        buf_256mb_size = 256 << 20
        self.stop_slave()
        proc = Popen(self.args, stdout=PIPE, stderr=PIPE, bufsize=buf_256mb_size)
        errstream = NonBlockingReader(proc.stderr.fileno())
        success = False
        for line in iter(proc.stdout.readline, ""):
            for errline in errstream.readlines():
                self.log.error("Error while process file %s: %s", self.fname, errline)

            result = regexp.match(line)
            if result is None:
                self.pusher.stdin.write(line)
                continue

            old_fname = self.fname
            self.log.debug("Processing %s completed (took %sms)", old_fname, tick())
            if result.group("dbname") is not None:
                self.dbname = result.group("dbname")
                self.pusher_file(self.dbname)
            elif result.group("table_name") is not None:
                table = result.group("table_name")
                self.pusher_file("{0}-{1}".format(self.dbname, table))
            elif result.group("routines") is not None:
                self.pusher_file("{0}-{1}".format(self.dbname, "routines"))
            elif result.group("completed") is not None:
                success = True
                self.pusher_file("{0}-{1}".format(self.dbname, "success"))
            self.pusher.stdin.write(line)

        for errline in errstream.readlines():
            self.log.error("Error while process file %s: %s", self.fname, errline)

        self.start_slave()
        self.done()
        return success

    def pusher_file(self, name):
        """Set pusher output filename"""
        fname = self.make_filename(name)
        self.log.info("Process %s", fname)
        self.pusher.reset(fname)

    def make_filename(self, name="intro"):
        """Increase file id and build filename according format"""
        self._id += 1
        self.fname = "{0:0>#5d}-{1}.sql".format(self._id, name)
        return self.fname

    def done(self):
        """Complete dump pipeline"""
        self.pusher.stop()

    def stop_slave(self):
        """stops mysql slave"""
        if not self.stop_replica:
            return True
        mysql = MySQL()
        if not mysql.connect():
            self.log.warning("Failed to connect mysql")
            return False
        mysql.query("stop slave;")
        self.log.debug("Slave stopeed successfully.")
        return True

    def start_slave(self):
        """starts mysql slave"""
        if not self.stop_replica:
            return True
        mysql = MySQL()
        if not mysql.connect():
            self.log.warning("Failed to connect mysql")
            return False
        mysql.query("start slave;")
        self.log.debug("Slave started successfully.")
        return True


class XtraBackup(object):
    """xtrabackup dumper"""

    def __init__(self, pusher):
        self.log = logging.getLogger(self.__class__.__name__)
        self.pusher = pusher
        self.args = ['/usr/bin/innobackupex', '--stream=tar', '.']
        mysql = MySQL()
        if not mysql.connect():
            self.log.error("Failed to connect mysql")
            self.done()
            sys.exit(1)
        variables = mysql.query("show variables", as_dict=True)
        os.chdir(variables["datadir"])

    def dump(self):
        """"run xtrabackup process"""
        tick = stopwatch()

        self.pusher.reset("data.tar")
        buf_256mb_size = 256 << 20
        proc = Popen(self.args, stdout=self.pusher.stdin, stderr=PIPE, bufsize=buf_256mb_size)
        errstream = NonBlockingReader(proc.stderr.fileno())
        success = False
        while proc.poll() is None:
            for errline in errstream.readlines():
                if "Error:" in errline:
                    self.log.error(errline)
                if "completed OK!" in errline:
                    self.log.debug("Success line seen: %s", errline)
                    success = True
            time.sleep(1)

        for errline in errstream.readlines():
            if "Error" in errline or "error" in errline:
                self.log.error(errline)
            if "completed OK!" in errline:
                self.log.debug("Success line seen: %s", errline)
                success = True

        proc.wait()
        self.log.info("XtraBackup done (took %sms)", tick())
        self.done()
        return success

    def done(self):
        """Complete dump pipeline"""
        self.pusher.stop()
