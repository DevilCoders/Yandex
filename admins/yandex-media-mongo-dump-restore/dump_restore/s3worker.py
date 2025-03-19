import logging
import subprocess
import re
import os
from datetime import date
from . import Monitor


class S3Woker:
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.log.name = __name__
        self.db = config.db
        self.host = config.host
        self.prefix = config.prefix
        self.bucket = config.s3bucket

        self.today = date.today().strftime("%Y%m%d")
        self.args = ['/usr/bin/s3cmd', '--no-progress', '-c', '/root/.s3cfg']
        self.fname = "{}/{}/{}.gz".format(self.prefix, date.today().strftime("%Y%m%d"), self.db)
        self.filename = self.db + ".gz"
        self.s3path = "s3://{}/backup/mongo/{}/".format(self.bucket, self.today)

        self.local_path = "{}/{}".format(self.prefix, self.filename)
        if config.monitor:
            self.mon = Monitor(config.monitor)
        else:
            self.mon = Monitor()

    def get_files_list(self):
        args = self.args + ['ls', self.s3path]
        self.log.info("Run s3cmd: {}".format(args))
        data = subprocess.check_output(args)
        files = re.findall(r"s3:[\w\-/.]+", data)
        return files

    def pullone(self, name):
        dst = "{}/{}".format(self.prefix, os.path.basename(name))
        args = self.args + ['get', name, dst]
        self.log.debug(self.args)
        if os.path.exists(dst):
            self.log.debug("Clean target file %s", dst)
            os.remove(dst)
        self.log.info("Run s3cmd: %s", args)
        rc = subprocess.check_output(args)
        self.log.info('Download finished with: %s.', rc)

    def push(self):
        args = self.args
        args.extend(['put', self.fname, self.s3path])
        self.log.info("Run s3cmd: {}".format(args))
        rc = subprocess.check_output(self.args)
        self.log.info('Upload finished with message: {}.'.format(rc))

    def pull(self):
        args = self.args
        fullpath = "{}{}".format(self.s3path, self.filename)
        args.extend(['get', fullpath, self.local_path])
        if os.path.exists(self.local_path):
            self.log.debug("Clean target file {}".format(self.local_path))
            os.remove(self.local_path)
        self.log.info("Run s3cmd: {}".format(args))
        rc = subprocess.check_output(self.args)
        self.log.info('Download finished with message: {}.'.format(rc))
