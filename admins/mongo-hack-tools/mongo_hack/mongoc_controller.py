from base_controller import BaseController
from misc import log


class MongocController (BaseController):

    def __init__(self, host):
        super(MongocController, self).__init__(host, 27019, '/etc/mongodbcfg.conf', 'mongodbcfg')

    @log
    def copy_from(self, remote_source):
        tmp = '/var/tmp/backup_restore'
        self.ssh.run('rm -rf %s' % tmp)
        self.ssh.run('rsync -avH %s %s' % (remote_source, tmp))
        self.ssh.run('mongorestore --port 27019 --drop --db config %s' % tmp)
