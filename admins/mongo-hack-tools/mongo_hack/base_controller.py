from pymongo import MongoClient
from misc import log
from ssh import SSH
from time import sleep


class BaseController (object):

    def __init__(self, host, port, config_file, service):
        self.host = host
        self.port = port
        self.config_file = config_file
        self.service = service

        self.connect()
        self.ssh = SSH(host)
        self.config = self.parse_config(self.ssh.run('cat %s' % config_file, True))

    def __str__(self):
        return self.host

    def connect(self):
        for x in range(120):
            try:
                self.mongo = MongoClient(self.host, self.port)
                return
            except:
                print "try %s" % x
            sleep(1)

    def parse_config(self, text):
        result = {}

        for line in text.strip().split('\n'):
            if '=' in line:
                k, v = line.split('=', 1)
                result[k.strip()] = v.strip()
        return result

    @log
    def write_config(self):
        r = ''
        for k,v in self.config.iteritems():
            r += '%s = %s\n' % (k, v)
        self.ssh.run('cat >%s' % self.config_file, False, r)

    @log
    def stop(self):
        self.ssh.run('service %s stop || /bin/true' % self.service)

    @log
    def start(self):
        self.ssh.run('service %s start' % self.service)

    @log
    def restart(self):
        self.ssh.run('service %s restart || /bin/true' % self.service)
