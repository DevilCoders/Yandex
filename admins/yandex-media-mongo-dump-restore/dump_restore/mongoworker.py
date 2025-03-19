"""
Mongo Dump and Restore class
"""
from . import S3Woker
from . import Monitor
from datetime import date
from pymongo import MongoClient
import logging
import subprocess
import os
import re


class MongoWorker:

    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.s3cmd = S3Woker(config)
        self.config = config
        self.auth = self.config.auth or None

        self.log.debug(str(config.toDict()))
        if self.config.monitor:
            self.mon = Monitor(self.config.monitor)
        else:
            self.mon = Monitor()

    def gen_command(self):
        auth_args = ''
        opts = self.config.args or ''
        passwd = ''
        if self.config.url:
            host = self.config.url
            port = ''
        else:
            host, port = self.config.host.split(':')
        target_db = self.config.db

        if self.config.auth:
            uname = self.config.auth.username
            passwd = self.config.auth.password
            auth_db = self.config.auth.auth_db or target_db
            auth_args = '--authenticationDatabase {} -u {}'.format(auth_db, uname)

        if self.config['action'] == 'restore':
            fname = "{}/{}.gz".format(self.config.prefix, target_db)
            cmd = '/usr/bin/mongorestore --gzip --drop --archive={}'.format(fname)
        elif self.config.action == 'backup':
            fname = "{}/{}/{}.gz".format(
                self.config.prefix, date.today().strftime("%Y%m%d"), self.config.db)
            cmd = '/usr/bin/mongodump --gzip --archive={} -d {}'.format(fname, self.config.db)
        else:
            self.log.debug('Unknown action %s', self.config.action)
            return None

        if self.config.url:
            cmd += " {} --host '{}'".format(opts, host)
        else:
            cmd += " {} --host {} --port {}".format(opts, host, port)

        if auth_args:
            cmd += " {} ".format(auth_args)

        self.log.debug('Generated command to run: %s', cmd)
        # now after log add password
        if self.config.auth:
            cmd += ' -p {}'.format(passwd)

        return cmd

    @property
    def is_master(self):
        host = self.config.host or 'localhost:27017'
        client = MongoClient('mongodb://{}/'.format(host))
        is_primary = client.is_primary
        self.log.debug("Is primary: %s", is_primary)
        return is_primary

    def dump(self):
        prefix = self.config.prefix
        db = self.config.db
        fname = "{}/{}/{}.gz".format(prefix, date.today().strftime("%Y%m%d"), db)
        os.path.exists(os.path.dirname(fname)) or os.makedirs(os.path.dirname(fname))
        cmd = self.gen_command()
        try:
            rc = subprocess.check_output(cmd, shell=True)
        except (subprocess.CalledProcessError, OSError) as fail:
            self.log.error("Dump failed with %s", fail)
            self.clean()
            return
        self.log.info("Dump ended. {}".format(rc))
        self.s3cmd.push()
        self.clean()

    def restore(self):
        # check only if conecting to one host
        if not self.config.url:
            if not self.is_master:
                self.log.info("Can't start restore on secondary. Exit.")
                self.mon.write("0;OK")
                return
        self.s3cmd.pull()
        self.log.info("Run restore with generated command.")
        cmd = self.gen_command()
        try:
            rc = subprocess.check_output(cmd, shell=True)
        except (subprocess.CalledProcessError, OSError) as fail:
            self.mon.write("2;Restore failed")
            self.log.error("Restore failed with %s", fail)
            self.clean()
            return
        self.log.info("Restore finished. {}".format(rc))
        self.clean()
        self.mon.write("0;OK")

    def clean(self):
        if re.match(r'/$|/\*|/root[\w/]*|/etc[\w/]*|/lib[\w/]*|/usr/[\w/]*', self.config.prefix):
            self.log.debug("ACHTUNG! Dangerous remove try: {}".format(self.config.prefix))
            return False
        for root, dirs, files in os.walk(self.config.prefix, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        return True
